#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <liburing.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

constexpr size_t BLOCK_SZ = 4096;
constexpr unsigned int QUEUE_DEPTH = 8;
constexpr size_t TOTAL_BUFFER_SIZE = BLOCK_SZ * QUEUE_DEPTH * 2;

struct RequestContext {
    uint8_t* buffer;
    off_t offset;
};

int main(int argc, char* argv[]) {
    if (argc != 2) [[unlikely]] {
        throw std::runtime_error("Exeecutable should be called with a file argument. Example: ./bench_io_uring <FILE_PATH>");
    }

    // use O_DIRECT to bypass the page cache
    int fd = open(argv[1], O_RDONLY | O_DIRECT);
    if (fd == -1) [[unlikely]] {
        throw std::runtime_error("Error occured when opening file. Error: " + std::string(strerror(errno)));
    }

    struct stat st;
    fstat(fd, &st);

    void* buffer = std::aligned_alloc(BLOCK_SZ, TOTAL_BUFFER_SIZE);
    if (buffer == nullptr) [[unlikely]] {
        throw std::runtime_error("Error occured when allocating buffer.");
    }

    struct io_uring ring;
    if (io_uring_queue_init(QUEUE_DEPTH * 2, &ring, 0) < 0) [[unlikely]] {
        throw std::runtime_error("io_uring_queue_init failed.");
    }

    // perform software pipelining of data
    size_t offset = 0;
    uint8_t in_flight = 0;
    std::vector<RequestContext> reqs(QUEUE_DEPTH * 2);
    for (unsigned int i = 0; i < QUEUE_DEPTH * 2; ++i) {
        reqs[i].buffer = static_cast<uint8_t*>(buffer) + (i * BLOCK_SZ);
    }

    for (int i = 0; i < QUEUE_DEPTH; ++i) {
        struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
        size_t read_size = std::min(BLOCK_SZ, st.st_size - offset);

        io_uring_prep_read(sqe, fd, reqs[i].buffer, read_size, offset);
        io_uring_sqe_set_data(sqe, &reqs[i]);

        reqs[i].offset = offset;
        offset += read_size;
        ++in_flight;
    }
    io_uring_submit(&ring);

    size_t total_bytes_read = 0;
    size_t start_sqes_idx = QUEUE_DEPTH;
    std::vector<uint8_t> histogram(256, 0);
    while (total_bytes_read < st.st_size) {
        struct io_uring_cqe* cqes;

        int ret = io_uring_wait_cqes(&ring, &cqes, in_flight, NULL, NULL);
        if (ret < 0) [[unlikely]] {
            throw std::runtime_error("io_uring_wait_cqes failed");
        }

        int next_in_flight = 0;
        // pipeline the next QUEUE_DEPTH requests first before processing
        for (int i = 0; i < QUEUE_DEPTH && offset < st.st_size; ++i) {
            struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
            size_t read_size = std::min(BLOCK_SZ, st.st_size - offset);
            
            io_uring_prep_read(sqe, fd, reqs[start_sqes_idx + i].buffer, read_size, offset);
            io_uring_sqe_set_data(sqe, &reqs[start_sqes_idx + i]);

            reqs[start_sqes_idx + i].offset = offset;
            offset += read_size;
            next_in_flight++;
        }
        if (next_in_flight > 0) {
            io_uring_submit(&ring);
        }
        start_sqes_idx ^= QUEUE_DEPTH;

        // process data
        unsigned int head;
        unsigned int processed_count = 0;
        io_uring_for_each_cqe(&ring, head, cqes) {
            if (cqes->res < 0) [[unlikely]] {
                throw std::runtime_error("io_uring async get failed: " + std::string(strerror(-cqes->res))); 
            }

            RequestContext* req = static_cast<RequestContext*>(io_uring_cqe_get_data(cqes));
            size_t bytes_processed = cqes->res;
            total_bytes_read += cqes->res;

            for (size_t j = 0; j < bytes_processed; ++j) {
                histogram[req->buffer[j]]++;
            } 
            ++processed_count;
        };

        io_uring_cq_advance(&ring, processed_count);
        in_flight = next_in_flight;
    }

    io_uring_queue_exit(&ring);
    free(buffer);
    close(fd);
}
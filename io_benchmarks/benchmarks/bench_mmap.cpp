#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 2) [[unlikely]] {
        throw std::runtime_error("Executable should be called with a file argument. Example: ./bench_mmap <FILE_PATH>");
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) [[unlikely]] {
        throw std::runtime_error("Error occurred when opening file. Error: " + std::string(strerror(errno)));
    }

    struct stat st;
    fstat(fd, &st);

    void* addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) [[unlikely]] {
        throw std::runtime_error("Error occured during mmap. Error: " + std::string(strerror(errno)));
    }
    madvise(addr, st.st_size, MADV_SEQUENTIAL | MADV_WILLNEED);

    // read data sequentially, perform some computation to simulate usage
    uint8_t* data = static_cast<uint8_t*>(addr);
    size_t num_elems = st.st_size / sizeof(uint8_t);
    
    std::vector<size_t> histogram(256, 0);
    for (size_t i = 0; i < num_elems; ++i) {
        histogram[data[i]]++;
    }

    // used to confirm correctness against other benchmarks
    std::cout << "Histogram results:\n<Value>:<Count>\n";
    for (int i = 0; i < 256; ++i) {
        std::cout << i << " : " << histogram[i] << '\n';
    }

    munmap(data, st.st_size);
    close(fd);
}
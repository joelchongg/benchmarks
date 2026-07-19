#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <size_in_GB> <file_path>"
    echo "Example: $0 5 ./data/test_file.bin"
    exit 1
fi

GB_SIZE=$1
FILE_PATH=$2

SIZE_IN_MB=$((GB_SIZE * 1024))

echo "Generating ${GB_SIZE}GB test file at ${FILE_PATH}..."

# create seed file of random data to speed up file generation
DIR_NAME=$(dirname "$FILE_PATH")
SEED_FILE="${DIR_NAME}/.seed.tmp"

dd if=/dev/urandom of="$SEED_FILE" bs=1M count=1 status=none

# clear target file if it already exists
> "$FILE_PATH"

for (( i=1; i<=SIZE_IN_MB; i++ )); do
    cat "$SEED_FILE" >> "$FILE_PATH"
done

rm "$SEED_FILE"

echo "Success: $(du -h "$FILE_PATH" | cut -f1) file generated."
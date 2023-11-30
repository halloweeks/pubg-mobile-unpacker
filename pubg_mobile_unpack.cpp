/*
 *  Author: Halloweeks
 *  Date: 2023-10-30
 *  Description: Extract pubg mobile pak contents and 
 *  the program not support extract pak after
 *  1.1.0 update of pubg mobile because change
 *  encryption algorithms and I don't know the algorithm and key 
 *  GitHub: https://github.com/halloweeks/
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <zlib.h>
#include <filesystem>

namespace fs = std::filesystem;

// The key use for deobfuscation of index offset
#define OFFSET_KEY 0xA6D17AB4D4783A41
#define BUFFER_SIZE 4096

// Pak header
typedef struct {
	uint32_t magic;
	uint32_t version;
	uint8_t hash[20];
	uint64_t size;
	uint64_t offset;
} __attribute__((packed)) PakInfo;

// Compression blocks
typedef struct {
    uint64_t CompressedStart;
    uint64_t CompressedEnd;
} __attribute__((packed)) CompressionBlock;

// decrypt xor 
void DecryptData(uint8_t *data, uint32_t size) {
	for (uint32_t index = 0; index < size; index++) {
		data[index] ^= 0x79;
	}
}

int CreateFile(const char *fullPath) {
	fs::path filePath(fullPath);
	fs::path extractedPath = filePath.parent_path();
	
	// Check if the provided path is not empty
    if (!extractedPath.empty()) {
        // Check if the parent path exists
        if (!fs::exists(extractedPath)) {
            try {
                fs::create_directories(extractedPath);
            } catch (const std::exception& e) {
                std::cerr << "Error creating path: " << e.what() << std::endl;
                return -1; // or any error code to indicate failure
            }
        }
    }
    return open(fullPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

int32_t DecompressBlock(const uint8_t *CompressedData, uint32_t CompressedSize, uint8_t *DecompressedData, uint32_t DecompressedSize) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    strm.avail_in = (uInt)CompressedSize;
    strm.next_in = (Bytef*)CompressedData;

    strm.avail_out = DecompressedSize;
    strm.next_out = (Bytef*)DecompressedData;

    if (inflateInit(&strm) != Z_OK) {
        fprintf(stderr, "Failed to initialize zlib.\n");
        return -1;
    }

    int ret = inflate(&strm, Z_FINISH);

    inflateEnd(&strm);

    if (ret != Z_STREAM_END) {
        fprintf(stderr, "Failed to decompress data.\n");
        return -1;
    }

    return DecompressedSize - strm.avail_out;
}

uint64_t current_data_index = 0;

void read_data(void *destination, const uint8_t *source, size_t length) {
	memcpy(destination, source + current_data_index, length);
	current_data_index += length;
}

int main(int argc, const char *argv[]) {
	PakInfo info;
	CompressionBlock Block[100];
	
	// Open input pak file
	int PakFile = open(argv[1], O_RDONLY);
	
	// opening failed
	if (PakFile == -1) {
		printf("Unable to open %s file\n", argv[1]);
		return 1;
	}
	
	// set file pointer into last 44 byte
	if (lseek(PakFile, -44, SEEK_END) == -1) {
		printf("failed to seek file position\n");
		return 0;
	}
	
	// read last 44 byte of pak header information
	if (read(PakFile, &info, 44) != 44) {
		printf("Failed to read pak header at -44\n");
		return 0;
	}
	
	info.offset = info.offset ^ OFFSET_KEY;
	
	// Get pak file size
	off_t fsize = lseek(PakFile, 0, SEEK_END);
	
	// set file pointer into index data
	if (lseek(PakFile, info.offset, SEEK_SET) == -1) {
		perror("Error seeking in file");
        close(PakFile);
        return 1;
	}
	
	// Calculate the index data size
	uint32_t IndexSize = fsize - info.offset;
	
	// Allocate memory for pak index data
	uint8_t *IndexData = (uint8_t*)malloc(IndexSize);
	
	// if allocation failed
	if (!IndexData) {
		printf("Failed to memory allocation\n");
		close(PakFile);
		return 3;
	}
	
	// read pak index data
	if (read(PakFile, IndexData, IndexSize) != IndexSize) {
		printf("Unable to load index data\n");
		return 4;
	}
	
	// Decrypt index data
	DecryptData(IndexData, IndexSize);
	
	uint32_t MountPointLength;
	char MountPoint[1024];
	int NumOfEntry;
	uint32_t FilenameSize;
	char Filename[1024];
	uint8_t FileHash[20];
	uint64_t FileOffset = 0;
	uint64_t FileSize = 0;
	int CompressionMethod = 0;
	uint64_t Zsize = 0;
	char Dummy[21];
	int NumOfBlocks;
	uint8_t Encrypted;
	uint32_t CompressedBlockSize = 0;
	
	read_data(&MountPointLength, IndexData, 4);
	read_data(MountPoint, IndexData, MountPointLength);
	
	printf("Mount Point: %s\n", MountPoint);
	
	read_data(&NumOfEntry, IndexData, 4);
	
	for (uint32_t Files = 0; Files < NumOfEntry; Files++) {
		read_data(&FilenameSize, IndexData, 4);
		read_data(Filename, IndexData, FilenameSize);
		read_data(FileHash, IndexData, 20);
		read_data(&FileOffset, IndexData, 8);
		read_data(&FileSize, IndexData, 8);
		read_data(&CompressionMethod, IndexData, 4);
		read_data(&Zsize, IndexData, 8);
		read_data(Dummy, IndexData, 21);
		
		if (CompressionMethod != 0) {
			read_data(&NumOfBlocks, IndexData, 4);
			
			for (int x = 0; x < NumOfBlocks; x++) {
				read_data(&Block[x].CompressedStart, IndexData, 8);
				read_data(&Block[x].CompressedEnd, IndexData, 8);
			}
		}
		
		read_data(&CompressedBlockSize, IndexData, 4);
		read_data(&Encrypted, IndexData, 1);
		
		int OutFile = CreateFile(Filename);
		
		if (OutFile == -1) {
			printf("Failed to open output file: %s\n", Filename);
			return 1;
		}
		
		if (CompressionMethod == 1) {
			// Zlib compression
			for (uint32_t x = 0; x < NumOfBlocks; x++) {
				if (lseek(PakFile, Block[x].CompressedStart, SEEK_SET) == -1) {
					printf("Failed to seek \n");
					continue;
				}
				
				uint8_t *CompressedData = (uint8_t*)malloc(Block[x].CompressedEnd - Block[x].CompressedStart);
				uint8_t *DecompressedData = (uint8_t*)malloc(FileSize);
				
				if (!CompressedData && !DecompressedData) {
					printf("Memory allocation failed\n");
					close(OutFile);
					continue;
				}
				
				if (read(PakFile, CompressedData, Block[x].CompressedEnd - Block[x].CompressedStart) != Block[x].CompressedEnd - Block[x].CompressedStart) {
					printf("Unable to read compressed block data\n");
					continue;
				}
				
				if (Encrypted == true) {
					DecryptData(CompressedData, Block[x].CompressedEnd - Block[x].CompressedStart);
				}
				
				int32_t result = DecompressBlock(CompressedData, Block[x].CompressedEnd - Block[x].CompressedStart, DecompressedData, FileSize);
				
				if (result == -1) {
					printf("Failed to decompress zlib block\n");
					continue;
				}
				
				if (write(OutFile, DecompressedData, result) != result) {
					printf("Failed to write data into %s\n", Filename);
					continue;
				}
				
				free(CompressedData);
				free(DecompressedData);
				
				printf("file: %s\n", Filename);
			}
		} else if (CompressionMethod == 0) {
			// No compression
			uint8_t buf[BUFFER_SIZE];
			uint64_t remaining = FileSize;
			ssize_t bytesRead, bytesWritten;
			
			while (remaining > 0) {
				size_t bytesToRead = remaining < BUFFER_SIZE ? remaining : BUFFER_SIZE;
				bytesRead = read(PakFile, buf, bytesToRead);
				
				if (bytesRead < 0) {
					printf("Error reading from file\n");
				}
				
				bytesWritten = write(OutFile, buf, bytesRead);
				
				if (bytesWritten != bytesRead) {
					printf("Error writing to output file\n");
				}
				remaining -= bytesRead;
			}
			printf("file: %s\n", Filename);
		} else {
			// Using another compression method like Zstd, Oodles, lz4
			printf("Unsupported the compression method\n");
			continue;
		}
	}
	
	printf("Number of files: %d\n", NumOfEntry);
	
	// closing pak file
	close(PakFile);
	return 0;
}


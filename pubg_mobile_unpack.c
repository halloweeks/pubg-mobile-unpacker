/*
 *  Author: Halloweeks
 *  Date: 2023-11-03
 *  Description: Extract pubg mobile pak contents and 
 *  the program not support extract pak after
 *  1.1.0 update of pubg mobile because change
 *  encryption algorithms and I don't know the algorithm and key
 *  GitHub: https://github.com/halloweeks/pubg-mobile-pak-extract
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <zlib.h>
#include <stdbool.h>

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

// Create output files
int create_file(const char *fullPath) {
	const char *lastSlash = strrchr(fullPath, '/');
	
	if (lastSlash != NULL) {
		size_t length = (size_t)(lastSlash - fullPath);
		char path[length + 1];
		strncpy(path, fullPath, length);
		path[length] = '\0';
		
		char *token = strtok(path, "/");
		char currentPath[1024] = "";
		
		while (token != NULL) {
			strcat(currentPath, token);
			strcat(currentPath, "/");
			mkdir(currentPath, 0777);
			token = strtok(NULL, "/");
		}
	}
	return open(fullPath, O_WRONLY | O_CREAT | O_TRUNC, 0644); 
}

// Decompress Zlib Block
int32_t zlib_decompress(uint8_t *CompressedData, uint32_t CompressedSize, uint8_t *DecompressedData, uint32_t DecompressedSize) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    strm.avail_in = CompressedSize;
    strm.next_in = CompressedData;

    strm.avail_out = DecompressedSize;
    strm.next_out = DecompressedData;

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
	clock_t start, end;
    double cpu_time_used;

    start = clock();
    
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
	
	// Get the index data size
	uint32_t IndexSize = lseek(PakFile, -info.offset, SEEK_END);
	
	// Allocate memory for pak index data
	uint8_t *IndexData = (uint8_t*)malloc(IndexSize);
	
	// if memory allocation failed
	if (!IndexData) {
		printf("Memory allocation failed.\n");
		close(PakFile);
		return 3;
	}
	
	// set file pointer into index data
	if (lseek(PakFile, info.offset, SEEK_SET) == -1) {
		perror("Error seeking in file.");
        close(PakFile);
        return 1;
	}
	
	// load pak index data
	if (read(PakFile, IndexData, IndexSize) != IndexSize) {
		printf("Unable to load index data\n");
		return 4;
	}
	
	// Decrypt index data
	DecryptData(IndexData, IndexSize);
	
	uint32_t MountPointLength;
	char MountPoint[1024];
	int NumOfEntry;
	int32_t FilenameSize;
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
	
	printf("offset\t\tfilesize\t\tfilename\n");
	printf("--------------------------------------\n");
	
	read_data(&NumOfEntry, IndexData, 4);
	
	for (uint32_t Files = 0; Files < NumOfEntry; Files++) {
		read_data(&FilenameSize, IndexData, 4);
		
		if (FilenameSize > 0 && FilenameSize < 1024) {
			read_data(Filename, IndexData, FilenameSize);
		} else {
			FilenameSize = -FilenameSize;
			// Unicode or filename contains special characters
			read_data(Filename, IndexData, FilenameSize * 2);
			Filename[FilenameSize * 2] = '\0';
		}
		
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
		
		// create output files
		int OutFile = create_file(Filename);
		
		// Failed
		if (OutFile == -1) {
			printf("Failed to open output file: %s\n", Filename);
			continue;
		}
		
		// zlib compression
		if (CompressionMethod == 1) {
			for (uint32_t x = 0; x < NumOfBlocks; x++) {
				if (lseek(PakFile, Block[x].CompressedStart, SEEK_SET) == -1) {
					printf("Failed to seek \n");
					continue;
				}
				
				// allocating memory
				uint8_t *CompressedData = (uint8_t*)malloc(Block[x].CompressedEnd - Block[x].CompressedStart);
				uint8_t *DecompressedData = (uint8_t*)malloc(FileSize);
				
				// when memory allocation failed
				if (!CompressedData && !DecompressedData) {
					printf("Memory allocation failed\n");
					close(OutFile);
					continue;
				}
				
				// Read compressed block
				if (read(PakFile, CompressedData, Block[x].CompressedEnd - Block[x].CompressedStart) != Block[x].CompressedEnd - Block[x].CompressedStart) {
					printf("Unable to read compressed block data\n");
					continue;
				}
				
				// if the compressed zlib block is encrypted
				if (Encrypted == true) {
					// Decrypt the encrypted zlib block
					DecryptData(CompressedData, Block[x].CompressedEnd - Block[x].CompressedStart);
				}
				
				// decompress zlib block
				int32_t result = zlib_decompress(CompressedData, Block[x].CompressedEnd - Block[x].CompressedStart, DecompressedData, FileSize);
				
				// if decompression was failed
				if (result == -1) {
					printf("Failed to decompress zlib block\n");
					continue;
				}
				
				// write uncompress data
				if (write(OutFile, DecompressedData, result) != result) {
					printf("Failed to write data into %s\n", Filename);
					continue;
				}
				
				// release allocated memory
				free(CompressedData);
				free(DecompressedData);
				
				// print info
				printf("%016lx %lu\t%s\n", Block[x].CompressedStart, (Block[x].CompressedEnd - Block[x].CompressedStart), Filename);
			}
		} else if (CompressionMethod == 0) {
			// No compression just Extract data chunk by chunk
			uint8_t buf[BUFFER_SIZE];
			uint64_t remaining = FileSize;
			ssize_t bytesRead, bytesWritten;
			
			while (remaining > 0) {
				size_t bytesToRead = remaining < BUFFER_SIZE ? remaining : BUFFER_SIZE;
				bytesRead = read(PakFile, buf, bytesToRead);
				
				if (bytesRead < 0) {
					printf("Error reading from file\n");
				}
				
				if (Encrypted == true) {
					DecryptData(buf, bytesToRead);
				}
				
				bytesWritten = write(OutFile, buf, bytesRead);
				
				if (bytesWritten != bytesRead) {
					printf("Error writing to output file\n");
				}
				remaining -= bytesRead;
			}
			printf("%016lx %lu\t%s\n", FileOffset, FileSize, Filename);
		} else {
			// data using another compression method like Zstd, Oodles, etc..
			printf("Zlib decompression support only\n");
			continue;
		}
		
		close(OutFile);
	}
	
	// closing pak file
	close(PakFile);
	
	end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    
	// print number of files in the pak files
	printf("%u files found in %f seconds\n", NumOfEntry, cpu_time_used);
	return 0;
}


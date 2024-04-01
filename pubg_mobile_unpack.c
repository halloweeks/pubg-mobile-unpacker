/*
 *  Author: Halloweeks.
 *  Date: 2023-11-03.
 *  Description: Extract pubg mobile pak contents. 
 *  The program not support pubg mobile 1.1.0 and above version 
 *  pak extraction because pubg mobile developer using actual
 *  encryption for encrypting pak index data it make unable to read data without decryption key
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

// The key use for deobfuscation of index offset
#define OFFSET_KEY 0xA6D17AB4D4783A41

#define CHUNK_SIZE 65536

// Pak Header
typedef struct {
	uint8_t encrypted;
	uint32_t magic;
	uint32_t version;
	uint8_t hash[20];
	uint64_t size;
	uint64_t offset;
} __attribute__((packed)) PakInfo;

// Compression Blocks
typedef struct {
    uint64_t CompressedStart;
    uint64_t CompressedEnd;
} __attribute__((packed)) CompressionBlock;

// Function to decrypt data
void DecryptData(uint8_t *data, uint32_t size) {
	for (uint32_t index = 0; index < size; index++) {
		data[index] ^= 0x79;
	}
}

// Function to create a file
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

// Function to decompress using zlib
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

// Global variable to track current index offset during reading
uint64_t current_index_offset = 0;

// Function to read data from the index
void read_data(void *destination, const uint8_t *source, size_t length) {
	memcpy(destination, source + current_index_offset, length);
	current_index_offset += length;
}

int main(int argc, const char *argv[]) {
	clock_t start, end;
        double cpu_time_used;
        start = clock();
	PakInfo info;
	CompressionBlock Block[100];
	
	int PakFile = open(argv[1], O_RDONLY);
	
	if (PakFile == -1) {
		printf("Unable to open %s file\n", argv[1]);
		return 1;
	}
	
	if (lseek(PakFile, -45, SEEK_END) == -1) {
		printf("failed to seek file position\n");
		return 1;
	}
	
	if (read(PakFile, &info, 45) != 45) {
		printf("Failed to read pak header at -44\n");
		return 1;

	}
	
	info.offset = info.offset ^ OFFSET_KEY;
	// info.size = info.size ^ SIZE_KEY;

	uint32_t IndexSize = lseek(PakFile, -info.offset, SEEK_END);
	
	uint8_t *IndexData = (uint8_t*)malloc(IndexSize);
	
	if (!IndexData) {
		printf("Memory allocation failed.\n");
		close(PakFile);
		return 3;
	}
	
	if (lseek(PakFile, info.offset, SEEK_SET) == -1) {
		perror("Error seeking in file.");
                close(PakFile);
                return 1;
	}
	
	if (read(PakFile, IndexData, IndexSize) != IndexSize) {
		printf("Unable to load index data\n");
		return 4;
	}

	// Decrypt if necessary
	if (info.encrypted ^ 0x01) {
		DecryptData(IndexData, IndexSize);
	}
	
	uint32_t MountPointLength;
	char MountPoint[1024];
	int NumOfEntry;
	int32_t FilenameSize;
	char Filename[1024];
	uint8_t FileHash[20];
	uint64_t FileOffset = 0;
	uint64_t FileSize = 0;
	int CompressionMethod = 0;
	uint64_t CompressedLength = 0;
	size_t DecompressLength = 0;
	uint8_t Dummy[21];
	int NumOfBlocks = 0;
	uint64_t CompressedStart = 0;
	uint64_t CompressedEnd = 0;
	uint8_t Encrypted;
	uint32_t CompressedBlockSize = 0;
	
	// hold compressed and uncompressed data
	uint8_t CompressedData[CHUNK_SIZE];
	uint8_t DecompressedData[CHUNK_SIZE];
	
	printf("offset\t\tfilesize\t\tfilename\n");
	printf("--------------------------------------\n");
	
	read_data(&MountPointLength, IndexData, 4);
	read_data(MountPoint, IndexData, MountPointLength);
	read_data(&NumOfEntry, IndexData, 4);
	
	for (uint32_t Files = 0; Files < NumOfEntry; Files++) {
		read_data(&FilenameSize, IndexData, 4);
		if (FilenameSize > 0 && FilenameSize < 1024) {
			read_data(Filename, IndexData, FilenameSize);
		} else {
			// Unicode or filename contains special characters maybe encounter with bug
			FilenameSize = -FilenameSize;
			read_data(Filename, IndexData, FilenameSize * 2);
			Filename[FilenameSize * 2] = '\0';
		}
		
		read_data(FileHash, IndexData, 20);
		read_data(&FileOffset, IndexData, 8);
		read_data(&FileSize, IndexData, 8);
		read_data(&CompressionMethod, IndexData, 4);
		read_data(&CompressedLength, IndexData, 8);
		read_data(Dummy, IndexData, 21);
		
		if (CompressionMethod != 0) {
			read_data(&NumOfBlocks, IndexData, 4);
			for (int x = 0; x < NumOfBlocks; x++) {
				read_data(&Block[x].CompressedStart, IndexData, 8);
				read_data(&Block[x].CompressedEnd, IndexData, 8);
			}
		} else {
			NumOfBlocks = 0;
		}
		
		read_data(&CompressedBlockSize, IndexData, 4);
		read_data(&Encrypted, IndexData, 1);
		
		// Open output file
		int OutFile = create_file(Filename);
		
		// if opening output file failed
		if (OutFile == -1) {
			fprintf(stderr, "Failed to open output file: %s\n", Filename);
			continue; // continue for ignore this file extract next
		}
		
		// Data is compressed
		if (NumOfBlocks > 0) {
			for (int x = 0; x < NumOfBlocks; x++) {
				lseek(PakFile, Block[x].CompressedStart, SEEK_SET);
				read(PakFile, CompressedData, Block[x].CompressedEnd - Block[x].CompressedStart);
				
				if (Encrypted) {
					DecryptData(CompressedData, Block[x].CompressedEnd - Block[x].CompressedStart);
				}
				
				// zlib compression
				if (CompressionMethod == 1) {
					DecompressLength = zlib_decompress(CompressedData, Block[x].CompressedEnd - Block[x].CompressedStart, DecompressedData, CHUNK_SIZE);
					
					if (DecompressLength == -1) {
						fprintf(stderr, "Zlib decompression failed\n");
						continue;
					}
				}
				
				write(OutFile, DecompressedData, DecompressLength);
				printf("%016lx %lu\t%s\n", Block[x].CompressedStart, DecompressLength, Filename);
			}
		} else {
			// Uncompressed data
			lseek(PakFile, FileOffset + 74, SEEK_SET);
			uint64_t remaining = FileSize;
			ssize_t bytesRead, bytesWritten;
			
			while (remaining > 0) {
				size_t bytesToRead = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;
				bytesRead = read(PakFile, DecompressedData, bytesToRead);
				if (bytesRead < 0) {
					printf("Error reading from file\n");
				}
				if (Encrypted) {
					DecryptData(DecompressedData, bytesToRead);
				}
				bytesWritten = write(OutFile, DecompressedData, bytesRead);
				if (bytesWritten != bytesRead) {
					printf("Error writing to output file\n");
				}
				remaining -= bytesRead;
				printf("%016lx %zd\t%s\n", FileOffset + 74, bytesWritten, Filename);
			}
		}
		
		close(OutFile);
	}
	
	free(IndexData);
	close(PakFile);
	end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("\n%u files found in %f seconds\n", NumOfEntry, cpu_time_used);
	return 0;
}

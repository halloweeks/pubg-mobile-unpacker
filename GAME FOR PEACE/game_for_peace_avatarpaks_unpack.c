/*
 *  Author: Halloweeks.
 *  Date: 2024-05-30.
 *  Description: Extract game for peace pak contents. 
 *  GitHub: https://github.com/halloweeks/pubg-mobile-unpacker
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
#include <zstd.h>
#include "sha1.h"

// The key use for deobfuscation
#define OFFSET_KEY 0xD74AF37FAA6B020D
#define SIZE_KEY 0x8924B0E3298B7069

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
		data[index] ^= 0x79u;
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
	for (size_t x = 0; x < length; x++) {
		((uint8_t*)destination)[x] = source[current_index_offset + x];
	}
	// memcpy(destination, source + current_index_offset, length);
	current_index_offset += length;
}

int unicode_to_utf8(const char *input, size_t input_len, char *out, size_t output_len) {
	char output[output_len];
	
    size_t i = 0, j = 0;
    
    while (i < input_len && j < output_len) {
        unsigned int unicode_char;
        
        // Read a UTF-16LE character
        unicode_char = (unsigned char)input[i++];
        unicode_char |= (unsigned char)input[i++] << 8;
        
        // Convert UTF-16LE to UTF-8
        if (unicode_char <= 0x7F) {
            output[j++] = (char)unicode_char;
        } else if (unicode_char <= 0x7FF) {
            if (j + 1 >= output_len) return -1; // Output buffer too small
            output[j++] = 0xC0 | (unicode_char >> 6);
            output[j++] = 0x80 | (unicode_char & 0x3F);
        } else {
            if (j + 2 >= output_len) return -1; // Output buffer too small
            output[j++] = 0xE0 | (unicode_char >> 12);
            output[j++] = 0x80 | ((unicode_char >> 6) & 0x3F);
            output[j++] = 0x80 | (unicode_char & 0x3F);
        }
    }
    
    if (i < input_len) return -1; // Input buffer not fully consumed
    
    memcpy(out, output, j);
    return 0;
}

// void extract(int PakFile, Entry entry, char *filename);

int main(int argc, const char *argv[]) {
	clock_t start = clock();
	PakInfo info;
	
	uint8_t hash_key[20] = {
		0x9B, 0x31, 0x24, 0x61, 0xCB, 0xD3, 0xF5, 0x18, 0x20, 0xA1,
		0x1B, 0xFB, 0xFD, 0x40, 0xB6, 0x00, 0x1E, 0x53, 0x5C, 0x24
	};
	
	if (argc != 2) {
		fprintf(stderr, "Usage %s <pak_file>\n", argv[0]);
		return 1;
	}

	if (access(argv[1], F_OK) == -1) {
		fprintf(stderr, "Input %s file does not exist.\n", argv[1]);
		return 1;
	}
	
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
		printf("Failed to read pak header at -45\n");
		return 1;

	}
	
	if (info.version != 7) {
		printf("This pak version unsupported!?\n");
		close(PakFile);
        return 1;
	}
	
	// deobfuscation
	info.encrypted ^= 0x6C;
	for (uint8_t x = 0; x < 20; x++) {
		info.hash[x] ^= hash_key[x];
	}
	info.offset ^= OFFSET_KEY;
	info.size ^= SIZE_KEY;
	
	uint8_t *IndexData = (uint8_t*)malloc(info.size);
	
	if (!IndexData) {
		printf("Memory allocation failed.\n");
		close(PakFile);
		return 1;
	}

	if (pread(PakFile, IndexData, info.size, info.offset) != info.size) {
		fprintf(stderr, "Failed to load index data\n");
		return 1;
	}
	
	// Decrypt if necessary
	if (info.encrypted) {
		DecryptData(IndexData, info.size);
	}
	
	uint8_t hash[20];
	SHA1(IndexData, info.size, hash);
	
	if (memcmp(hash, info.hash, 20) != 0) {
		printf("pak index corrupted, mismatch (SHA1 HASH)\n");
		return 1;
	}
	
	uint32_t MountPointLength;
	char MountPoint[1024];
	int NumOfEntry;
	int32_t FilenameLength;
	char Filename[8192];
	uint8_t FileHash[20];
	uint64_t FileOffset;
	uint64_t FileSize;
	uint32_t CompressionMethod;
	uint64_t CompressedLength;
	uint8_t Dummy[21];
	uint32_t NumOfBlocks;
	CompressionBlock Block[500];
	uint32_t CompressedBlockSize;
	uint8_t Encrypted;
	
	int DecompressLength;
	
	char fullpath[8192];
	
	uint8_t CompressedData[CHUNK_SIZE];
	uint8_t DecompressedData[CHUNK_SIZE];
	
	printf("offset\t\tfilesize\t\tfilename\n");
	printf("--------------------------------------\n");
	
	read_data(&MountPointLength, IndexData, 4);
	read_data(MountPoint, IndexData, MountPointLength);
	
	for (int x = 0; x < MountPointLength - 9; x++) {
		MountPoint[x] = MountPoint[x + 9];
	}
	
	read_data(&NumOfEntry, IndexData, 4);
	
	for (uint32_t index = 0; index < NumOfEntry; index++) {
		read_data(&FilenameLength, IndexData, 4);
		
		if (FilenameLength > 8192) {
			printf("Filename too long!\n");
			exit(1);
		}
		
		if (FilenameLength < 0) {
			read_data(Filename, IndexData, -FilenameLength * 2);
			if (unicode_to_utf8(Filename, -FilenameLength * 2, Filename, sizeof(Filename)) == -1) {
				printf("failed to convert UTF-16LE filename into UTF-8!\n");
				exit(1);
			}
		} else {
			read_data(Filename, IndexData, FilenameLength);
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
		
		snprintf(fullpath, 4096, "%s%s", MountPoint, Filename);
		
		int OutFile = create_file(fullpath);
		
		if (OutFile == -1) {
			fprintf(stderr, "Failed to open output file: %s\n", fullpath);
			continue; // continue for ignore this file extract next
		}
		
		if (NumOfBlocks > 0) {
			for (int x = 0; x < NumOfBlocks; x++) {
				if (pread(PakFile, CompressedData, Block[x].CompressedEnd - Block[x].CompressedStart, Block[x].CompressedStart) != Block[x].CompressedEnd - Block[x].CompressedStart) {
					fprintf(stderr, "Failed to read compressed chunk at %lx\n", Block[x].CompressedStart);
					printf("chunk size: %lu\n", Block[x].CompressedEnd - Block[x].CompressedStart);
					perror("error");
					exit(1);
				}
				
				if (Encrypted) {
					DecryptData(CompressedData, Block[x].CompressedEnd - Block[x].CompressedStart);
				}
				
				// zlib compression
				if (CompressionMethod == 1) {
					if ((DecompressLength = zlib_decompress(CompressedData, Block[x].CompressedEnd - Block[x].CompressedStart, DecompressedData, CHUNK_SIZE)) == -1) {
						fprintf(stderr, "Zlib decompression failed\n");
						continue;
					}
				} else {
					printf("Unknown compression method %u\n", CompressionMethod);
					close(OutFile);
				}
				
				write(OutFile, DecompressedData, DecompressLength);
				printf("%016lx %d\t%s\n", Block[x].CompressedStart, DecompressLength, fullpath);
			}
		} else {
			// Uncompressed data
			lseek(PakFile, FileOffset + 74, SEEK_SET);
			
			ssize_t bytesRead, bytesWritten;
			
			while (FileSize > 0) {
				size_t bytesToRead = FileSize < CHUNK_SIZE ? FileSize : CHUNK_SIZE;
				
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
				
				FileSize -= bytesRead;
				printf("%016lx %zd\t%s\n", FileOffset + 74, bytesWritten, fullpath);
			}
		}
		
		close(OutFile);
		memset(fullpath, 0, 4096);
	}
	
	free(IndexData);
	
	close(PakFile);
	clock_t end = clock();
	double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("\n%u files found in %f seconds\n", NumOfEntry, cpu_time_used);
	return 0;
}
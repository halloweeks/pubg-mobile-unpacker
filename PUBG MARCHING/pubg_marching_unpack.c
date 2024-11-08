/*
 *  This project are not completed 
 *  Author: Halloweeks.
 *  Date: 2024-10-28.
 *  Description: Extract pubg marching pak contents. 
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
#include <errno.h>
#include "AES_256_ECB.h"
#include "sha1.h"

#define CHUNK_SIZE 65536

// Pak Header
typedef struct {
	uint8_t encrypted;
	uint32_t magic;
	uint32_t version;
	uint64_t offset;
	uint64_t size;
	uint8_t hash[20];
} __attribute__((packed)) PakInfo;

// Compression Blocks
typedef struct {
    uint64_t CompressedStart;
    uint64_t CompressedEnd;
} __attribute__((packed)) CompressionBlock;


// Function to decrypt data
void DecryptData(uint8_t *data, uint32_t size, const uint8_t *key) {
	AES_CTX ctx;
	AES_DecryptInit(&ctx, key);
	
	for (uint32_t offset = 0; offset < size; offset += AES_BLOCK_SIZE) {
		AES_Decrypt(&ctx, data + offset, data + offset);
	}
	
	AES_CTX_Free(&ctx);
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

unsigned int ZLIB_decompress(unsigned char *InData, unsigned int InSize, unsigned char *OutData, unsigned int OutSize) {
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.next_in = InData;
	strm.avail_in = InSize;
	strm.next_out = OutData;
	strm.avail_out = OutSize;
	
	if (inflateInit(&strm) != Z_OK) {
		fprintf(stderr, "Failed to initialize zlib.\n");
		return 0;
	}
	
	if (inflate(&strm, Z_FINISH) != Z_STREAM_END) {
		fprintf(stderr, "inflate failed: %s\n", strm.msg);
		inflateEnd(&strm);
		return 0;
	}
	
	if (inflateEnd(&strm) != Z_OK) {
		fprintf(stderr, "inflateEnd failed: %s\n", strm.msg);
		return 0;
	}
	
	return strm.total_out;
}

// Global variable to track current index offset during reading
uint64_t current_index_offset = 0;

// Function to read data from the index
void read_data(void *destination, const uint8_t *source, size_t length) {
	memcpy(destination, source + current_index_offset, length);
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


int main(int argc, const char *argv[]) {
	PakInfo info;
	
	// Decryption key
	uint8_t key[32] = {
		0x70, 0x6E, 0x76, 0x68, 0x7D, 0x58, 0x50, 0x47,
		0x21, 0x2A, 0x76, 0x36, 0x4C, 0x29, 0x40, 0x66,
		0x62, 0x51, 0x72, 0x48, 0x28, 0x24, 0x4C, 0x2D,
		0x37, 0x67, 0x42, 0x77, 0x32, 0x67, 0x34, 0x6C
	};
	
	if (argc != 2) {
		fprintf(stderr, "Usage %s <pak_file>\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	if (access(argv[1], F_OK) == -1) {
		fprintf(stderr, "Input %s file does not exist.\n", argv[1]);
		return EXIT_FAILURE;
	}
	
	int PakFile = open(argv[1], O_RDONLY);
	
	if (PakFile == -1) {
		fprintf(stderr, "Unable to open %s file\n", argv[1]);
		return EXIT_FAILURE;
	}
	
	if (lseek(PakFile, -45, SEEK_END) == -1) {
		fprintf(stderr, "failed to seek file position\n");
		return EXIT_FAILURE;
	}
	
	if (read(PakFile, &info, 45) != 45) {
		fprintf(stderr, "Failed to read pak header at -45\n");
		return EXIT_FAILURE;

	}
	
	if (info.magic != 0x5A6F12E1) {
		fprintf(stderr, "Incorrect pak magic!\n");
		return EXIT_FAILURE;
	}
	
	if (info.version != 4) {
		fprintf(stderr, "This pak version unsupported!\n");
		close(PakFile);
        return EXIT_FAILURE;
	}
	
	uint8_t *IndexData = (uint8_t*)malloc(info.size);
	
	if (!IndexData) {
		fprintf(stderr, "Memory allocation failed.\n");
		close(PakFile);
		return EXIT_FAILURE;
	}

	if (pread(PakFile, IndexData, info.size, info.offset) != info.size) {
		fprintf(stderr, "Failed to load index data\n");
		return EXIT_FAILURE;
	}
	
	// Decrypt if necessary
	if (info.encrypted) {
		DecryptData(IndexData, info.size, key);
	}
	
	uint8_t hash[20];
	SHA1(IndexData, info.size, hash);
	
	if (memcmp(hash, info.hash, 20) != 0) {
		fprintf(stderr, "pak index corrupted, mismatch (SHA1 HASH)\n");
		return EXIT_FAILURE;
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
	uint64_t UncompressedSize;
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
			fprintf(stderr, "Filename too long!\n");
			free(IndexData);
			close(PakFile);
			exit(EXIT_FAILURE);
		}
		
		if (FilenameLength < 0) {
			read_data(Filename, IndexData, -FilenameLength * 2);
			if (unicode_to_utf8(Filename, -FilenameLength * 2, Filename, sizeof(Filename)) == -1) {
				fprintf(stderr, "failed to convert UTF-16LE filename into UTF-8!\n");
				free(IndexData);
				close(PakFile);
				exit(EXIT_FAILURE);
			}
		} else {
			read_data(Filename, IndexData, FilenameLength);
		}
		
		read_data(&FileOffset, IndexData, 8);
		read_data(&FileSize, IndexData, 8);
		read_data(&UncompressedSize, IndexData, 8);
		read_data(&CompressionMethod, IndexData, 4);
		read_data(FileHash, IndexData, 20);
		
		if (CompressionMethod != 0) {
			read_data(&NumOfBlocks, IndexData, 4);
			
			for (int x = 0; x < NumOfBlocks; x++) {
				read_data(&Block[x].CompressedStart, IndexData, 8);
				read_data(&Block[x].CompressedEnd, IndexData, 8);
			}
		} else {
			NumOfBlocks = 0;
		}
		
		read_data(&Encrypted, IndexData, 1);
		read_data(&CompressedBlockSize, IndexData, 4);
		
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
					fprintf(stderr, "data decryption not supported!\n");
					// DecryptData(CompressedData, Block[x].CompressedEnd - Block[x].CompressedStart);
				}
				
				// zlib compression
				if (CompressionMethod == 1) {
					if ((DecompressLength = ZLIB_decompress(CompressedData, Block[x].CompressedEnd - Block[x].CompressedStart, DecompressedData, CHUNK_SIZE)) == -1) {
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
			lseek(PakFile, FileOffset, SEEK_SET);
			
			ssize_t bytesRead, bytesWritten;
			
			while (FileSize > 0) {
				size_t bytesToRead = FileSize < CHUNK_SIZE ? FileSize : CHUNK_SIZE;
				
				bytesRead = read(PakFile, DecompressedData, bytesToRead);
				
				if (bytesRead < 0) {
					printf("Error reading from file\n");
				}
				
				if (Encrypted) {
					fprintf(stderr, "data decryption not supported!\n");
					
					// DecryptData(DecompressedData, bytesToRead);
				}
				
				bytesWritten = write(OutFile, DecompressedData, bytesRead);
				
				if (bytesWritten != bytesRead) {
					printf("Error writing to output file\n");
				}
				
				FileSize -= bytesRead;
				printf("%016lx %zd\t%s\n", FileOffset + 74, bytesWritten, fullpath);
			}
		}
		
		
		printf("file: %s\n", fullpath);
		
		close(OutFile);
		memset(fullpath, 0, 4096);
		
	}
    
    free(IndexData);
	close(PakFile);
	return EXIT_SUCCESS;
}

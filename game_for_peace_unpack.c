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
#include <errno.h>

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
    uint64_t start;
    uint64_t end;
} __attribute__((packed)) CompressionBlock;


typedef struct {
	uint8_t FileHash[20];
	uint64_t FileOffset;
	uint64_t FileSize;
	uint32_t CompressionMethod;
	uint64_t CompressedLength;
	uint8_t Dummy[21];
	uint32_t NumOfBlocks;
	CompressionBlock *blocks;
	uint32_t CompressedBlockSize;
	uint8_t Encrypted;
} __attribute__((packed)) Entry;


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

void extract(int PakFile, Entry entry, char *filename);

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
	clock_t t0 = clock();
	
	PakInfo info;
	CompressionBlock Block[500];
	
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
	
	off_t total = lseek(PakFile, 0, SEEK_END);
	lseek(PakFile, 0, SEEK_SET);
	
	if (lseek(PakFile, -45, SEEK_END) == -1) {
		printf("failed to seek file position\n");
		return 1;
	}
	
	if (read(PakFile, &info, 45) != 45) {
		printf("Failed to read pak header at -45\n");
		return 1;

	}
	
	/*
	if (info.version != 8) {
		printf("This pak version unsupported!\n");
		close(PakFile);
        return 1;
	}
	*/
	
	// deobfuscation
	info.offset ^= OFFSET_KEY;
	info.encrypted ^= 0x6C;
	
	int64_t size = lseek(PakFile, -info.offset, SEEK_END); 
	size -= 45;
	
	// check if index data size is greater than 50MB
	if (size > 52428800) {
		fprintf(stderr, "Index data size is not compatible.\n");
		close(PakFile);
		return 1;
	}
	
	uint8_t *IndexData = (uint8_t*)malloc(size);
	
	if (!IndexData) {
		printf("Memory allocation failed.\n");
		close(PakFile);
		return 1;
	}

	if (pread(PakFile, IndexData, size, info.offset) != size) {
		fprintf(stderr, "Failed to load index data\n");
		return 1;
	}
	
	// Decrypt if necessary
	if (info.encrypted) {
		DecryptData(IndexData, size);
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
	uint8_t Encrypted;
	uint32_t CompressedBlockSize = 0;
	
	
	printf("offset\t\tfilesize\t\tfilename\n");
	printf("--------------------------------------\n");
	
	read_data(&MountPointLength, IndexData, 4);
	read_data(MountPoint, IndexData, MountPointLength);
	
	for (int x = 0; x < MountPointLength - 9; x++) {
		MountPoint[x] = MountPoint[x + 9];
	}
	read_data(&NumOfEntry, IndexData, 4);
	
	Entry *entry = (Entry*)malloc(NumOfEntry * sizeof(Entry));
	
	if (!entry) {
		fprintf(stderr, "Memory allocation failed!\n");
		free(IndexData);
		close(PakFile);
		return 1;
	}
	
	for (uint32_t Files = 0; Files < NumOfEntry; Files++) {
		read_data(entry[Files].FileHash, IndexData, 20);
		read_data(&entry[Files].FileOffset, IndexData, 8);
		read_data(&entry[Files].FileSize, IndexData, 8);
		read_data(&entry[Files].CompressionMethod, IndexData, 4);
		read_data(&entry[Files].CompressedLength, IndexData, 8);
		read_data(entry[Files].Dummy, IndexData, 21);
		
		// Check if CompressionMethod is not equal to 0 (no compression)
		if (entry[Files].CompressionMethod != 0) {
			read_data(&entry[Files].NumOfBlocks, IndexData, 4);
			
			entry[Files].blocks = (CompressionBlock*)malloc(entry[Files].NumOfBlocks * sizeof(CompressionBlock));
			
			if (!entry[Files].blocks) {
				fprintf(stderr, "Memory allocation for blocks failed\n");
				return 0;
			}
			
			for (uint32_t i = 0; i < entry[Files].NumOfBlocks; i++) {
				read_data(&entry[Files].blocks[i].start, IndexData, 8);
				read_data(&entry[Files].blocks[i].end, IndexData, 8);
			}
		} else {
			entry[Files].NumOfBlocks = 0;
		}
		
		read_data(&entry[Files].CompressedBlockSize, IndexData, 4);
		read_data(&entry[Files].Encrypted, IndexData, 1);
	}
	
	uint64_t ENTRIES = 0;
	uint64_t DIR_COUNT = 0;
	
	read_data(&ENTRIES, IndexData, 8);
	read_data(&DIR_COUNT, IndexData, 8);
	
	int32_t DIR_LEN = 0;
	char DIR_NAME[1024];
	uint64_t DIR_FILES = 0;
	int32_t ENTRY;
	
	char path[1024];
	
	for (int files = 0; files < DIR_COUNT; files++) {
		read_data(&DIR_LEN, IndexData, 4);
		read_data(DIR_NAME, IndexData, DIR_LEN);
		read_data(&DIR_FILES, IndexData, 8);
		
		for (int x = 0; x < DIR_FILES; x++) {
			read_data(&FilenameSize, IndexData, 4);
			
			if (FilenameSize > 0) {
				read_data(Filename, IndexData, FilenameSize);
			} else {
				// filename is unicode
				read_data(Filename, IndexData, -FilenameSize * 2);
				if (unicode_to_utf8(Filename, -FilenameSize * 2, Filename, sizeof(Filename)) == -1) {
					printf("failed to convert UTF-16LE filename into UTF-8!\n");
					exit(1);
				}
			}
			
			read_data(&ENTRY, IndexData, 4);
			
			memset(path, 0, 1024);
			snprintf(path, 1024, "%s%s%s", MountPoint, DIR_NAME, Filename);
			
			extract(PakFile, entry[ENTRY], path);
		}
	}
	
	
	// Free the allocated memory when done
    for (uint32_t Files = 0; Files < NumOfEntry; Files++) {
        free(entry[Files].blocks);
    }
    free(entry);
	
	free(IndexData);
	close(PakFile);
	
	clock_t t1 = clock();
    double time = ((double)(t1 - t0)) / CLOCKS_PER_SEC;
    const double MB = 1024.0 * 1024.0;
    printf("Processed %.2f MB, speed = %.2f MB/s, %u files found in %f seconds\n", total / MB, total / MB / time, NumOfEntry, time);
	return 0;
}

// hold compressed and uncompressed data
uint8_t CompressedData[CHUNK_SIZE * 2];
uint8_t DecompressedData[CHUNK_SIZE * 2];
size_t DecompressLength = 0;

void extract(int PakFile, Entry entry, char *filename) {
	int OutFile = create_file(filename);
	
	if (OutFile == -1) {
		printf("Failed to open output file: %s\n", filename);
		exit(1);
	}
	
	if (entry.NumOfBlocks > 0) {
		for (int x = 0; x < entry.NumOfBlocks; x++) {
			if (pread(PakFile, CompressedData, entry.blocks[x].end - entry.blocks[x].start, entry.blocks[x].start) != entry.blocks[x].end - entry.blocks[x].start) {
				printf("Failed to read compressed chunk at %lx\n", entry.blocks[x].start);
				exit(1);
			}
			
			if (entry.Encrypted) {
				DecryptData(CompressedData, entry.blocks[x].end - entry.blocks[x].start);
			}
			
			if (entry.CompressionMethod == 1) {
				if ((DecompressLength = ZLIB_decompress(CompressedData, entry.blocks[x].end - entry.blocks[x].start, DecompressedData, CHUNK_SIZE)) == 0) {
					fprintf(stderr, "ZLIB Decompression failed\n");
					exit(1);
				}
			} else {
				printf("Unknown compression method %u\n", entry.CompressionMethod);
				close(OutFile);
				exit(1);
			}
			
			write(OutFile, DecompressedData, DecompressLength);
			printf("%016lx %lu\t%s\n", entry.blocks[x].start, DecompressLength, filename);
		}
	} else {
		lseek(PakFile, entry.FileOffset + 74, SEEK_SET);
		
		ssize_t bytesRead, bytesWritten;
		
		while (entry.FileSize > 0) {
			size_t bytesToRead = entry.FileSize < CHUNK_SIZE ? entry.FileSize : CHUNK_SIZE;
			bytesRead = read(PakFile, DecompressedData, bytesToRead);
			
			if (bytesRead < 0) {
				printf("Error reading from file\n");
			}
			
			if (entry.Encrypted) {
				DecryptData(DecompressedData, bytesToRead);
			}
			
			bytesWritten = write(OutFile, DecompressedData, bytesRead);
			
			if (bytesWritten != bytesRead) {
				printf("Error writing to output file\n");
			}
			
			entry.FileSize -= bytesRead;
			printf("%016lx %zd\t%s\n", entry.FileOffset, bytesWritten, filename);
		}
	}
	
	close(OutFile);
	return;
}

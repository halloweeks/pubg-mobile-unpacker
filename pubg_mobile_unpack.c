/*
 *  Author: Halloweeks
 *  Date: 2023-10-30
 *  Description: Code to display the contents of PUBG Mobile pak files.
 *  GitHub: https://github.com/halloweeks
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

// The key use for deobfuscation of index offset
#define OFFSET_KEY 0xA6D17AB4D4783A41

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

uint32_t current_data_index = 0;

void read_data(void *destination, uint8_t *source, size_t length) {
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
	
	// Get pak file size
	off_t fsize = lseek(PakFile, 0, SEEK_END);
	
	// set file pointer into index data
	if (lseek(PakFile, info.offset ^ OFFSET_KEY, SEEK_SET) == -1) {
		perror("Error seeking in file");
        close(PakFile);
        return 1;
	}
	
	// Allocate memory for pak index data
	uint8_t *IndexData = (uint8_t*)malloc(fsize - (info.offset ^ OFFSET_KEY));
	
	// if allocation failed
	if (!IndexData) {
		printf("Failed to memory allocation\n");
		close(PakFile);
		return 3;
	}
	
	// read pak index data
	if (read(PakFile, IndexData, fsize - (info.offset ^ OFFSET_KEY)) != (fsize - (info.offset ^ OFFSET_KEY))) {
		printf("Unable to load index data\n");
		return 4;
	}
	
	// Decrypt index data
	DecryptData(IndexData, fsize - (info.offset ^ OFFSET_KEY));
	
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
	
	for (uint32_t FILES = 0; FILES < NumOfEntry; FILES++) {
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
		
		printf("File: %s\n", Filename);
	}
	
	printf("Number of files: %d\n", NumOfEntry);
	
	// closing pak file
	close(PakFile);
	return 0;
}
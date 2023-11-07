#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#define OFFSET_KEY 0xA6D17AB4D4783A41
#define SIZE_KEY 0x00

typedef struct {
	uint32_t magic;
	uint32_t version;
	uint8_t hash[20];
	uint64_t size;
	uint64_t offset;
} __attribute__((packed)) PakInfo;

int main(int argc, const char *argv[]) {
	PakInfo info;
	info.version = 7; // by default
	
	int fd = open(argv[1], O_RDONLY);
	
	if (fd == -1) {
		printf("Failed to open file\n");
		return 1;
	}
	
	uint32_t MountPointSize = 0;
	char MountPoint[1024];
	uint32_t NumEntries = 0;
	
	char temp[1024];
	char data[1024];
	
	uint32_t FilenameSize = 0;
	char Filename[1024];
	uint8_t FileHash[20];
	
	uint64_t FileOffset = 0;
	uint64_t FileSize = 0;
	uint32_t Zip = 0;
	uint64_t Zsize = 0;
	uint8_t Dummy[21];
	uint8_t Encrypted = 0;
	
	uint64_t Chunk = 0;
	
	uint64_t Chunk_Offset = 0;
	uint64_t Chunk_End_Offset = 0;
	
	uint64_t index_size = 0;
	
	
	read(fd, &MountPointSize, 4);
	read(fd, MountPoint, MountPointSize);
	read(fd, &NumEntries, 4);
	
	index_size = 8 + MountPointSize;
	
	
	printf("MountPointSize: %u\n", MountPointSize);
	printf("MountPoint: %s\n", MountPoint);
	printf("NumEntries: %u\n\n", NumEntries);
	
	read(fd, temp, 4);
	read(fd, data, 13);
	index_size += (4 + 13);
	
	printf("1: %s\n", data);
	memset(data, 0, sizeof(data));
	
	read(fd, temp, 78);
	read(fd, data, 24);
	index_size += (78+24);
	
	printf("2: %s\n", data);
	memset(data, 0, sizeof(data));
	
	read(fd, temp, 79);
	read(fd, data, 106);
	index_size += (79+106);
	
	printf("3: %s\n", data);
	memset(data, 0, sizeof(data));
	
	
	read(fd, temp, 98);
	read(fd, data, 104);
	index_size += (98+104);
	printf("4: %s\n", data);
	memset(data, 0, sizeof(data));
	
	
	read(fd, temp, 98);
	read(fd, data, 103);
	index_size += (98+103);
	printf("5: %s\n", data);
	
	memset(data, 0, sizeof(data));
	
	
	read(fd, temp, 99);
	read(fd, data, 101);
	index_size += (99+101);
	printf("6: %s\n", data);
	
	memset(data, 0, sizeof(data));
	
	
	read(fd, temp, 99);
	read(fd, data, 104);
	index_size += (99+104);
	printf("7: %s\n", data);
	
	memset(data, 0, sizeof(data));
	
	
	read(fd, temp, 99);
	read(fd, data, 102);
	index_size += (99+102);
	printf("8: %s\n", data);
	
	memset(data, 0, sizeof(data));
	
	
	
	read(fd, temp, 99);
	read(fd, data, 102);
	index_size += (99+102);
	printf("9: %s\n", data);
	
	memset(data, 0, sizeof(data));
	
	
	
	read(fd, temp, 99);
	read(fd, data, 100);
	index_size += (99+100);
	printf("10: %s\n", data);
	
	memset(data, 0, sizeof(data));
	
	index_size += (99);
	
	printf("Index Size: %lu\n", index_size);
	/*
	for (uint32_t Entry = 0; Entry < 3; Entry++) {
		read(fd, &FilenameSize, 4);
		read(fd, Filename, FilenameSize);
		read(fd, FileHash, 20);
		read(fd, &FileOffset, 8);
		read(fd, &FileSize, 8);
		read(fd, &Zip, 4);
		read(fd, &Zsize, 8);
		read(fd, Dummy, 21);
		read(fd, &Encrypted, 1);
		
		if (Zip != 0) {
			printf("Zip %u\n", Zip);
			read(fd, &Chunk, 4);
			printf("Chunk %lu\n", Chunk);
			if (Chunk != 0) {
				read(fd, &Chunk_Offset, 8);
				read(fd, &Chunk_End_Offset, 8);
			}
		}
		
		printf("FilenameSize: %u\n", FilenameSize);
		printf("Filename: %s\n", Filename);
		printf("FileHash: 0x");
		for (uint8_t i = 0; i < 20; i++) {
			printf("%02X", FileHash[i]);
		}
		printf("\n");
		printf("File Offset: %lu\n", FileOffset);
		printf("File Size: %lu\n", FileSize);
		printf("Zip: %u\n", Zip);
		printf("Zsize: %lu\n", Zsize);
		printf("Encrypted: %u\n", Encrypted);
		
		printf("\n\n");
		
	}*/
	
	
	/*
	if (lseek(fd, -44, SEEK_END) == -1) {
		printf("failed to seek file position\n");
		return 0;
	}
	
	if (read(fd, &info, 44) != 44) {
		printf("Failed to read\n");
		return 0;
	}
	*/
	/*
	if (lseek(fd, info.offset ^ OFFSET_KEY, SEEK_SET) == -1) {
		perror("Error seeking in file");
        close(fd);
        return 1;
	}*/
	
	
	/*
	printf("index_size key: %016lX\n", index_size - 44);
	
	printf("index_size: %lu\n", index_size - 44);
	
	
	*/
	close(fd);
	return 0;
}
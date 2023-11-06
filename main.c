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
	info.version = 7;
	
	int fd = open(argv[1], O_RDONLY);
	
	if (fd == -1) {
		printf("Failed to open file\n");
		return 1;
	}
	
	uint32_t MountPointSize = 0;
	char MountPoint[1024];
	uint32_t NumEntries = 0;
	
	uint32_t FilenameSize = 0;
	char Filename[1024];
	uint8_t FileHash[20];
	
	uint64_t FileOffset = 0;
	uint64_t FileSize = 0;
	uint32_t Zip = 0;
	uint64_t Zsize = 0;
	uint8_t Dummy[21];
	
	uint32_t Chunk = 0;
	
	
	read(fd, &MountPointSize, 4);
	read(fd, MountPoint, MountPointSize);
	read(fd, &NumEntries, 4);
	
	read(fd, &FilenameSize, 4);
	read(fd, Filename, FilenameSize);
	
	read(fd, FileHash, 20);
	
	read(fd, &FileOffset, 8);
	
	read(fd, &FileSize, 8);
	read(fd, &Zip, 4);
	read(fd, &Zsize, 8);
	read(fd, Dummy, 21);
	
	if (info.version >= 3) {
		if (Zip != 0) {
			read(fd, &Chunk, 4);
			
		}
		
		
	}
	
	printf("MountPointSize: %u\n", MountPointSize);
	printf("MountPoint: %s\n", MountPoint);
	printf("NumEntries: %u\n", NumEntries);
	
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
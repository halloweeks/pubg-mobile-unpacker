#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/evp.h>

#define OFFSET_KEY 0xA6D17AB4D4783A41

// #define SIZE_KEY 0x1FFBEE0AB84D0C53
// #define SIZE_KEY 0x1FFBEE0AB84D0C47

/*
void DecryptData(uint8_t *data, uint32_t size, const uint8_t *key, const uint8_t *iv) {
	int len;
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, iv);
	EVP_CIPHER_CTX_set_padding(ctx, 0);
	EVP_DecryptUpdate(ctx, data, &len, data, size);
	EVP_CIPHER_CTX_free(ctx);
}*/

typedef struct {
	uint32_t magic;
	uint32_t version;
	uint8_t hash[20];
	uint64_t size;
	uint64_t offset;
} __attribute__((packed)) PakInfo;

int main(int argc, const char *argv[]) {
	PakInfo info;
	
	int fin = open(argv[1], O_RDONLY);
	
	if (fin == -1) {
		printf("Failed to open file\n");
		return 1;
	}
	
	if (lseek(fin, -44, SEEK_END) == -1) {
		printf("failed to seek file position\n");
		return 0;
	}
	
	if (read(fin, &info, 44) != 44) {
		printf("Failed to read\n");
		return 0;
	}
	
	// printf("index size key: 0x%016lX\n", info.size ^ 1828);
	
	
	if (lseek(fin, info.offset ^ OFFSET_KEY, SEEK_SET) == -1) {
		perror("Error seeking in file");
        close(fin);
        return 1;
	}
	
	
	uint8_t *block = (uint8_t*)malloc(1024 * 1024 * 10);
	
	read(fin, block, (info.size ^ SIZE_KEY));
	
	close(fin);
	
	/*
	read(fin, data, 256);
	close(fin);
	
	uint8_t key[16] = {0};
	uint8_t iv[16] = {0};
	
	DecryptData(data, 256, key, NULL);
	*/
	
	
	int fout = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	
	for (uint32_t index = 0; index < (info.size ^ SIZE_KEY); index++) {
		block[index] ^= 0x79;
	}
	
	lseek(fout, info.offset ^ OFFSET_KEY, SEEK_SET);
	
	write(fout, block, (info.size ^ SIZE_KEY));
	
	
	close(fout);
	
	return 0;
}


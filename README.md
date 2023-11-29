# pubg-mobile-pak-extract

## DecryptData aes algorithm

```
#include <openssl/evp.h>
```

```
void DecryptData(uint8_t *data, uint32_t size, const uint8_t *key) {
	int len;
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL);
	EVP_CIPHER_CTX_set_padding(ctx, 0);
	EVP_DecryptUpdate(ctx, data, &len, data, size);
	EVP_CIPHER_CTX_free(ctx);
}
```

PUBG MOBILE Pak Index Offset obfuscation key
```
0xA6D17AB4D4783A41
```

PUBG MOBILE Pak Index Size obfuscation key,

```
0x1FFBEE0AB84D0C53
```


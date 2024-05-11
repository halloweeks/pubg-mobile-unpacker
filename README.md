# PUBG Mobile Unpacker

**Description:**

PUBG Mobile Pak Extractor is a C program designed to extract contents from PUBG Mobile pak files. This tool is tailored for versions before the 1.1.0 update, as post-update changes in encryption algorithms are not currently supported.

 **Key Features:**

- **Decryption:** Implements XOR decryption for deobfuscation of index offset using a specific key.
- **Decompression Support:** Handles zlib and zstd decompression for extracting compressed data blocks.
- **Filesystem Management:** Utilizes C standard library for efficient file handling, ensuring extracted files are organized in the correct directory structure.
- **User Interface:** Provides clear console output with information on extracted files, including offset, file size, and filename.
- **Execution Time:** Displays the number of files found and the overall execution time for the extraction process.

**Usage:**

1. Compile the program.
2. Run the executable with the PUBG Mobile pak file as an argument.

**Note:**
Ensure compatibility with the version specified, and be aware of the limitations post the 1.1.0 update due to changes in encryption methods.

**How to Use:**

```bash
./pubg_mobile_unpack.exe <path_to_pak_file>
```

**Build Instructions:**

To build the executable, use the following command:

```bash
cc pubg_mobile_unpack.c -o pubg_mobile_unpack.exe -lz -lzstd -O2
```


**pubgm pak file structure:**

```
Format Specifications
Unreal Engine 4 PUBGM PAK file format

FILE DATA
  for each file
    uint8 {20}       - File Hash
    uint64 {8}       - File Offset
    uint64 {8}       - File Size
    uint32 {4}       - Compression Method (0=uncompressed, 1=zLib, 6=zstd)
    uint64 {8}       - Compressed Length
    uint8 {21}       - Dummy byte

    if (compressed) {
        uint32 {4}       - Number of Compressed Blocks
        for each compressed block
            uint64 {8}       - Offset to the start of the compressed data block (relative to the start of the archive)
            uint64 {8}       - Offset to the end of the compressed data block (relative to the start of the archive)
    }
    
    uint32 {4}       - Compressed Block Size
    uint8 {1}        - Is Encrypted
    byte {X}         - File Data
    
INDEX DATA
  uint32 {4}       - Relative Directory Name Length (including null terminator) (10)
  byte {X}         - Relative Directory Name (../../../)
  uint32 {4}       - Number of Files
  for each file
    uint32 {4}       - Filename Length (including null terminator)
    byte {X}         - Filename
    uint8 {20}       - File Hash
    uint64 {8}       - File Offset
    uint64 {8}       - File size
    uint32 {4}       - Compression Method (0=uncompressed, 1=zlib, 6=zstd)
    uint64 {8}       - Compressed Length 
    uint8 {21}       - Dummy byte
    
    if (compressed) {
        uint32 {4}       - Number of Compressed Blocks
        for each compressed block
            uint64 {8}       - Offset to the start of the compressed data block (relative to the start of the archive)
            uint64 {8}       - Offset to the end of the compressed data block (relative to the start of the archive)
    }
    
    uint32 {4}       - Compressed Block Size
    uint8 {1}        - Is Encrypted

FOOTER (45 bytes)
  uint8  {1}       - IsEncrypted (obfuscated) (key) 0x01
  uint32 {4}       - Signature (obfuscated) (key) 0xA0116E7
  uint32 {4}       - Version (normal)
  byte {20}        - IndexData sha1 hash (obfuscated) (key) 0x8CD3A05AD36453DEEDA8CA5926C6955484259BE0
  uint64 {8}       - IndexData length (obfuscated) (key) 0x1FFBEE0AB84D0C43
  uint64 {8}       - IndexData offset (obfuscated) (key) 0xA6D17AB4D4783A41
  
```


**Contributing:**

Contributions are welcome! If you find any issues or have improvements to suggest, please open an issue or submit a pull request.

**License:**

This project is licensed under the [MIT License](LICENSE).

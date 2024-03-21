# PUBG Mobile Pak Extractor

**Description:**

PUBG Mobile Pak Extractor is a C program designed to extract contents from PUBG Mobile pak files. This tool is tailored for versions before the 1.1.0 update, as post-update changes in encryption algorithms are not currently supported.

 **Key Features:**

- **Decryption:** Implements XOR decryption for deobfuscation of index offset using a specific key.
- **Compression Support:** Handles zlib compression for extracting compressed data blocks.
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
cc pubg_mobile_unpack.c -o pubg_mobile_unpack.exe -lz -O2
```

**Contributing:**

Contributions are welcome! If you find any issues or have improvements to suggest, please open an issue or submit a pull request.

**License:**

This project is licensed under the [MIT License](LICENSE).

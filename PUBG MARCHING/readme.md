## compile 

```bash
cc pubg_marching_unpack.c -o pubg_marching_unpack -lz -O2
```
## Usage 
```
./pubg_marching_unpack example.pak
```

## extract multiple pak file
<p>Use bash script </p>

```bash
#!/bin/bash

for pak_file in nzres_extracted/*.pak; do
  if [ -e "$pak_file" ]; then  # Check if the file exists
    ./pubg_marching_unpack $pak_file
  fi
done
```

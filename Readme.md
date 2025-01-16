# About
This application can be used to decompress LZSS files, extract U8 Archives, and convert level files from Super Paper Mario into .fbx files.

# Building
```
git clone https://github.com/AchtungKatse/SPME.git --recurse-submodules SPME
mkdir SPME/build
cd SPME/build
cmake .. && make
```

# Usage

```
SPME convert <map.bin>
    Converts a map file into a .FBX file
    This requires map files to be extracted from Super Paper Mario (i.e. map/he1_01.bin)

SPME u8
    --dump <input file> <output directory>
        Extraces a u8 archive into a directory
    --compile <input directory> <output file> <Compressed (0 or 1)>
        Creates a U8 archive from a directory

SPME lzss
    --decompress <input file> <output file>
    --compress   <input file> <output file>
        Currently only lzss11 is supported
```

# TO DO
- [ ] Reverse engineer vertex attributes UNK_1 / UNK_2
- [ ] Create a blender script to automatically setup meshes (Vertex colors are not mixed with the textures)
- [ ] Map exporting
- [ ] Create Wiki

# Challenges
- Models in SPM are encoded as triangle strips instead of vertices and indices. This creates problems with exporting models since triangle strips are non-standard.

# Known Issues
- Windows builds may have less compatibility than the linux builds since SPME is developed on linux, although they should both be tested before release

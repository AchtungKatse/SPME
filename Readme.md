# About
This application can be used to decompress LZSS files, extract U8 Archives, and convert level files from Super Paper Mario into .fbx files.

![he1_01.bin](https://raw.githubusercontent.com/AchtungKatse/SPME/refs/heads/main/Images/he1_01.png)

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

SPME display <map.bin>
    Opens a fullscreen map viewer
    Controls:
        WASD - Move
        Q - Decrease movement speed
        E - Increase movement speed
        Esc - Close

SPME u8
    dump <input file> <output directory>
        Extraces a u8 archive into a directory
    compile <input directory> <output file> <Compressed (0 or 1)>
        Creates a U8 archive from a directory

SPME lzss
    decompress <input file> <output file>
    compress   <input file> <output file>
        Currently only lzss11 is supported

SPME tpl 
    dump <input tpl file> <output directory>
        Dumps all textures from a TPL into a directory.
```

# TO DO
- [ ] Reverse engineer vertex attributes UNK_1 / UNK_2
- [ ] Create a blender script to automatically setup meshes (Vertex colors are not mixed with the textures)
- [ ] Map exporting
- [ ] Create Wiki

# Challenges
- Models in SPM are encoded as triangle strips instead of vertices and indices. This creates problems with exporting models since triangle strips are non-standard.

# Known Issues
- [ ] UV scaling is not consistent between maps
- [ ] Some maps do not place objects in the correct spots (Seen Below).
	- ![he1_05 Bad Transforms](https://raw.githubusercontent.com/AchtungKatse/SPME/refs/heads/main/Images/he1_05%20Bad%20Transforms.png)
- [ ] Some object positions are dependent on animations which causes extra objects to be visible or in the incorrect place. This is especially noticable on levels with into animations (Seen below)
  	- ![he1_01 Required Animations](https://raw.githubusercontent.com/AchtungKatse/SPME/refs/heads/main/Images/he1_01%20Required%20Animations.png)
- Windows builds may have less compatibility than the linux builds since SPME is developed on linux, although they should both be tested before release

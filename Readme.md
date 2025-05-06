# About
This application can be used to modify level data in Super Paper Mario, namely the level geometry.

![he1_01.bin](/Images/he1_01.png)

# Building
```
git clone https://github.com/AchtungKatse/SPME.git --recurse-submodules SPME
mkdir SPME/build
cd SPME/build
cmake .. && make
```

# Commands
Arguments in square brackets are \[optional] while arguments in brackets are \<required>
## u8
  		extract
  		Format:      <u8 file> <output directory> <compressed>
  		Description: Extracts the contents of a u8 to a directory
  
  		compile
  		Format:      <directory> <output file>
  		Description: Creates a u8 archive file from a directory
  
## tpl
  		dump
  		Format:      <texture.tpl> <output directory>
  		Description: Writes all textures in a tpl to a directory
  
## map
  		to_fbx
  		Format:      <map.bin> <output file> [map name]
  		Description: Converts a SPM map.bin file into an FBX file. The map file should be taken from DATA/files/map.
  
  		from_glb
  		Format:      <map.glb> <config.yaml path> <output_directory>
  		Description: Converts a glb model to a SPM map.dat file. output_directory is a u8 extracted directory with existing map data (required for cameraroad.bin, setup.bin, and skybox textures). Please run u8 compile to create a spm map.bin to reinsert into the game files.
  
  		create_config
  		Format:      <map name> <map.glb> <output path>
  		Description: Creates a map config from an existing model
  
## debug
  		view
  		Format:      <map.bin>
  		Description: Opens a 3d preview of a SPM map.bin file
  
## lzss
  		decompress
  		Format:      <input file> <output file>
  		Description: Decompresses an lzss compressed file.
  
  		compress
  		Format:      <input file> <output file>
  		Description: Uses lzss to compress a file.
  
# Creating New Maps
## Limitations
1. SPME is currently unable to fully generate all data for maps. The currently unsupported files are: cameraroad.bin, setup/(map_name).bin, and all background textures in bg/*.tpl. This means that new map data needs to be 'frankensteined' with an existing map to function.
2. Textures that are too large will cause the game to hang upon loading the map. The largest tested size is 512x512.
3. All textures will be converted to RGBA32. Other image formats are currently unsupported.
4. SPM encodes all geometry as triangle stips. Currently, no algorithm is implemented to convert indexed triangles to triangle strips meaning each triangle will be drawn individually. This may impact performance.
5. LZSS compression is not currently implemented correctly which will result in large map files.

## Creating Map Files
### Extracting files
1. In dolphin, right click Super Paper Mario and select Properties. Navigate to Filesystem, right click the disk, and extract the entire disk to a folder

### Extracting Map Data
2. Select a map to modify and run "./SPME u8 extract <map file> <output directory> true".
    - E.x. "./SPME u8 extract he1_01.bin he1_01 true"
3. Convert the map to an FBX by running "./SPME map to_fbx <map.dat> <output file> \[(optionally) map name\]"
    - E.x. "./SPME map to_fbx he1_01/dvd/map/he1_01/map.dat he1_01.fbx \[he1_01]"
    - If you did not name the output directory in step 1 to the same name as the map file (i.e. he1_01.bin -> he1_01), you may need to specify it at the end of the above command.

The map file is now ready to be imported into a 3D modeling program such as Blender. Please note, graphcial issues may occur due to materials not applying vertex colors.
From here, the map can be edited or optionally created from scratch.

### Importing Models Into SPM
It is important to note that SPME has only been tested with .glb files since they allow for material data and embedded textures.
4. Exporting the geometry to a glb file. The recommended settings for blender are
    1. "Data/Mesh/Vertex Colors/Export Active Vertex Colors"    on
    1. "Data/Mesh/Vertex Colors/Export All Vertex Colors"       on
    3. "Material/Images"            JPEG
    4. "Material/Images/Quality"    100
5. Generate a configuration for the model by running "./SPME map create_config <map name> <map.glb> <output config>"
    - E.x. "./SPME map generate_config he1_01 he1_01_mod.glb mod.yaml"
    - This is created to allow configuring textures and materials. In the future, this will allow for different image formats and material parameters but is mostly unused currently.
6. Reinsert the new geometry data by running "./SPME map from_glb <map.glb> <config.yaml> <output directory>"
    - E.x. "./SPME map from_glb he1_01.glb mod.yaml he1_01"
    - Note: the output directory will be the same one that was created in step 2 explained by Limitations #1.
7. Recompile the working directory into a map.bin file by running "./SPME u8 compile <map directory> <output map> true"
    - E.x. "./SPME u8 compile he1_01 he1_01.bin true"
8. Reinsert the new map file back into DATA/files/map/

### Command Example Cheat Sheet
1. ./SPME u8 extract he1_01.bin he1_01 true
2. ./SPME map to_fbx he1_01/dvd/map/he1_01/map.dat he1_01.fbx \[he1_01]
3. ./SPME map generate_config he1_01 he1_01_mod.glb mod.yaml
4. ./SPME map from_glb he1_01.glb mod.yaml he1_01
5. ./SPME u8 compile he1_01 he1_01.bin true

Items 1 and 2 only need to be done once.
Item 3 will only need to be done if additional materials or textures are added.
Items 4 and 5 will need to be done every time a map is modified.

# TO DO
- [ ] Export animations
- [ ] Reverse engineer cameraroad.bin. Note, this is much more complex then initially expected.
- [ ] Read curve_table section
- [ ] Reverse engineer vertex attributes
- [ ] Create a blender script to automatically setup meshes (Vertex colors are not mixed with the textures)
- [ ] Create Wiki
- [ ] Create algorithm to export triangle strips
- [ ] Rework CMakeLists.txt
- [ ] Map Setup file (might already be done with the SPM Randomizer)

# Challenges
- Models in SPM are encoded as triangle strips instead of vertices and indices. This creates problems with exporting models since triangle strips are non-standard.
- Windows builds may have less compatibility than the linux builds since SPME is developed on linux, although they should both be tested before release

# Known Issues
- Debug view reads TPL images with incorrect colors and does not use vertex colors
- Images that are too large will result in the game hanging.
- Blocks are unable to be changed, removed, or added. Unlike enemies, these are not stoerd in the setup.bin files. The current location of this data seems to be hardcoded in the binary. Further research required

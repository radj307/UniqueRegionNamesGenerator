# UniqueRegionNamesGenerator
Image parser utility used by the Unique Region Names Synthesis patcher to generate maps using a drawn map.

## Building from Source

### Dependencies
- [CMake](https://cmake.org/download) _3.20+_
- [OpenCV](https://github.com/opencv/opencv) _4.x.x_
- [307lib](https://github.com/radj307/307lib) _(git submodule)_

### Build Process
 1. Assuming you have already installed CMake, first install OpenCV to a location of your choice, and add it to your PATH.
 2. Clone the repository to a location of your choice, then `cd` into it and run `git submodule update --init --recursive`
 3. Now use cmake to configure the project. `cmake -B out -S . -DCMAKE_BUILD_TYPE=Release "-DOpenCV_DIR=<PATH_TO_OPENCV>/build"`  
    Make sure you replace `<PATH_TO_OPENCV>` with the location of your OpenCV installation.  
 4. Now use your preferred build tools to build the project.  
    If you're using visual studio on windows, this is the project: `out/ParseImage.sln`.
 
## Usage
 Use `parseimg -h` to see a usage guide, or read below for more details.


# How to Create Source Maps

This program uses images to generate the configurations used by UniqueRegionNamesPatcher.  
It does this by partitioning an image into cells, and comparing the color values of each pixel in that cell to each known region's color values. _(specified using an `ini` file.)_  

This is part of the source image used for the Tamriel worldspace:  
![image](https://user-images.githubusercontent.com/1927798/163854632-59bccec2-0b7f-4ab0-9e35-705c675b3ebb.png)  
![image](https://user-images.githubusercontent.com/1927798/163854701-06edfe6f-818f-48ed-b194-65636676f949.png)  
Each 100px by 100px red box is 1 exterior cell.  
The colors present within each cell determine which regions are assigned to it.  
If multiple regions are assigned to the same cell, the region with the highest priority wins.

A region's area data is found by parsing all of the points in a region to find the verticies, which are then converted to raw coordinates by the patcher and assigned to the new region.  

The regions located in each cell are then written to a file, in addition to the area data for each region.  
That data is then used by UniqueRegionNamesPatcher to create the `esp` file.

## Requirements
- Paint.NET or similar.
- A decent amount of RAM
- A map of all of the cells in the worldspace.

## Process

You'll need to choose a unique RGB color value for each region you want to add.  
The colors cannot match any lines or numbers already present on the map!

 1. Draw the regions on the map using your chosen colors, adding each new region/color to an `ini` file in the following format:
```ini
; EditorID  (required)
; This is the 'header' tag, which defines the editor ID. All setter lines belong to the header they are located in.
; It must be prefixed with `xxxMap`!
[EditorID]
; color  (required)
; This is the RGB hexadecimal color code of the region on the source map. If you're using Paint.NET, paste the hex color here.
; DO NOT INCLUDE ANY OTHER CHARACTERS! This must be EXACTLY 6 characters long!
color = "FFFFFF"
; priority  (optional)
; This determines the level of priority that this region's name takes over other regions in the same areas.
; If this is lower than other regions in the same area, this region's map name won't be shown on doors.
priority = 60
; mapName  (optional)
; This is used to directly set the text shown on doors, in this format: `Open <mapName>`. Spaces are allowed.
; The default map name is parsed using camel casing from the editor ID field. (Excluding the `xxxMap` prefix.)
; An editor ID of `xxxMapShorsStone` will provide a default map name of `Shors Stone`, which appears on doors as `Open Shors Stone`.
mapName = "Text that will be shown on doors"
```
 2. Run `parseimg` and provide the following options:
    - `-f`/`--file` specifies the input map image path.
    - `i`/`--ini` specifies the corresponding `ini` file.  
      _(This option can be specified multiple times, each subsequent file is merged in-memory overwriting any previous key-value pairs with identical names.)_
    - `-d`/`--dim` specifies the resolution of an individual cell.  
      For the default skyrim map in this repository, use `-d=100:100`, since each cell is 100px by 100px.
    - `-w`/`--worldspace` specifies the name of the output files.  
      2 files are created with the following names:
      - `<worldspace>.region.txt`
      - `<worldspace>.map.txt`
    - You can also use the `-o`/`--out` option to specify an output ___directory___, where the files listed above will be located.
 3. You can now run UniqueRegionNamesPatcher with the newly created files specified as overrides in the settings menu.

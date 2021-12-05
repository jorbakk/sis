# SIS - Single  Image  Stereogram  generator
---
Sis is a program for generating pictures with a 3-dimensional effect which can
be percieved without any supplementary technical devices. At a first glance the
sis images look like random patterns or noise but if you know how to view them
correctly you can see a 3-dimensional shape in it. This can be achieved by
converging your eyes to a more distant point behind the screen while focusing
at the distance of the screen. However, it needs a little training. If you know
those "Magic Eye" books, you know what to do.

By default, the background of the SIS is made of random black and white dots
but you can also set a colored texture-map instead. Texture-maps that change
their information whithin a small environment give the best results. The depth
information of the 3-D objects can be read from a grayscale picture.  Examples
of depth-maps are provided in the repo. The farest distance is coded as black
while near points are white. For creating depth-maps you may use a ray-tracer or 3D-modeller.

## Supported file formats
---
* tiff - for input, output and texture
* tga  - for input

## Requirements
---
* libgr (at least version 1.3)

## Installation
---
Build from sources:

* Change to the src directory
* Edit the Makefile to change the path for libgr, and where you may want
  to install the binary and man-page.
* type 'make'.
* type 'make install' to install the binary and man-page (as root).
If everything went o.k. you should have a sis excecutable in the src directory.

## Usage
---
Generate a SIS of a simple 3-D sinus oval figure which is supplied in the
depthmaps subdirectory:

     ./sis ../depthmaps/oval.tif out.tif

Invoked with the -m option, sis adds two triangles to the top of the
picture, which help you to find the right convergence.

     ./sis ../depthmaps/oval.tif out.tif -m

Use a texture instead of the random noise background:

     ./sis ../depthmaps/oval.tif out.tif -t ../textures/cork.tif

To see all available options run sis without any arguments.

## Printing a sis
---
  You can size and choose the resolution of your SIS with the -x, -y options.
  For example, if your printer has 300dpi, use:

     ./sis ../depthmaps/oval.tif out.tif -x40i300 -y30i300

  which gives a picture of size 4x3 inch at 300 dpi. Note that 40 means
  40 tenths of an inch = 4 inch.

Let me know if you have any suggestions, bugs, improvements, questions, new
sis-algorithms, fancy depth- and texture-maps.

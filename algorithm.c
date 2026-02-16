/*
 * Copyright 1995, 2021, 2022, 2025 Jörg Bakker
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "sis.h"

/*

1. Overview of most important parameters and formulas for the algorithms:


      (_O_)     <-   eye_dist   ->     (_O_)
         \                              /                            ^
          \                            /                             |
           \                          /                              | view_dist
            \                        /                               |
             \                      /                                |
              \   <-   sep   ->    /                                 =
 ====================================================================== screen_z
                \                /                                   ^
                 \              /                                    |
 --------------------------------------------------  near plane (z=SIS_MAX_DEPTH)
                   \          /                          ^           |
                    \        /                           |           |
                     \      /                            |           |
                      \    /                             |           |
                       \  /                              | u = 0.8   | t = 1.2
                        \/                               | (-n 20)   | (-f 120)
              *****************          --- z=max_depth |           |
         *****        ^                                  |           |
     ****          zvalue      ********  --- z=min_depth |           |
                      =                                  =           =
 ---------------------------------------------------------  far plane (z=0) ----

 screen_z * u  =  SIS_MAX_DEPTH
 => screen_z   =  SIS_MAX_DEPTH / u
 screen_z / t  =  view_dist

 sep / (screen_z - z_val)  =  eye_dist / (screen_z - z_val + view_dist)

 z values are zero at far plane and increase towards the viewer.


2. Data structures

DBuffer:
Grey-scale values with depth information taken from the depth image.

IdentBuffer:
Contains for each pixel of the SIS output image of one row the column index
of the pixel image that has the same color in this row. So, IdentBuffer
contains indices of locations in the image, not color indices.

SISBuffer:
Color values (palette indices) of each pixel in the output SIS image.

SIScolorRGB:
Like the SISBuffer, but with rgb color values and not palette indices.
This allows for having more colors in the output SIS image than
the color palette taken from the texture image provides.

*/

/// The biggest and smallest distance in one line (!)
/// of the depth-map. This is just for efficiency.
z_t max_depth_in_row, min_depth_in_row, max_depth, min_depth;

int algorithm;
/// Just for statistics.
long forwards_obscure_c, backwards_obscure_c;
long inner_propagate_c, outer_propagate_c;

/// Near and far plane
float t, u;

/// References to equally-colored pixels
static ind_t *IdentBuffer;
// static col_t *IdentBuffer;

/// Separation of each possible z
static ind_t separation[SIS_MAX_COLORS + 1];
/// Ascend dz to check for hidden pixels
static pos_t dz[SIS_MAX_COLORS + 1];

static pos_t DBufStep;
/// Proportions of near and far-plane
static int numerator, denominator;

col_t *SISBuffer = NULL;

/// IdentBuffer's equivalent in algo #4 are lookL and lookR
static int *lookL, *lookR;

void
InitAlgorithm(void)
{
	if (!SISwidth && !SISheight) {
		SISwidth = Dwidth;
		SISheight = Dheight;
	}
	if (!SISwidth) {
		SISwidth = SISheight * (float)Dwidth / (float)Dheight;
	}
	if (!SISheight) {
		SISheight = SISwidth * (float)Dheight / (float)Dwidth;
	}

	/// max(int) > SIS_MAX_DEPTH
	int i;

	if (origin > SISwidth) {
		fprintf(stderr, "Starting point out of range\n");
		exit(1);
	}

	DBufStep = (double)Dwidth / (double)SISwidth;
	for (i = 0; i <= SIS_MAX_COLORS; i++) {
		zvalue[i] = -1;
	}
	numerator = SIS_MAX_DEPTH / u;
	denominator = SIS_MAX_DEPTH / u + SIS_MAX_DEPTH / (t * u);
	// printf("DBufStep: %f\n", DBufStep);
	// printf("SIS_MAX_DEPTH: %d\n", SIS_MAX_DEPTH);
	// printf("u: %f\n", u);
	// printf("t: %f\n", t);
	// printf("numerator: %d\n", numerator);
	// printf("denominator: %d\n", denominator);
	if (SIStype != SIS_TEXT_MAP) {
		Twidth = 1;
		Theight = 1;
	}

	if (eye_dist == 0) {
		eye_dist = metric2pixel(22, resolution);
	}
	halfstripwidth = eye_dist * t / (2 * (1 + t));
	halftriangwidth = SISwidth / 75;
	if (!halftriangwidth)
		halftriangwidth = 4;
	DLineStep = (double)Dheight / (double)SISheight;
	DLinePosition = 0.0;
	/// Set the original default value for origin in case of algo #4,
	/// it is not in the center of the image because some artifacts
	/// in the background appear
	if (origin == -1 && algorithm < 4) {
		origin = SISwidth >> 1;
	}
	switch (SIStype) {
	case SIS_RANDOM_GREY:
		SISred[0] = SISgreen[0] = SISblue[0] = white_value;
		white = 0;
		for (int i = 1; i < rand_grey_num; i++) {
			SISred[i] = SISgreen[i] = SISblue[i] =
			    i * (float)SIS_MAX_CMAP / (float)rand_grey_num;
		}
		break;
	case SIS_RANDOM_COLOR:
		for (int i = 0; i < rand_col_num; i++) {
			SISred[i] = rand() / ((float)RAND_MAX / (float)SIS_MAX_CMAP);
			SISgreen[i] = rand() / ((float)RAND_MAX / (float)SIS_MAX_CMAP);
			SISblue[i] = rand() / ((float)RAND_MAX / (float)SIS_MAX_CMAP);
		}
		break;
	}
	SISred[SIS_MAX_COLORS] = SISgreen[SIS_MAX_COLORS] = SISblue[SIS_MAX_COLORS]
	    = black_value;
	black = SIS_MAX_COLORS;
	max_depth = SIS_MIN_DEPTH;
	min_depth = SIS_MAX_DEPTH;

	inner_propagate_c = 0;
	outer_propagate_c = 0;
	forwards_obscure_c = 0;
	backwards_obscure_c = 0;
}


void
DaddEntry(col_t index, z_t zval)
{
	if (invert)
		zval = SIS_MAX_DEPTH - zval;
	if (zval < min_depth_in_row)
		min_depth_in_row = zval;
	if (zval > max_depth_in_row)
		max_depth_in_row = zval;
	if (min_depth_in_row < min_depth)
		min_depth = min_depth_in_row;
	if (max_depth_in_row > max_depth)
		max_depth = max_depth_in_row;

	if (zvalue[index] == -1) {
		zvalue[index] = zval;
		if (algorithm == 4) {
			separation[index] = eye_dist * oversam * (numerator - zval) / (denominator - zval);
			dz[index] = (double)(denominator - zval) / (double)(((eye_dist * oversam) >> 1) * DBufStep);
		} else {
			separation[index] = eye_dist * (numerator - zval) / (denominator - zval);
			dz[index] = (double)(denominator - zval) / (double)((eye_dist >> 1) * DBufStep);
		}
	}
}


void
AllocBuffers(void)
{
	if ((DBuffer = (col_t *)calloc(Dwidth * oversam, sizeof(col_t))) == NULL) {
		fprintf(stderr, "Couldn't alloc memory for depth buffer.\n");
		exit(1);
	}
	if ((IdentBuffer = (ind_t *)calloc(SISwidth * oversam, sizeof(ind_t))) == NULL) {
	// if ((IdentBuffer = (col_t *) calloc(SISwidth * oversam, sizeof(col_t))) == NULL) {
		fprintf(stderr, "Couldn't alloc memory for ident buffer\n");
		free(DBuffer);
		exit(1);
	}
	if ((SISBuffer = (col_t *)calloc(SISwidth * oversam, sizeof(col_t))) == NULL) {
		fprintf(stderr, "Couldn't alloc memory for SIS buffer.\n");
		free(DBuffer);
		free(IdentBuffer);
		exit(1);
	}
	if ((SIScolorRGB = (col_rgb_t *)calloc(SISwidth * oversam, sizeof(col_rgb_t))) == NULL ) {;
		fprintf(stderr, "Couldn't alloc memory for SIScolorRGB buffer.\n");
		free(DBuffer);
		free(IdentBuffer);
		free(SISBuffer);
		exit(1);
	}
	lookL = (int *)calloc(SISwidth * oversam, sizeof(int));
	lookR = (int *)calloc(SISwidth * oversam, sizeof(int));
}


void
FreeBuffers(void)
{
	free(DBuffer);
	free(IdentBuffer);
	free(SISBuffer);
	free(SIScolorRGB);
	free(lookL);
	free(lookR);
}


/// Add the little, nice triangles in black
void
AddTriangles(ind_t y)
{
	col_rgb_t black_rgb = {0, 0, 0};
	if ((y < halftriangwidth) && ((SISwidth >> 1) > halfstripwidth + halftriangwidth)) {
		for (ind_t i = halftriangwidth - y; i >= 0; i--) {
			SIScolorRGB[(SISwidth >> 1) - halfstripwidth - i] = black_rgb;
			SIScolorRGB[(SISwidth >> 1) - halfstripwidth + i] = black_rgb;
			SIScolorRGB[(SISwidth >> 1) + halfstripwidth - i] = black_rgb;
			SIScolorRGB[(SISwidth >> 1) + halfstripwidth + i] = black_rgb;
		}
	}
}


void
CalcIdentLine(void)
{
	z_t z, z_limit;             /* z_limit > SIS_MAX_DEPTH !!!!!!!!! */
	ind_t i, DBufInd, IdentBufInd, left, right, IdInd;
	pos_t DBufPos, ZPos;
	int visible;

	for (i = 0; i < SISwidth; i++)  /* point to yourself */
		IdentBuffer[i] = i;

	/// Handle the right half of the picture from the origin
	DBufPos = (SISwidth - 1) * DBufStep;
	for (IdentBufInd = SISwidth - 1; IdentBufInd >= origin; IdentBufInd--) {
		DBufInd = (int)DBufPos;
		z = zvalue[DBuffer[DBufInd]];

		/// Left eye sees this:
		left = IdentBufInd - (separation[DBuffer[DBufInd]] >> 1);
		/// Right eye sees this:
		right = left + separation[DBuffer[DBufInd]];
		/// Both is within the SIS-picture
		if ((0 <= left) && (right < SISwidth)) {
			visible = 1;
			if (algorithm > 2) {
				/// Check for hidden pixels:
				ZPos = z + dz[DBuffer[DBufInd]];
				z_limit = (unsigned int)ZPos;
				for (i = 1; z_limit < (unsigned int)max_depth_in_row; i++) {
					/// Does right eye see all?
					if (zvalue[DBuffer[DBufInd + i]] > z_limit) {
						visible = 0;
						backwards_obscure_c++;
						break;
					}
					/// Does left eye see all?
					if (zvalue[DBuffer[DBufInd - i]] > z_limit) {
						visible = 0;
						forwards_obscure_c++;
						break;
					}
					ZPos += dz[DBuffer[DBufInd]];
					/// Don't go further than the nearest point
					z_limit = (unsigned int)ZPos;
				}
			}
			if (visible) {
				if (algorithm > 1) {
					/// Now do the propagation stuff
					IdInd = IdentBuffer[right];
					/// Already pointed at a pixel between left and right
					while ((origin <= left) && (IdInd != left)
					       && (IdInd != right)) {
						if (IdInd > left) {
							inner_propagate_c++;
							right = IdInd;
							IdInd = IdentBuffer[right];
						}
						/// Already pointed at a pixel outside of left and right
						else {
							outer_propagate_c++;
							IdentBuffer[right] = left;
							right = left;
							left = IdInd;
							IdInd = IdentBuffer[right];
						}
					}
				}
				/// Here's what the most simple SIS-algorithm does (nearly nothing)
				IdentBuffer[right] = left;
			}
		}
		DBufPos -= DBufStep;
	}

	/// Handle the left half of the picture from the origin
	DBufPos = 0.0;
	for (IdentBufInd = 0; IdentBufInd < origin; IdentBufInd++) {
		DBufInd = (int)DBufPos;
		z = zvalue[DBuffer[DBufInd]];

		left = IdentBufInd - (separation[DBuffer[DBufInd]] >> 1);
		right = left + separation[DBuffer[DBufInd]];

		if ((0 <= left) && (right < SISwidth)) {
			visible = 1;
			if (algorithm > 2) {
				ZPos = z + dz[DBuffer[DBufInd]];
				z_limit = (unsigned int)ZPos;
				for (i = 1; z_limit < (unsigned int)max_depth_in_row; i++) {
					if (zvalue[DBuffer[DBufInd + i]] > z_limit) {
						visible = 0;
						forwards_obscure_c++;
						break;
					}
					if (zvalue[DBuffer[DBufInd - i]] > z_limit) {
						visible = 0;
						backwards_obscure_c++;
						break;
					}
					ZPos += dz[DBuffer[DBufInd]];
					z_limit = (unsigned int)ZPos;
				}
			}
			if (visible) {
				if (algorithm > 1) {
					IdInd = IdentBuffer[left];
					while ((right < origin) && (IdInd != left)
					       && (IdInd != right)) {
						if (IdInd < right) {
							inner_propagate_c++;
							left = IdInd;
							IdInd = IdentBuffer[left];
						} else {
							outer_propagate_c++;
							IdentBuffer[left] = right;
							left = right;
							right = IdInd;
							IdInd = IdentBuffer[left];
						}
					}
				}
				IdentBuffer[left] = right;
			}
		}
		DBufPos += DBufStep;
	}
}


#define random_texture_size (200)
col_t random_texture[random_texture_size][random_texture_size];

void
init_random_texture(void)
{
	Theight = random_texture_size;
	Twidth = random_texture_size;
	for (int line_number = 0; line_number < random_texture_size; ++line_number) {
		for (int i = 0; i < random_texture_size; i++) {
			switch (SIStype) {
			case SIS_RANDOM_GREY:
				if (rand_grey_num == 2)
					random_texture[line_number][i] = (rand() > (RAND_MAX * density)) ? white : black;
				else
					random_texture[line_number][i] = rand() / (RAND_MAX / rand_grey_num);
				break;
			case SIS_RANDOM_COLOR:
				random_texture[line_number][i] = rand() / (RAND_MAX / rand_col_num);
				break;
			}
		}
	}
}


void
InitSISBuffer(ind_t LineNumber)
{
	// init_random_texture();
	for (ind_t i = 0; i < SISwidth * oversam; i++) {
		switch (SIStype) {
		case SIS_RANDOM_GREY:
			if (rand_grey_num == 2)
				SISBuffer[i] = (rand() > (RAND_MAX * density)) ? white : black;
			else
				SISBuffer[i] = rand() / (RAND_MAX / rand_grey_num);
			break;
		case SIS_RANDOM_COLOR:
			SISBuffer[i] = rand() / (RAND_MAX / rand_col_num);
			break;
		case SIS_TEXT_MAP:
			SISBuffer[i] = ReadTPixel(LineNumber % Theight, i % Twidth);
			break;
		}
	}
}


void
FillRGBBuffer(ind_t LineNumber)
{
	ind_t i;
	/// Set the color of two corresponding pixels to the same value.
	/// right half:
	for (i = origin; i < SISwidth; i++) {
		if (IdentBuffer[i] != i)
			SISBuffer[i] = SISBuffer[IdentBuffer[i]];
	}

	/// Left half:
	for (i = origin - 1; i >= 0; i--) {
		if (IdentBuffer[i] != i)
			SISBuffer[i] = SISBuffer[IdentBuffer[i]];
	}
	/// Fill the RGB buffer for writing to the output image
	for (i = 0; i < SISwidth; i++) {
		col_rgb_t colrgb = { SISred[SISBuffer[i]], SISgreen[SISBuffer[i]], SISblue[SISBuffer[i]] };
		SIScolorRGB[i] = colrgb;
	}
	if (mark) AddTriangles(LineNumber);
}


col_t
get_pixel_from_pattern(int x, int y)
{
	col_t ret = {0};
	switch (SIStype) {
	case SIS_RANDOM_GREY:
	case SIS_RANDOM_COLOR:
		ret = SISBuffer[x];
		// ret = random_texture[y % random_texture_size][x % random_texture_size];
		break;
	case SIS_TEXT_MAP:
		ret = ReadTPixel(y % Theight, x % Twidth);
		break;
	}
	// ret = ReadTPixel(y % Theight, x % Twidth);
	// printf("get texture pixel from [x,y]: [%d,%d] -> %d\n", x, y, ret);
	return ret;
}


/// Oversampling ratio
int oversam;

void
asteer(ind_t LineNumber)
{
	// int obsDist  = 1500;   /// original distance from viewer to screen
	// int maxdepth = 675;    /// original distance from screen to far plane
	int obsDist  = SIS_MAX_DEPTH / u;          /// distance from viewer to screen
	int maxdepth = SIS_MAX_DEPTH / (u * t);    /// distance from screen to far plane
	int lastlinked;
	int i;
	/// Shift texture map 4 pixels in vertical direction
	int yShift = 4;
	/// Pattern must be at least this wide
	int maxsep = (int)(((long)eye_dist * oversam * maxdepth) / (maxdepth + obsDist));
	int vmaxsep = oversam * maxsep;
	int vwidth = SISwidth * oversam;
	int start = vwidth / 2 - vmaxsep / 2;
	if (origin != -1) {
		start = origin * oversam;
	}
	int poffset = vmaxsep - (start % vmaxsep);
	int sep = 0;
	int x, left, right;
	bool vis;

	if (LineNumber == 0) {
		// printf("PARAMS --------------------------------------------------------------\n");
		// printf("SISwidth: %d\n", SISwidth);
		// printf("vwidth: %d\n", vwidth);
		// printf("xdpi: %d\n", xdpi);
		// printf("ydpi: %d\n", ydpi);
		// printf("yShift: %d\n", yShift);
		// printf("obsDist: %d\n", obsDist);
		// printf("eye_sep: %d\n", eye_sep);
		// printf("veyeSep: %d\n", veyeSep);
		// printf("maxdepth: %d\n", maxdepth);
		// printf("maxsep: %d\n", maxsep);
		// printf("oversam: %d\n", oversam);
		// printf("vmaxsep: %d\n", vmaxsep);
		// printf("start: %d\n", start);
		// printf("poffset: %d\n", poffset);
	}

	/// Initialize ident buffer (lookL, lookR correspond to IdentBuffer in algorithm < 4)
	for (x = 0; x < vwidth; x++) {
		lookL[x] = x;
		lookR[x] = x;
	}
	/// Set indices of identical color pixels in 'virtual' buffer based on
	/// eye separation and depth value
	for (x = 0; x < vwidth; x++) {
		if ((x % oversam) == 0) // speedup for oversampled pictures
		{
	        // featureZ = maxdepth - DBuffer[x / oversam] * maxdepth / 256;
	        // featureZ = maxdepth - zvalue[x / oversam] * maxdepth / 256;
		    // sep = (int)(((long)veyeSep * featureZ) / (featureZ + obsDist));
            // printf("sep: %d\n", sep);

            // sep = separation[x / oversam];
            ind_t DBufInd = (x / oversam) * DBufStep;
            sep = separation[DBuffer[DBufInd]];
		}
		left = x - sep / 2;
		right = left + sep;
		vis = true;
		if ((left >= 0) && (right < vwidth)) {
			if (lookL[right] != right)      // right pt already linked
			{
				if (lookL[right] < left)    // deeper than current
				{
					lookR[lookL[right]] = lookL[right]; // break old links
					lookL[right] = right;
				} else
					vis = false;
			}

			if (lookR[left] != left)        // left pt already linked
			{
				if (lookR[left] > right)    // deeper than current
				{
					lookL[lookR[left]] = lookR[left];   // break old links
					lookR[left] = left;
				} else
					vis = false;
			}
			if (vis == true) {
				lookL[right] = left;
				lookR[left] = right;
			}                   // make link
		}
	}
	// for (x = 0; x < vwidth; x++) {
		// printf("%d|%d,", lookL[x], lookR[x]);
	// }
	// printf("\n");

	/// Set color values based on the ident buffers and texture map
	/// ... starting from roughly the center start, going right ...
	lastlinked = -10;           // dummy initial value
	for (x = start; x < vwidth; x++) {
		if ((lookL[x] == x) || (lookL[x] < start)) {
			if (lastlinked == (x - 1))
				SISBuffer[x] = SISBuffer[x - 1];
			else {
				SISBuffer[x] = get_pixel_from_pattern(
				  ((x + poffset) % vmaxsep) / oversam,
				   (LineNumber + ((x - start) / vmaxsep) * yShift) % Theight);
			}
		} else {
			SISBuffer[x] = SISBuffer[lookL[x]];
			lastlinked = x;     // keep track of the last pixel to be constrained
		}
	}
	/// ... starting from roughly the center start, going left ...
	lastlinked = -10;           // dummy initial value
	for (x = start - 1; x >= 0; x--) {
		if (lookR[x] == x) {
			if (lastlinked == (x + 1))
				SISBuffer[x] = SISBuffer[x + 1];
			else {
				SISBuffer[x] = get_pixel_from_pattern(
				  ((x + poffset) % vmaxsep) / oversam,
				  (LineNumber + ((start - x) / vmaxsep + 1) * yShift) % Theight);
			}
		} else {
			SISBuffer[x] = SISBuffer[lookR[x]];
			lastlinked = x;     // keep track of the last pixel to be constrained
		}
	}

	int red, green, blue;
	for (x = 0; x < vwidth; x += oversam) {
		red = 0;
		green = 0;
		blue = 0;
		/// Use average color of virtual pixels for screen pixel
		for (i = x; i < (x + oversam); i++) {
			/// Get RGB colors for color palette index col and sum them up
			red += SISred[SISBuffer[i]];
			green += SISgreen[SISBuffer[i]];
			blue += SISblue[SISBuffer[i]];
		}
		col_rgb_t colrgb = { red / oversam, green / oversam, blue / oversam };
		SIScolorRGB[x / oversam] = colrgb;
	}
	if (mark) AddTriangles(LineNumber);

	// free(lookL);
	// free(lookR);
}

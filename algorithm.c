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

 Overview of most important parameters and formulas for the algorithms:


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
     ****            z_val     ********  --- z=min_depth |           |
                      =                                  =           =
 ---------------------------------------------------------  far plane (z=0) ----

 screen_z * u  =  SIS_MAX_DEPTH
 => screen_z   =  SIS_MAX_DEPTH / u
 screen_z / t  =  view_dist

 sep / (screen_z - z_val)  =  eye_dist / (screen_z - z_val + view_dist)

 z values are zero at far plane and increase towards the viewer.

*/

/// The biggest and smallest distance in one line (!)
/// of the depth-map. This is just for efficiency.
z_t max_depth_in_row, min_depth_in_row, max_depth, min_depth;

int algorithm;
/// Just for statistics.
long forwards_obscure_c, backwards_obscure_c;
long inner_propagate_c, outer_propagate_c;

/// Near and far plane
double t, u;

/// References to equally-colored pixels
static ind_t *IdentBuffer;

/// Separation of each possible z
static ind_t separation[SIS_MAX_COLORS + 1];
/// Ascend dz to check for hidden pixels
static pos_t dz[SIS_MAX_COLORS + 1];

static pos_t DBufStep;
/// Proportions of near and far-plane
static int numerator, denominator;


void
InitAlgorithm(void)
{
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
		separation[index] = E * (numerator - zval) / (denominator - zval);
		dz[index] = (double)(denominator - zval) / (double)((E >> 1) * DBufStep);
	}
}


void
InitBuffers(void)
{
	if ((DBuffer = (col_t *) calloc(Dwidth, sizeof(col_t))) == NULL) {
		fprintf(stderr, "Couldn't alloc space for DBuffer.\n");
		exit(1);
	}
	if ((IdentBuffer = (ind_t *) calloc(SISwidth, sizeof(ind_t))) == NULL) {
		fprintf(stderr, "Couldn't alloc space for IdentBuffer\n");
		free(DBuffer);
		exit(1);
	}
	if ((SISBuffer = (col_t *) calloc(SISwidth, sizeof(col_t))) == NULL) {
		fprintf(stderr, "Couldn't alloc space for SISBuffer.\n");
		free(DBuffer);
		free(IdentBuffer);
		exit(1);
	}
	if (algorithm < 4) {
		SIScolorRGB = (col_rgb_t *)calloc(SISwidth, sizeof(col_rgb_t));
	}
}


void
FreeBuffers(void)
{
	free(DBuffer);
	free(IdentBuffer);
	free(SISBuffer);
	if (algorithm < 4) {
		free(SIScolorRGB);
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

	/* handle the right half of the picture from the origin. */
	DBufPos = (SISwidth - 1) * DBufStep;
	for (IdentBufInd = SISwidth - 1; IdentBufInd >= origin; IdentBufInd--) {
		DBufInd = (int)DBufPos;
		z = zvalue[DBuffer[DBufInd]];

		/* left eye sees this: */
		left = IdentBufInd - (separation[DBuffer[DBufInd]] >> 1);
		/* right eye sees this: */
		right = left + separation[DBuffer[DBufInd]];
		/* both is within the SIS-picture */
		if ((0 <= left) && (right < SISwidth)) {
			visible = 1;
			if (algorithm > 2) {
				/* check for hidden pixels: */
				ZPos = z + dz[DBuffer[DBufInd]];
				z_limit = (unsigned int)ZPos;
				for (i = 1; z_limit < (unsigned int)max_depth_in_row; i++) {
					/* does right eye see all? */
					if (zvalue[DBuffer[DBufInd + i]] > z_limit) {
						visible = 0;
						backwards_obscure_c++;
						break;
					}
					/* does left eye see all? */
					if (zvalue[DBuffer[DBufInd - i]] > z_limit) {
						visible = 0;
						forwards_obscure_c++;
						break;
					}
					ZPos += dz[DBuffer[DBufInd]];
					/* don't go further than the nearest point */
					z_limit = (unsigned int)ZPos;
				}
			}
			if (visible) {
				if (algorithm > 1) {
					/* now do the propagation stuff */
					IdInd = IdentBuffer[right];
					/* already pointed at a pixel between left and right */
					while ((origin <= left) && (IdInd != left)
					       && (IdInd != right)) {
						if (IdInd > left) {
							inner_propagate_c++;
							right = IdInd;
							IdInd = IdentBuffer[right];
						}
						/* already pointed at a pixel outside of left and right */
						else {
							outer_propagate_c++;
							IdentBuffer[right] = left;
							right = left;
							left = IdInd;
							IdInd = IdentBuffer[right];
						}
					}
				}
				/* here's what the original SIS-algorithm does (nearly nothing). */
				IdentBuffer[right] = left;
			}
		}
		DBufPos -= DBufStep;
	}

	/* handle the left half of the picture from the origin. */
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


void
FillSISBuffer(ind_t LineNumber)
{
	ind_t i;

	for (i = 0; i < SISwidth; i++) {
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

	/* set the color of two corresponding pixels to the same value. */
	/* right half: */
	for (i = origin; i < SISwidth; i++) {
		if (IdentBuffer[i] != i)
			SISBuffer[i] = SISBuffer[IdentBuffer[i]];
	}

	/* left half: */
	for (i = origin - 1; i >= 0; i--) {
		if (IdentBuffer[i] != i)
			SISBuffer[i] = SISBuffer[IdentBuffer[i]];
	}

	/* add the little, nice triangles in some color. */
	if ((LineNumber < halftriangwidth) && mark
	    && ((SISwidth >> 1) > halfstripwidth + halftriangwidth))
		for (i = halftriangwidth - LineNumber; i >= 0; i--) {
			SISBuffer[(SISwidth >> 1) - halfstripwidth - i] = black;
			SISBuffer[(SISwidth >> 1) - halfstripwidth + i] = black;
			SISBuffer[(SISwidth >> 1) + halfstripwidth - i] = black;
			SISBuffer[(SISwidth >> 1) + halfstripwidth + i] = black;
		}
	/// Fill the RGB buffer for writing to the output image
	for (i = 0; i < SISwidth; i++) {
		col_rgb_t colrgb = { SISred[SISBuffer[i]], SISgreen[SISBuffer[i]], SISblue[SISBuffer[i]] };
		SIScolorRGB[i] = colrgb;
	}
}


col_t
get_pixel_from_pattern(int x, int y)
{
	col_t ret = ReadTPixel(y % Theight, x % Twidth);
	// printf("get texture pixel from [x,y]: [%d,%d] -> %d\n", x, y, ret);
	return ret;
}


/// Oversampling ratio
int oversam;

void
asteer(ind_t y)
{
	// int obsDist  = 1500;   /// original distance from viewer to screen
	// int maxdepth = 675;    /// original distance from screen to far plane
	int obsDist  = SIS_MAX_DEPTH / u;          /// distance from viewer to screen
	int maxdepth = SIS_MAX_DEPTH / (u * t);    /// distance from screen to far plane

	int lastlinked;
	int i;
	/// Shift texture map 4 pixels in vertical direction
	int yShift = 4;
	int veyeSep = E * oversam;
	int vwidth = SISwidth * oversam;
	int *lookL = (int *)calloc(vwidth, sizeof(int));
	int *lookR = (int *)calloc(vwidth, sizeof(int));
	col_t *color = (col_t *)calloc(vwidth, sizeof(col_t));
	SIScolorRGB = (col_rgb_t *)calloc(vwidth, sizeof(col_rgb_t));

	/// Pattern must be at least this wide
	int maxsep = (int)(((long)E * maxdepth) / (maxdepth + obsDist));
	int vmaxsep = oversam * maxsep;
	int s = vwidth / 2 - vmaxsep / 2;
	int poffset = vmaxsep - (s % vmaxsep);

	// printf("PARAMS --------------------------------------------------------------\n");
	// printf("SISwidth: %d\n", SISwidth);
	// printf("vwidth: %d\n", vwidth);
	// printf("xdpi: %d\n", xdpi);
	// printf("ydpi: %d\n", ydpi);
	// printf("yShift: %d\n", yShift);
	// printf("obsDist: %d\n", obsDist);
	// printf("E: %d\n", eyeSep);
	// printf("veyeSep: %d\n", veyeSep);
	// printf("maxdepth: %d\n", maxdepth);
	// printf("maxsep: %d\n", maxsep);
	// printf("oversam: %d\n", oversam);
	// printf("vmaxsep: %d\n", vmaxsep);
	// printf("s: %d\n", s);
	// printf("poffset: %d\n", poffset);

	int featureZ = 0, sep = 0;
	int x, left, right;
	bool vis;

	/// Initialize ident buffer (IdentBuffer in algorithm < 4)
	for (x = 0; x < vwidth; x++) {
		lookL[x] = x;
		lookR[x] = x;
	}
	/// Set indices of identical color pixels in 'virtual' buffer based on
	/// eye separation and depth value
	for (x = 0; x < vwidth; x++) {
		if ((x % oversam) == 0) // speedup for oversampled pictures
		{
	        featureZ = maxdepth - DBuffer[x / oversam] * maxdepth / 256;
		    sep = (int)(((long)veyeSep * featureZ) / (featureZ + obsDist));
            // printf("sep: %d\n", sep);
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
	/// ... starting from roughly the center s, going right ...
	lastlinked = -10;           // dummy initial value
	for (x = s; x < vwidth; x++) {
		if ((lookL[x] == x) || (lookL[x] < s)) {
			if (lastlinked == (x - 1))
				color[x] = color[x - 1];
			else {
				color[x] = get_pixel_from_pattern(
				  ((x + poffset) % vmaxsep) / oversam,
				   (y + ((x - s) / vmaxsep) * yShift) % Theight);
			}
		} else {
			color[x] = color[lookL[x]];
			lastlinked = x;     // keep track of the last pixel to be constrained
		}
	}
	/// ... starting from roughly the center s, going left ...
	lastlinked = -10;           // dummy initial value
	for (x = s - 1; x >= 0; x--) {
		if (lookR[x] == x) {
			if (lastlinked == (x + 1))
				color[x] = color[x + 1];
			else {
				color[x] = get_pixel_from_pattern(
				  ((x + poffset) % vmaxsep) / oversam,
				  (y + ((s - x) / vmaxsep + 1) * yShift) % Theight);
			}
		} else {
			color[x] = color[lookR[x]];
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
			// col = color[i];
			/// Get RGB colors for color palette index col and sum them up
			red += SISred[color[i]];
			green += SISgreen[color[i]];
			blue += SISblue[color[i]];
			// red += col.r;
			// green += col.g;
			// blue += col.b;
		}
		// col_rgb_t colrgb = rgb_value(red / oversam, green / oversam, blue / oversam);
		// set_pixel(x / oversam, colrgb);
		// SetSISPixel(x / oversam, colrgb);
		col_rgb_t colrgb = { red / oversam, green / oversam, blue / oversam };
		SIScolorRGB[x / oversam] = colrgb;
	}

	free(lookL);
	free(lookR);
	free(color);
	free(SIScolorRGB);
}

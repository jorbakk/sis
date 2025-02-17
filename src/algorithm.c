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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "defines.h"
#include "sis.h"

/// The biggest and smallest distance in one line (!)
/// of the depth-map. This is just for efficiency.
z_t max_depth, min_depth;

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
}


void
DaddEntry(col_t index, z_t zval)
{
	if (invert)
		zval = SIS_MAX_DEPTH - zval;
	if (zval < min_depth)
		min_depth = zval;
	if (zval > max_depth)
		max_depth = zval;

	if (zvalue[index] == -1) {
		zvalue[index] = zval;
		separation[index] = E * (numerator - zval) / (denominator - zval);
		dz[index] =
		    (double)(denominator - zval) / (double)((E >> 1) * DBufStep);
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
}


void
FreeBuffers(void)
{
	free(DBuffer);
	free(IdentBuffer);
	free(SISBuffer);
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
				for (i = 1; z_limit < (unsigned int)max_depth; i++) {
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
				for (i = 1; z_limit < (unsigned int)max_depth; i++) {
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
}

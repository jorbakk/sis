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

#include "sis.h"
#include "stbimg.h"
#include "tiff.h"

// CURRENT TODO LIST ////////////////////////////////////////////////////////////////////////
//
// 1. Set the image format for input and output images based on the filenames
//    given at the command line or introduce a new command line option.
//    Beware that the input image (depth-map) can be read from stdin.
// 2. Update docs (build process, supported image formats, add input images:
//    depth map and texture, ...)
// 3. Support color palette for the texture image with more than 256 colors,
//    depalettize input images while reading
//
//////////////////////////////////////////////////////////////////////////////////////////////

int ImgFileFormat = SIS_IMGFMT_DFLT;
/* int ImgFileFormat = SIS_IMGFMT_TIFF; */
ind_t Tcolcount;

char *DFileName = NULL;
char *SISFileName = NULL;
char *TFileName = NULL;

col_t *DBuffer = NULL;
col_t *SISBuffer = NULL;
z_t zvalue[SIS_MAX_COLORS + 1];

cmap_t *SISred, *SISgreen, *SISblue;
col_rgb_t *SIScolorRGB;

cmap_t black_value, white_value;
int rand_grey_num, rand_col_num;
col_t white, black;
ind_t Dwidth, Dheight, SISwidth, SISheight, Twidth, Theight;
ind_t E, origin, halfstripwidth, halftriangwidth;
int SIStype, SIScompress, verbose, mark, invert;
char metric;
int resolution;
int debug;
double density;

void (*OpenDFile)(char *DFileName, ind_t * width, ind_t * height);
void (*OpenSISFile)(char *SISFileName, ind_t width, ind_t height, int SIStype);
void (*OpenTFile)(char *TFileName, ind_t * width, ind_t * height);
void (*CloseDFile)(void);
void (*CloseTFile)(ind_t height);
void (*CloseSISFile)(void);
void (*ReadDBuffer)(ind_t r);
col_t(*ReadTPixel) (ind_t r, ind_t c);
void (*WriteSISBuffer)(ind_t r);
void (*WriteSISColorBuffer)(ind_t r);

static char *DefaultDFileName = "in.tif";
static char *DefaultSISFileName = "out.tif";
static char *DefaultTFileName = "texture.tif";
static pos_t DLinePosition, DLineStep;
static ind_t SISLineNumber;
static ind_t DLineNumber;


static void
SetDefaults(void)
{
	DFileName = DefaultDFileName;
	TFileName = DefaultTFileName;
	SISFileName = DefaultSISFileName;
	SIStype = SIS_RANDOM_GREY;
	SISwidth = SISheight = 0;
	algorithm = 4;
	origin = -1;                /* that means, it is set to SISwidth/2 later */
	verbose = 0;
	invert = 0;
	mark = 0;
	metric = 'i';
	resolution = 75;
	oversam = 4;
	E = 0;
	t = 1.0;
	u = 0.67;
	rand_grey_num = 2;
	rand_col_num = SIS_MAX_COLORS;
	density = 0.5;
	SIScompress = 0;
	debug = 0;
}


static void
InitFuncs(void)
{
	OpenDFile = Stb_OpenDFile;
	CloseDFile = Stb_CloseDFile;
	ReadDBuffer = Stb_ReadDBuffer;
	OpenSISFile = Stb_OpenSISFile;
	OpenTFile = Stb_OpenTFile;
	CloseTFile = Stb_CloseTFile;
	CloseSISFile = Stb_CloseSISFile;
	ReadTPixel = Stb_ReadTPixel;
	WriteSISBuffer = Stb_WriteSISBuffer;
	WriteSISColorBuffer = Stb_WriteSISColorBuffer;
	/* switch (ImgFileFormat) { */
	/*     case SIS_IMGFMT_TIFF: */
	/*         OpenDFile = Tiff_OpenDFile; */
	/*         CloseDFile = Tiff_CloseDFile; */
	/*         ReadDBuffer = Tiff_ReadDBuffer; */
	/*         OpenSISFile = Tiff_OpenSISFile; */
	/*         OpenTFile = Tiff_OpenTFile; */
	/*         CloseTFile = Tiff_CloseTFile; */
	/*         CloseSISFile = Tiff_CloseSISFile; */
	/*         ReadTPixel = Tiff_ReadTPixel; */
	/*         WriteSISBuffer = Tiff_WriteSISBuffer; */
	/*         break; */
	/*     default: */
	/*         OpenDFile = Stb_OpenDFile; */
	/*         CloseDFile = Stb_CloseDFile; */
	/*         ReadDBuffer = Stb_ReadDBuffer; */
	/*         OpenSISFile = Stb_OpenSISFile; */
	/*         OpenTFile = Stb_OpenTFile; */
	/*         CloseTFile = Stb_CloseTFile; */
	/*         CloseSISFile = Stb_CloseSISFile; */
	/*         ReadTPixel = Stb_ReadTPixel; */
	/*         WriteSISBuffer = Stb_WriteSISBuffer; */
	/*         break; */
	/* } */
}


static void
InitVars(void)
{
	int i;

	inner_propagate_c = 0;
	outer_propagate_c = 0;
	forwards_obscure_c = 0;
	backwards_obscure_c = 0;
	metric = 'i';
	if (E == 0)
		E = metric2pixel(22, resolution);
	halfstripwidth = E * t / (2 * (1 + t));
	halftriangwidth = SISwidth / 75;
	if (!halftriangwidth)
		halftriangwidth = 4;
	DLineStep = (double)Dheight / (double)SISheight;
	DLinePosition = 0.0;
	if (origin == -1)
		origin = SISwidth >> 1;

	switch (SIStype) {
	case SIS_RANDOM_GREY:
		SISred[0] = SISgreen[0] = SISblue[0] = white_value;
		white = 0;
		for (i = 1; i < rand_grey_num; i++) {
			SISred[i] = SISgreen[i] = SISblue[i] =
			    i * (float)SIS_MAX_CMAP / (float)rand_grey_num;
		}
		break;
	case SIS_RANDOM_COLOR:
		for (i = 0; i < rand_col_num; i++) {
			SISred[i] = rand() / (RAND_MAX / SIS_MAX_CMAP);
			SISgreen[i] = rand() / (RAND_MAX / SIS_MAX_CMAP);
			SISblue[i] = rand() / (RAND_MAX / SIS_MAX_CMAP);
		}
		break;
	}
	SISred[SIS_MAX_COLORS] = SISgreen[SIS_MAX_COLORS] = SISblue[SIS_MAX_COLORS]
	    = black_value;
	black = SIS_MAX_COLORS;
}


static void
print_warnings(void)
{
	if (algorithm < 4 && oversam > 1) {
		fprintf(stderr, "warning: oversampling is currently only available for algorithm 4\n");
	}
	if (algorithm == 4 && (u != 0.67 || t != 1.0)) {
		fprintf(stderr, "warning: near- and far plane adjustment is currently not available for algorithm 4\n");
	}
	if (algorithm == 4 && SIStype != SIS_TEXT_MAP) {
		fprintf(stderr, "warning: random dot stereograms are currently not available for algorithm 4. You need to specify a texture map image, aborting!\n");
		exit(1);
	}
	if (algorithm == 4 && mark) {
		fprintf(stderr, "warning: eye markers are currently not available for algorithm 4\n");
	}
	if (algorithm == 4 && origin != (SISwidth >> 1)) {
		fprintf(stderr, "warning: setting origin is currently not available for algorithm 4\n");
	}
	if (algorithm == 4 && verbose == 1) {
		fprintf(stderr, "warning: verbose output is currently limited for algorithm 4\n");
	}
	// if (algorithm == 4 && SISwidth != Dwidth) {
		// fprintf(stderr, "warning: setting SIS width is currently not available for algorithm 4\n");
	// }
}


static void
print_message_header(void)
{
	printf("\n  DEPTH FILE:     %s (%ldx%ld)\n", DFileName, Dwidth, Dheight);
	printf("  SIS FILE:       %s (%ldx%ld)\n\n", SISFileName, SISwidth,
	       SISheight);
	if (SIStype == SIS_TEXT_MAP) {
		printf("  ... using texture-map: %s\n\n\n", TFileName);
	}
	printf("  ----    --- PROPAGATE ---    ---- OBSCURE ----     --- DEPTH ---\n");
	printf("  Line       inner    outer        forw    backw      min      max\n");
}


static void
print_statistics(void)
{
	printf("  %4ld    %8ld %8ld    %8ld %8ld   %6ld   %6ld\r", SISLineNumber + 1,
	       inner_propagate_c, outer_propagate_c,
	       forwards_obscure_c, backwards_obscure_c,
	       min_depth_in_row, max_depth_in_row);
	if (fflush(stdout)) {
		printf("stdout didn't flush\n");
		verbose = 0;
	}
}


static void
print_summary(void)
{
	printf("  Depth map values [min,max]: [%ld, %ld]\n", min_depth, max_depth);
	if (SIStype == SIS_TEXT_MAP) {
		printf("  Texture unique color count: %ld\n", Tcolcount);
	}
}


int
main(int argc, char **argv)
{
	SISred = (cmap_t *) calloc(SIS_MAX_COLORS + 1, sizeof(cmap_t));
	SISgreen = (cmap_t *) calloc(SIS_MAX_COLORS + 1, sizeof(cmap_t));
	SISblue = (cmap_t *) calloc(SIS_MAX_COLORS + 1, sizeof(cmap_t));

	SetDefaults();
	get_options(argc, argv);
	InitFuncs();
	OpenDFile(DFileName, &Dwidth, &Dheight);
	if (!SISwidth && !SISheight) {
		SISwidth = Dwidth;
		SISheight = Dheight;
	}
	if (!SISwidth)
		SISwidth = SISheight * (float)Dwidth / (float)Dheight;
	if (!SISheight)
		SISheight = SISwidth * (float)Dheight / (float)Dwidth;
	if (SIStype == SIS_TEXT_MAP)
		OpenTFile(TFileName, &Twidth, &Theight);
	InitAlgorithm();
	InitBuffers();
	InitVars();
	OpenSISFile(SISFileName, SISwidth, SISheight, SIStype);

	print_warnings();
	if (verbose) {
		print_message_header();
	}

	max_depth = SIS_MIN_DEPTH;
	min_depth = SIS_MAX_DEPTH;
	for (SISLineNumber = 0; SISLineNumber < SISheight; SISLineNumber++) {
		DLineNumber = (int)DLinePosition;
		DLinePosition += DLineStep;
		max_depth_in_row = SIS_MIN_DEPTH;
		min_depth_in_row = SIS_MAX_DEPTH;

		ReadDBuffer(DLineNumber);            /// Read in one line of depth-map

		if (algorithm < 4) {
			CalcIdentLine();                     /// My SIS-algorithm
			FillSISBuffer(SISLineNumber);        /// Fill in the right color indices,
			                                     /// according to the SIS-type
		} else {
			asteer(SISLineNumber);               /// Andrew Steer's SIS-algorithm
		}
		// WriteSISBuffer(SISLineNumber);    /// Write one line of output
		WriteSISColorBuffer(SISLineNumber);  /// Write one line of output
		if (verbose)
			print_statistics();
	}
	if (verbose) {
		puts("\n");
		print_summary();
	}
	CloseDFile();
	if (SIStype == SIS_TEXT_MAP)
		CloseTFile(Theight);
	CloseSISFile();
	FreeBuffers();
	free(SISred);
	free(SISgreen);
	free(SISblue);
	return EXIT_SUCCESS;
}

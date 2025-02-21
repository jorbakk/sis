#ifndef SIS_INCLUDED
#define SIS_INCLUDED
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

#include <stdint.h>

#define SIS_RANDOM_GREY  1
#define SIS_RANDOM_COLOR 2
#define SIS_TEXT_MAP     3

#define SIS_IMGFMT_DFLT  0
#define SIS_IMGFMT_TIFF  1

#define SIS_MAX_COLORS   0xffff
#define SIS_MAX_CMAP     0xffff
#define SIS_MAX_DEPTH    0xffff
#define SIS_MIN_DEPTH    0x0

typedef uint32_t col_t;
typedef uint16_t cmap_t;
/// MAX(z_t) needs to be greater than SIS_MAX_DEPTH
typedef long z_t;
typedef long ind_t;
typedef float pos_t;

typedef struct {
	int r, g, b;
} col_rgb_t;

/*
   Interface to bitmap handlers (stbimg.c, tiff.c):
   */

extern int ImgFileFormat;
extern char *DFileName;
extern char *SISFileName;
extern char *TFileName;
extern col_t *DBuffer;
extern col_t *SISBuffer;
extern int SIStype, SIScompress;

extern z_t zvalue[SIS_MAX_COLORS + 1];
extern cmap_t *SISred;
extern cmap_t *SISgreen;
extern cmap_t *SISblue;
extern col_rgb_t *SIScolorRGB;
extern ind_t Dwidth, Dheight, SISwidth, SISheight, Twidth, Theight;
extern cmap_t white_value, black_value;

/*
   Interface to get_opt.c:
   */
extern ind_t E, origin;
extern int verbose, mark, debug, algorithm, invert;
extern int rand_grey_num, rand_col_num;
extern double t, u;
extern double density;
extern char metric;
extern int resolution;

void get_options(int argc, char **argv);
ind_t metric2pixel(int metric_val, int resolution);

/*
   Interface to algorithm.c:
   */
extern z_t min_depth, max_depth;
extern col_t black, white;
extern ind_t halfstripwidth, halftriangwidth;
extern long forwards_obscure_c, backwards_obscure_c;
extern long inner_propagate_c, outer_propagate_c;

extern col_t(*ReadTPixel) (ind_t r, ind_t c);
extern void (*WriteSISBuffer)(ind_t r);
extern void (*WriteSISColorBuffer)(ind_t r);
void InitAlgorithm(void);
void DaddEntry(col_t index, z_t zval);
void InitBuffers(void);
void FreeBuffers(void);
void FillSISBuffer(ind_t LineNumber);
void CalcIdentLine(void);
void asteer(ind_t LineNumber);

#endif     /// SIS_INCLUDED

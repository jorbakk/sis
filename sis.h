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
#include <stdbool.h>

#define SIS_RANDOM_GREY  1
#define SIS_RANDOM_COLOR 2
#define SIS_TEXT_MAP     3

#define SIS_IMGFMT_DFLT  0
// #define SIS_IMGFMT_TIFF  1

#define SIS_MAX_COLORS   0xffff    /// Max index in color palette
#define SIS_MAX_CMAP     0xffff    /// Max value for color component (gray, r, g, b)
#define SIS_MAX_DEPTH    0xffff    /// Max possible pixel value in the depth map image
#define SIS_MIN_DEPTH    0x0       /// Min possible pixel value in the depth map image

#define SIS_MIN_ALGO     1
#define SIS_MAX_ALGO     4


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
   Interface to bitmap handlers (stbimg.c):
   */

extern int ImgFileFormat;
extern char *DFileName;
extern char *SISFileName;
extern char *TFileName;
extern int SIStype;
/// Color palettes for depth and sis image colors (from texture or random dots)
extern col_t *DBuffer;

extern z_t zvalue[SIS_MAX_COLORS + 1];
extern cmap_t *SISred;
extern cmap_t *SISgreen;
extern cmap_t *SISblue;
extern col_rgb_t *SIScolorRGB;
extern ind_t Dwidth, Dheight, SISwidth, SISheight, Twidth, Theight, Tcolcount;
extern cmap_t white_value, black_value;

extern void (*OpenDFile)(char *DFileName, ind_t * width, ind_t * height);
extern void (*OpenSISFile)(char *SISFileName, ind_t width, ind_t height, int SIStype);
extern void (*OpenTFile)(char *TFileName, ind_t * width, ind_t * height);
extern void (*CloseDFile)(void);
extern void (*CloseTFile)(ind_t height);
extern void (*CloseSISFile)(void);
extern void (*ReadDBuffer)(ind_t r);
extern col_t(*ReadTPixel) (ind_t r, ind_t c);
// void (*WriteSISBuffer)(ind_t r);
extern void (*WriteSISColorBuffer)(ind_t r);
extern unsigned char *(*GetDFileBuffer)(void);
extern unsigned char *(*GetTFileBuffer)(void);
extern unsigned char *(*GetSISFileBuffer)(void);

extern char *DefaultDFileName;
extern char *DefaultSISFileName;
extern char *DefaultTFileName;
extern pos_t DLinePosition, DLineStep;
extern ind_t SISLineNumber;
extern ind_t DLineNumber;

void SetDefaults(void);
void InitFuncs(void);

/*
   Interface to get_opt.c:
   */
extern ind_t eye_dist, origin;
extern int verbose, debug, algorithm, invert;
extern bool mark;
extern int rand_grey_num, rand_col_num;
extern double t, u;
extern double density;
extern char metric;
extern int resolution;
extern int oversam;

void get_options(int argc, char **argv);
void init_all(int argc, char **argv);
void render_sis(void);
void finish_all(void);
void show_statistics(void);
ind_t metric2pixel(int metric_val, int resolution);

/*
   Interface to algorithm.c:
   */
extern z_t min_depth_in_row, max_depth_in_row, min_depth, max_depth;
extern col_t black, white;
extern ind_t halfstripwidth, halftriangwidth;
extern long forwards_obscure_c, backwards_obscure_c;
extern long inner_propagate_c, outer_propagate_c;

extern col_t(*ReadTPixel) (ind_t r, ind_t c);
// extern void (*WriteSISBuffer)(ind_t r);
extern void (*WriteSISColorBuffer)(ind_t r);
void InitAlgorithm(void);
void DaddEntry(col_t index, z_t zval);
void AllocBuffers(void);
void FreeBuffers(void);
void InitSISBuffer(ind_t LineNumber);
void FillRGBBuffer(ind_t LineNumber);
void CalcIdentLine(void);
void asteer(ind_t LineNumber);

#endif     /// SIS_INCLUDED

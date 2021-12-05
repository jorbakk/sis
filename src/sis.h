/*
 * Copyright 1995, 2021 Jörg Bakker
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


typedef unsigned char col_t;
typedef unsigned short cmap_t;
typedef long z_t; /* for portability: MAX(z_t) > SIS_MAX_DEPTH */
typedef long ind_t;
typedef float pos_t;

/*
   Interface to bitmap handlers (tga.c, tiff.c):
   */

extern char *DFileName;
extern char *SISFileName;
extern char *TFileName;
extern col_t *DBuffer;
extern col_t *SISBuffer;
extern int SIStype, SIScompress;

extern z_t zvalue[SIS_MAX_COLORS+1];
extern cmap_t *SISred;
extern cmap_t *SISgreen;
extern cmap_t *SISblue;
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

void get_options (int argc, char **argv);
ind_t metric2pixel (int metric_val, int resolution);

/*
   Interface to algorith.c:
   */
extern z_t min_depth, max_depth;
extern col_t black, white;
extern ind_t halfstripwidth, halftriangwidth;
extern long forwards_obscure_c, backwards_obscure_c;
extern long inner_propagate_c, outer_propagate_c;

extern col_t (*ReadTPixel) (ind_t r, ind_t c);
extern void (*WriteSISBuffer) (ind_t r);
void InitAlgorithm (void);
void DaddEntry (col_t index, z_t zval);
void InitBuffers (void);
void FreeBuffers (void);
void FillSISBuffer (ind_t LineNumber);
void CalcIdentLine (void);

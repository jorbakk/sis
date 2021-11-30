/*
 * Copyright 1995 Joerg Bakker
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  The author makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
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

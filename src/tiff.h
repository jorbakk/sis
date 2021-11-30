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


void Tiff_OpenDFile (char *DFileName, ind_t *width, ind_t *height);
void Tiff_OpenSISFile (char *SISFileName, ind_t width, ind_t height,
    int SIStype);
void Tiff_OpenTFile (char *TFileName, ind_t *width, ind_t *height);

void Tiff_ReadDBuffer (ind_t r);
void Tiff_WriteSISBuffer (ind_t r);
col_t Tiff_ReadTPixel (ind_t r, ind_t c);

void Tiff_CloseDFile (void);
void Tiff_CloseTFile (void);
void Tiff_CloseSISFile (void);

/*
 * Copyright 1995, 2021, 2022 Jörg Bakker
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

#include "sis.h"

void Tiff_OpenDFile (char *DFileName, ind_t *width, ind_t *height);
void Tiff_OpenSISFile (char *SISFileName, ind_t width, ind_t height,
    int SIStype);
void Tiff_OpenTFile (char *TFileName, ind_t *width, ind_t *height);

void Tiff_ReadDBuffer (ind_t r);
void Tiff_WriteSISBuffer (ind_t r);
col_t Tiff_ReadTPixel (ind_t r, ind_t c);

void Tiff_CloseDFile (void);
void Tiff_CloseTFile (ind_t height);
void Tiff_CloseSISFile (void);

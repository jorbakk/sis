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
#include <tiffio.h>
#include "defines.h"
#include "sis.h"
#include "tiff.h"

static TIFF *inpic_p, *outpic_p, *texpic_p;
static cmap_t *Dredcmap, *Dgreencmap, *Dbluecmap;
static col_t **Tread_buf;
static ind_t cur_Dread = -1, cur_Tread = -1;
static unsigned short phm;


void
Tiff_OpenDFile(char *DFileName, ind_t * width, ind_t * height)
{
	unsigned short int bps;

	black_value = 0;
	white_value = SIS_MAX_CMAP;

	if (!(inpic_p = TIFFOpen(DFileName, "r")))
		exit(1);
	if (debug) {
		TIFFPrintDirectory(inpic_p, stdout, TIFFPRINT_NONE);
	}
	TIFFGetField(inpic_p, TIFFTAG_IMAGEWIDTH, width);
	TIFFGetField(inpic_p, TIFFTAG_IMAGELENGTH, height);
	TIFFGetField(inpic_p, TIFFTAG_BITSPERSAMPLE, &bps);
	if (bps != 8) {
		fprintf(stderr, "BITSPERSAMPLE != 8\n");
		exit(1);
	}

	TIFFGetField(inpic_p, TIFFTAG_PHOTOMETRIC, &phm);
	switch (phm) {
	case PHOTOMETRIC_PALETTE:
		TIFFGetField(inpic_p, TIFFTAG_COLORMAP, &Dredcmap, &Dgreencmap,
		             &Dbluecmap);
		break;
	case PHOTOMETRIC_MINISWHITE:
	case PHOTOMETRIC_MINISBLACK:
		break;
	default:
		fprintf(stderr, "Don't support this format for depthmap file.\n");
		exit(1);
	}
}


void
Tiff_OpenSISFile(char *SISFileName, ind_t width, ind_t height, int SIStype)
{
	if (!(outpic_p = TIFFOpen(SISFileName, "w")))
		exit(1);

	TIFFSetField(outpic_p, TIFFTAG_IMAGEWIDTH, width);
	TIFFSetField(outpic_p, TIFFTAG_IMAGELENGTH, height);
	TIFFSetField(outpic_p, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(outpic_p, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField(outpic_p, TIFFTAG_ROWSPERSTRIP, 12);
	TIFFSetField(outpic_p, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(outpic_p, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(outpic_p, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
	TIFFSetField(outpic_p, TIFFTAG_COLORMAP, SISred, SISgreen, SISblue);
	if (SIScompress)
		TIFFSetField(outpic_p, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
}


void
Tiff_OpenTFile(char *TFileName, ind_t * width, ind_t * height)
{
	unsigned short bps, phm;

	if (!(texpic_p = TIFFOpen(TFileName, "r")))
		exit(1);
	if (debug) {
		TIFFPrintDirectory(texpic_p, stdout, TIFFPRINT_NONE);
	}
	TIFFGetField(texpic_p, TIFFTAG_IMAGEWIDTH, width);
	TIFFGetField(texpic_p, TIFFTAG_IMAGELENGTH, height);
	TIFFGetField(texpic_p, TIFFTAG_BITSPERSAMPLE, &bps);
	if (bps != 8) {
		fprintf(stderr, "BITSPERSAMPLE != 8\n");
		exit(1);
	}
	TIFFGetField(texpic_p, TIFFTAG_PHOTOMETRIC, &phm);
	if (phm == PHOTOMETRIC_PALETTE) {
		TIFFGetField(texpic_p, TIFFTAG_COLORMAP, &SISred, &SISgreen, &SISblue);
	} else {
		fprintf(stderr, "No colormap in texture file.\n");
		exit(1);
	}
	/* Allocate buffer for reading the texture image */
	if (!(Tread_buf = (col_t **) calloc(*height, sizeof(col_t *)))) {
		fprintf(stderr, "Failed to allocate texture readbuf.\n");
		exit(1);
	}
	for (int r = 0; r < *height; ++r) {
		// TODO check if we need to allocate Tread_buf or if TIFFReadScanline does the job
		if (!(Tread_buf[r] = (col_t *) _TIFFmalloc(TIFFScanlineSize(texpic_p)))) {
			fprintf(stderr, "Failed to allocate texture readbuf.\n");
			exit(1);
		}
		if (TIFFReadScanline(texpic_p, Tread_buf[r], r, 0) != 1) {
			fprintf(stderr, "Reading texture scan line failed\n");
			exit(1);
		}
	}
}


void
Tiff_ReadDBuffer(ind_t r)
{
	ind_t i;
	z_t zval = 0;

	while (cur_Dread < r) {
		cur_Dread++;
		TIFFReadScanline(inpic_p, DBuffer, cur_Dread, 0);
	}
	for (i = 0; i < Dwidth; i++) {
		switch (phm) {
		case PHOTOMETRIC_PALETTE:
			zval =
			    (11 * Dredcmap[DBuffer[i]] +
			     16 * Dgreencmap[DBuffer[i]] + 5 * Dbluecmap[DBuffer[i]]) >> 5;
			break;
		case PHOTOMETRIC_MINISWHITE:
			zval = SIS_MAX_DEPTH - (DBuffer[i] << 8);
			break;
		case PHOTOMETRIC_MINISBLACK:
			zval = DBuffer[i] << 8;
			break;
		}
		DaddEntry(DBuffer[i], zval);
	}
}


void
Tiff_WriteSISBuffer(ind_t r)
{
	TIFFWriteScanline(outpic_p, SISBuffer, r, 0);
}


col_t
Tiff_ReadTPixel(ind_t r, ind_t c)
{
	if (cur_Tread != r) {
		cur_Tread = r;
	}
	return Tread_buf[r][c];
}


void
Tiff_CloseDFile(void)
{
	TIFFClose(inpic_p);
}


void
Tiff_CloseTFile(ind_t height)
{
	/* There is still a memleak, but at least no double free */
	for (ind_t r = 0; r < height; ++r) {
		_TIFFfree(Tread_buf[r]);
	}
	free(Tread_buf);
	/* TIFFClose (texpic_p); */
}


void
Tiff_CloseSISFile(void)
{
	TIFFClose(outpic_p);
}

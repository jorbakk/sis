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
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "3rd-party/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "3rd-party/stb_image_write.h"
#define STB_DS_IMPLEMENTATION
#include "3rd-party/stb_ds.h"

#include "sis.h"
#include "stbimg.h"

static unsigned char *inpic_p, *outpic_buf_p, *texpic_p;
static ind_t outpic_width = 0, outpic_height = 0;
static col_t **Tread_buf;
/* static ind_t cur_Dread=-1, */
static ind_t cur_Tread = -1;
static const int SISChannelCount = 3;


void
Stb_OpenDFile(char *DFileName, ind_t *width, ind_t *height)
{
	int channel_count = 0, desired_channel_count = 1;
	if (! (inpic_p = (unsigned char *)stbi_load(DFileName, (int *)width, (int *)height,
	                                &channel_count, desired_channel_count)))
		exit(1);
	if (channel_count != desired_channel_count) {
		fprintf(stderr,
		        "Input depth map image has more than one color channel, it should be a grayscale image\n");
		// TODO: check if we can continue with the desired channel count
		exit(1);
	}
	black_value = 0;
	white_value = SIS_MAX_CMAP;
}


void
Stb_OpenSISFile(char *SISFileName, ind_t width, ind_t height, int SIStype)
{
	outpic_width = width;
	outpic_height = height;
	// Allocate buffer for the output image data that can be directly written with stb_image_write()
	if (! (outpic_buf_p = (unsigned char *)calloc(height * width, SISChannelCount))) {
		fprintf(stderr, "No space for output image buffer.\n");
		exit(1);
	}
}


void
Stb_OpenTFile(char *TFileName, ind_t * width, ind_t * height)
{
	int channel_count = 0, desired_channel_count = 3;
	if (!(texpic_p = (unsigned char *)stbi_load(TFileName, (int *)width, (int *)height,
	                                &channel_count, desired_channel_count)))
		exit(1);
	if (channel_count != desired_channel_count) {
		fprintf(stderr,
		        "Input texture map image must have three color channels\n");
		/// TODO: check if we can continue if channel count is not equal
		///       to the desired channel count
		exit(1);
	}
	/// Copy the interleaved color values and populate the texture image
	/// planes SISred, SISgreen, SISblue and the image color index buffer
	/// Tread_buf by creating a color map from the interleaved stb_image data.
	struct {
		uint32_t key;
		col_t value;
	} *colmap = NULL;
	const col_t default_value = -1;
	hmdefault(colmap, default_value);
	col_t col_idx = 0;
	if (!(Tread_buf = (col_t **) calloc(*height, sizeof(col_t *)))) {
		fprintf(stderr, "Failed to allocate texture readbuf.\n");
		exit(1);
	}
	for (ind_t r = 0; r < *height; ++r) {
		if (!(Tread_buf[r] = (col_t *) calloc(*width, sizeof(col_t)))) {
			fprintf(stderr, "Failed to allocate texture readbuf.\n");
			exit(1);
		}
		// TODO We also need to downsample the texture image to 256 different
		// colors, as col_t is unsigned char and SIS_MAX_COLORS is 0xff.
		// However, currently the tiff implementation rejects texture images
		// with more than 256 different colors (BITSPERSAMPLE != 8). We could
		// raise this limit for both implementations (tiff and stb_image) or
		// remove tiff and fix it with stb_image.
		for (ind_t c = 0; c < *width; ++c) {
			ind_t row_pos = r * (*width) * channel_count;
			ind_t col_base_pos = c * channel_count;
			uint32_t col_rgb =
			    (uint32_t) (texpic_p[row_pos + col_base_pos + 0]) |
			    (uint32_t) (texpic_p[row_pos + col_base_pos + 1]) << 8 |
			    (uint32_t) (texpic_p[row_pos + col_base_pos + 2]) << 16;
			col_t current_col_idx = hmget(colmap, col_rgb);
			if (current_col_idx == default_value) {
				hmput(colmap, col_rgb, col_idx);
				current_col_idx = col_idx;
				col_idx++;
			}
			Tread_buf[r][c] = current_col_idx;
			SISred[current_col_idx] = texpic_p[row_pos + col_base_pos + 0];
			SISgreen[current_col_idx] = texpic_p[row_pos + col_base_pos + 1];
			SISblue[current_col_idx] = texpic_p[row_pos + col_base_pos + 2];

		}
	}
	hmfree(colmap);
	/// Number of unique colors is col_idx + black
	// printf("texture unique color count: %d\n", col_idx + 1);
	Tcolcount = col_idx + 1;
}


unsigned char *
Stb_GetDFileBuffer(void)
{
	return inpic_p;
}


unsigned char *
Stb_GetTFileBuffer(void)
{
	return texpic_p;
}


void
Stb_ReadDBuffer(ind_t r)
{
	z_t zval = 0;
	for (ind_t c = 0; c < Dwidth; c++) {
		DBuffer[c] = inpic_p[r * Dwidth + c];
		zval = DBuffer[c] << 8;
		DaddEntry(DBuffer[c], zval);
	}
}


// void
// Stb_WriteSISBuffer(ind_t r)
// {
	// // Write each line of the generated SIS subsequently to the output file / buffer
	// for (ind_t c = 0; c < outpic_width; c++) {
		// ind_t row_pos = r * outpic_width * SISChannelCount;
		// ind_t col_base_pos = c * SISChannelCount;
		// // Write all color components of the ouput image. The color is taken from the color palette
		// // and the index into the color palette is taken from SISBuffer.
		// outpic_buf_p[row_pos + col_base_pos + 0] = SISred[SISBuffer[c]];
		// outpic_buf_p[row_pos + col_base_pos + 1] = SISgreen[SISBuffer[c]];
		// outpic_buf_p[row_pos + col_base_pos + 2] = SISblue[SISBuffer[c]];
	// }
// }


void
Stb_WriteSISColorBuffer(ind_t r)
{
	// Write each line of the generated SIS subsequently to the output file / buffer
	for (ind_t c = 0; c < outpic_width; c++) {
		ind_t row_pos = r * outpic_width * SISChannelCount;
		ind_t col_base_pos = c * SISChannelCount;
		// Write all color components of the ouput image. The color is taken from the color palette
		// and the index into the color palette is taken from SISBuffer.
		outpic_buf_p[row_pos + col_base_pos + 0] = SIScolorRGB[c].r;
		outpic_buf_p[row_pos + col_base_pos + 1] = SIScolorRGB[c].g;
		outpic_buf_p[row_pos + col_base_pos + 2] = SIScolorRGB[c].b;
	}
}


// Return the index into the color palette of the pixel with coordinates (r, c)
// in the texture image
col_t
Stb_ReadTPixel(ind_t r, ind_t c)
{
	// TODO is cur_Tread actually used in stbimg.c and tiff.c?
	if (cur_Tread != r) {
		cur_Tread = r;
	}
	return Tread_buf[r][c];
}


void
Stb_CloseDFile(void)
{
	stbi_image_free(inpic_p);
}


void
Stb_CloseTFile(ind_t height)
{
	for (ind_t r = 0; r < height; ++r) {
		free(Tread_buf[r]);
	}
	free(Tread_buf);
	stbi_image_free(texpic_p);
}


void
Stb_CloseSISFile(void)
{
	// TODO write to other image formats
	stbi_write_png(SISFileName,
	               outpic_width,
	               outpic_height,
	               SISChannelCount,
	               outpic_buf_p, outpic_width * SISChannelCount);
	free(outpic_buf_p);
}

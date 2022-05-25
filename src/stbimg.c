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


#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../3rd-party/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../3rd-party/stb_image_write.h"

#include "defines.h"
#include "sis.h"
#include "stbimg.h"

static unsigned char *inpic_p, *outpic_buf_p, *texpic_p;
static ind_t outpic_width = 0, outpic_height = 0;
static col_t **Tread_buf;
/* static ind_t cur_Dread=-1, */
static ind_t cur_Tread=-1;
static const int SISChannelCount = 3;

void Stb_OpenDFile (char *DFileName, ind_t *width, ind_t *height)
{
    int channel_count = 0, desired_channel_count = 1;
    if (!(inpic_p = (unsigned char*)stbi_load(DFileName, (int*)width, (int*)height, &channel_count, desired_channel_count)))
        exit(1);
    if (channel_count != desired_channel_count) {
        fprintf (stderr, "Input depth map image has more than one color channel, it should be a grayscale image\n");
        // TODO: check if we can continue with the desired channel count
        exit(1);
    }
    black_value = 0; white_value = SIS_MAX_CMAP;
}

void Stb_OpenSISFile (char *SISFileName, ind_t width, ind_t height, int SIStype)
{
    outpic_width = width;
    outpic_height = height;
    // Allocate buffer for the output image data that can be directly written with stb_image_write()
    if (!(outpic_buf_p = (unsigned char *) calloc(height * width, SISChannelCount))) {
        fprintf(stderr, "No space for output image buffer.\n");
        exit(1);
    }
}

void Stb_OpenTFile (char *TFileName, ind_t *width, ind_t *height)
{
    int channel_count = 0, desired_channel_count = 3;
    if (!(texpic_p = (unsigned char*)stbi_load(TFileName, (int*)width, (int*)height, &channel_count, desired_channel_count)))
        exit(1);
    if (channel_count != desired_channel_count) {
        fprintf (stderr, "Input texture map image must have three color channels\n");
        // TODO: check if we can continue with the desired channel count
        exit(1);
    }
    // TODO copy the interleaved color values and populate the texture image
    // planes SISred, SISgreen, SISblue and the image color index buffer
    // Tread_buf by creating a color map from the interleaved stb_image data.
    for (ind_t r = 0; r < *width; ++r) {
        for (ind_t c = 0; c < *height; ++c) {
        }
    }
}

void Stb_ReadDBuffer (ind_t r)
{
    z_t zval = 0;
    for (ind_t c = 0; c < Dwidth; c++) {
        zval = inpic_p[r * Dwidth + c] << 8;
        DaddEntry (DBuffer[c], zval);
    }
}

void Stb_WriteSISBuffer (ind_t r)
{
    // Write each line of the generated SIS subsequently to the output file / buffer
    // TODO Convert from palette to interleaved pixel format and store the
    // interleaved pixels in outpic_buf_p ... ?!
    for (ind_t c = 0; c < outpic_width; c++) {
        ind_t row_pos = r * outpic_width * SISChannelCount;
        ind_t col_base_pos = c * SISChannelCount;
        outpic_buf_p[row_pos + col_base_pos + 0] = SISBuffer[c];
    }
}

col_t Stb_ReadTPixel (ind_t r, ind_t c)
{
    if (cur_Tread != r) {
        cur_Tread = r;
    }
    return Tread_buf[r][c];
}

void Stb_CloseDFile (void)
{
    stbi_image_free(inpic_p);
}

void Stb_CloseTFile (ind_t height)
{
    free(texpic_p);
}

void Stb_CloseSISFile (void)
{
    // TODO write to other image formats
    stbi_write_png(SISFileName, outpic_width, outpic_height, SISChannelCount, outpic_buf_p, outpic_width * SISChannelCount);
    free(outpic_buf_p);
}

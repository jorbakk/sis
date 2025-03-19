#include <stddef.h>

#include "sis.h"


int ImgFileFormat = SIS_IMGFMT_DFLT;
/* int ImgFileFormat = SIS_IMGFMT_TIFF; */
ind_t Tcolcount;

char *DFileName = NULL;
char *SISFileName = NULL;
char *TFileName = NULL;

col_t *DBuffer = NULL;
z_t zvalue[SIS_MAX_COLORS + 1];

cmap_t *SISred, *SISgreen, *SISblue;
col_rgb_t *SIScolorRGB;

cmap_t black_value, white_value;
int rand_grey_num, rand_col_num;
col_t white, black;
ind_t Dwidth, Dheight, SISwidth, SISheight, Twidth, Theight;
ind_t eye_dist, origin, halfstripwidth, halftriangwidth;
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
// void (*WriteSISBuffer)(ind_t r);
void (*WriteSISColorBuffer)(ind_t r);
unsigned char *(*GetDFileBuffer)(void);
unsigned char *(*GetTFileBuffer)(void);

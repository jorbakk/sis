#include <stdlib.h>
#include <string.h>

#include "sis.h"
#include "stbimg.h"


int ImgFileFormat = SIS_IMGFMT_DFLT;
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
int SIStype, SIScompress, verbose, invert;
bool mark;
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
unsigned char *(*GetSISFileBuffer)(void);

char *DefaultDFileName = "depthmaps/flowers.png";
char *DefaultSISFileName = "/tmp/out.png";
char *DefaultTFileName = "textures/clover.png";
pos_t DLinePosition, DLineStep;
ind_t SISLineNumber;
ind_t DLineNumber;


void
SetDefaults(void)
{
	DFileName = DefaultDFileName;
	TFileName = DefaultTFileName;
	SISFileName = DefaultSISFileName;
	// SIStype = SIS_RANDOM_GREY;
	SIStype = SIS_TEXT_MAP;
	SISwidth = SISheight = 0;
	algorithm = 2;
	origin = -1;                /* that means, it is set to SISwidth/2 later */
	verbose = 0;
	invert = 0;
	mark = 1;
	metric = 'i';
	resolution = 75;
	oversam = 4;
	eye_dist = 300;
	t = 1.0;
	u = 0.67;
	rand_grey_num = 2;
	rand_col_num = SIS_MAX_COLORS;
	density = 0.5;
	debug = 0;
}


void
InitFuncs(void)
{
	OpenDFile = Stb_OpenDFile;
	CloseDFile = Stb_CloseDFile;
	ReadDBuffer = Stb_ReadDBuffer;
	OpenSISFile = Stb_OpenSISFile;
	OpenTFile = Stb_OpenTFile;
	CloseTFile = Stb_CloseTFile;
	CloseSISFile = Stb_CloseSISFile;
	ReadTPixel = Stb_ReadTPixel;
	// WriteSISBuffer = Stb_WriteSISBuffer;
	WriteSISColorBuffer = Stb_WriteSISColorBuffer;
	GetDFileBuffer = Stb_GetDFileBuffer;
	GetTFileBuffer = Stb_GetTFileBuffer;
	GetSISFileBuffer = Stb_GetSISFileBuffer;
}


void
InitVars(void)
{
	int i;

	inner_propagate_c = 0;
	outer_propagate_c = 0;
	forwards_obscure_c = 0;
	backwards_obscure_c = 0;
	metric = 'i';
	if (eye_dist == 0) {
		eye_dist = metric2pixel(22, resolution);
	}
	if (algorithm < 4) {
		halfstripwidth = eye_dist * t / (2 * (1 + t));
	} else {
		halfstripwidth = (eye_dist / oversam) * t / (2 * (1 + t));
	}
	halftriangwidth = SISwidth / 75;
	if (!halftriangwidth)
		halftriangwidth = 4;
	DLineStep = (double)Dheight / (double)SISheight;
	DLinePosition = 0.0;
	/// Set the original default value for origin in case of algo #4,
	/// it is not in the center of the image because some artifacts
	/// in the background appear
	if (origin == -1 && algorithm < 4) {
		origin = SISwidth >> 1;
	}

	switch (SIStype) {
	case SIS_RANDOM_GREY:
		SISred[0] = SISgreen[0] = SISblue[0] = white_value;
		white = 0;
		for (i = 1; i < rand_grey_num; i++) {
			SISred[i] = SISgreen[i] = SISblue[i] =
			    i * (float)SIS_MAX_CMAP / (float)rand_grey_num;
		}
		break;
	case SIS_RANDOM_COLOR:
		for (i = 0; i < rand_col_num; i++) {
			SISred[i] = rand() / (RAND_MAX / SIS_MAX_CMAP);
			SISgreen[i] = rand() / (RAND_MAX / SIS_MAX_CMAP);
			SISblue[i] = rand() / (RAND_MAX / SIS_MAX_CMAP);
		}
		break;
	}
	SISred[SIS_MAX_COLORS] = SISgreen[SIS_MAX_COLORS] = SISblue[SIS_MAX_COLORS]
	    = black_value;
	black = SIS_MAX_COLORS;
}


void
init_sis(int argc, char **argv)
{
	SISred = (cmap_t *) calloc(SIS_MAX_COLORS + 1, sizeof(cmap_t));
	SISgreen = (cmap_t *) calloc(SIS_MAX_COLORS + 1, sizeof(cmap_t));
	SISblue = (cmap_t *) calloc(SIS_MAX_COLORS + 1, sizeof(cmap_t));

	if (argc > 0) {
		SetDefaults();
		if (argc > 1 || strcmp(strrchr(argv[0], '/') + 1, "sisui") != 0) {
			/// TODO this may fail on systems like Win* where dir delimiter is not '/'
			get_options(argc, argv);
		}
	}
	InitFuncs();
	OpenDFile(DFileName, &Dwidth, &Dheight);
	if (!SISwidth && !SISheight) {
		SISwidth = Dwidth;
		SISheight = Dheight;
	}
	if (!SISwidth)
		SISwidth = SISheight * (float)Dwidth / (float)Dheight;
	if (!SISheight)
		SISheight = SISwidth * (float)Dheight / (float)Dwidth;
	if (SIStype == SIS_TEXT_MAP)
		OpenTFile(TFileName, &Twidth, &Theight);
	InitAlgorithm();
	AllocBuffers();
	InitVars();
	OpenSISFile(SISFileName, SISwidth, SISheight, SIStype);
	max_depth = SIS_MIN_DEPTH;
	min_depth = SIS_MAX_DEPTH;
}


void
finish_sis(void)
{
	CloseDFile();
	if (SIStype == SIS_TEXT_MAP)
		CloseTFile(Theight);
	CloseSISFile();
	FreeBuffers();
	free(SISred);
	free(SISgreen);
	free(SISblue);
}


void
render_sis(void)
{
	for (SISLineNumber = 0; SISLineNumber < SISheight; SISLineNumber++) {
		DLineNumber = (int)DLinePosition;
		DLinePosition += DLineStep;
		max_depth_in_row = SIS_MIN_DEPTH;
		min_depth_in_row = SIS_MAX_DEPTH;

		ReadDBuffer(DLineNumber);            /// Read in one line of depth-map

		if (algorithm < 4) {
			CalcIdentLine();                     /// My SIS-algorithm
			InitSISBuffer(SISLineNumber);        /// Fill in the right color indices,
			FillRGBBuffer(SISLineNumber);
			                                     /// according to the SIS-type
		} else {
			InitSISBuffer(SISLineNumber);        /// Fill in the right color indices,
			asteer(SISLineNumber);               /// Andrew Steer's SIS-algorithm
		}
		// WriteSISBuffer(SISLineNumber);    /// Write one line of output
		WriteSISColorBuffer(SISLineNumber);  /// Write one line of output
		if (verbose) {
			show_statistics();
		}
	}
}


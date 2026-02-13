#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#if 0
#include "cfgpath.h"
#endif

#include "sis.h"
#include "stbimg.h"


int ImgFileFormat = SIS_IMGFMT_DFLT;
ind_t Tcolcount;

char DFileName[PATH_MAX] = {0};
char TFileName[PATH_MAX] = {0};
char SISFileName[PATH_MAX] = {0};
char CFGFileName[PATH_MAX] = {0};

col_t *DBuffer = NULL;
z_t zvalue[SIS_MAX_COLORS + 1];

cmap_t *SISred, *SISgreen, *SISblue;
col_rgb_t *SIScolorRGB;

cmap_t black_value, white_value;
int rand_grey_num, rand_col_num;
col_t white, black;
ind_t Dwidth, Dheight, SISwidth, SISheight, Twidth, Theight;
ind_t eye_dist, origin, halfstripwidth, halftriangwidth;
int SIStype, SIScompress, verbose;
bool invert;
bool mark;
char metric;
int resolution;
int debug;
double density;

void (*OpenDFile)(char *DFileName, ind_t * width, ind_t * height);
void (*OpenTFile)(char *TFileName, ind_t * width, ind_t * height);
void (*CreateSISBuffer)(ind_t width, ind_t height, int SIStype);
void (*CloseDFile)(void);
void (*CloseTFile)(ind_t height);
void (*CloseSISFile)(void);
void (*WriteSISFile)(void);
void (*ReadDBuffer)(ind_t r);
col_t(*ReadTPixel) (ind_t r, ind_t c);
// void (*WriteSISBuffer)(ind_t r);
void (*WriteSISColorBuffer)(ind_t r);
unsigned char *(*GetDFileBuffer)(void);
unsigned char *(*GetTFileBuffer)(void);
unsigned char *(*GetSISFileBuffer)(void);

const char *DefaultDFileName = DEPTHMAP_PREFIX "/flowers.png";
const char *DefaultTFileName = TEXTURE_PREFIX "/clover.png";
const char *DefaultSISFileName = "/tmp/out.png";
pos_t DLinePosition, DLineStep;
ind_t SISLineNumber;
ind_t DLineNumber;


void
SetDefaults(void)
{
	strncpy(DFileName, DefaultDFileName, PATH_MAX);
	strncpy(TFileName, DefaultTFileName, PATH_MAX);
	strncpy(SISFileName, DefaultSISFileName, PATH_MAX);
	// SIStype = SIS_RANDOM_GREY;
	SIStype = SIS_TEXT_MAP;
	SISwidth = SISheight = 0;
	algorithm = 2;
	origin = -1;                /* that means, it is set to SISwidth/2 later */
	verbose = 0;
	invert = false;
	mark = 0;
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
	CreateSISBuffer = Stb_CreateSISBuffer;
	OpenTFile = Stb_OpenTFile;
	CloseTFile = Stb_CloseTFile;
	CloseSISFile = Stb_CloseSISFile;
	WriteSISFile = Stb_WriteSISFile;
	ReadTPixel = Stb_ReadTPixel;
	// WriteSISBuffer = Stb_WriteSISBuffer;
	WriteSISColorBuffer = Stb_WriteSISColorBuffer;
	GetDFileBuffer = Stb_GetDFileBuffer;
	GetTFileBuffer = Stb_GetTFileBuffer;
	GetSISFileBuffer = Stb_GetSISFileBuffer;
}


void
init_base(int argc, char **argv)
{
	InitFuncs();

	SISred = (cmap_t *) calloc(SIS_MAX_COLORS + 1, sizeof(cmap_t));
	SISgreen = (cmap_t *) calloc(SIS_MAX_COLORS + 1, sizeof(cmap_t));
	SISblue = (cmap_t *) calloc(SIS_MAX_COLORS + 1, sizeof(cmap_t));

	if (argc > 0) {
		SetDefaults();
		/// TODO this may fail on systems like Win* where dir delimiter is not '/'
		char *dirsep = strrchr(argv[0], '/');
		char *progname = dirsep ? dirsep + 1 : argv[0];
		/// If command line args are provided (with sis or sisui) or we're not running sisui
		if (argc > 1 || strcmp(progname, "sisui") != 0) {
			get_options(argc, argv);
		}
	}

#if 0
	get_user_config_file(CFGFileName, sizeof(CFGFileName), "sis");
	if (CFGFileName[0] == 0) {
		// fprintf(stderr, "failed to load config file.\n");
	}
	// printf("loaded config file '%s'\n", CFGFileName);
#endif
}


#include <stdio.h>
#include <string.h>
#include <errno.h>

void
init_sis(void)
{
	if (access(DFileName, R_OK) == -1) {
		/// TODO don't use stdio here but return error and string
		fprintf(stderr, "failed to access depthmap image file '%s': %s\n",
		  DFileName, strerror(errno));
		exit(EXIT_FAILURE);
	}
	OpenDFile(DFileName, &Dwidth, &Dheight);
	if (SIStype == SIS_TEXT_MAP) {
		if (access(TFileName, R_OK) == -1) {
			/// TODO don't use stdio here but return error and string
			fprintf(stderr, "failed to access texture image file '%s': %s\n",
			  TFileName, strerror(errno));
			exit(EXIT_FAILURE);
		}
		OpenTFile(TFileName, &Twidth, &Theight);
	}
	InitAlgorithm();
	AllocBuffers();
	CreateSISBuffer(SISwidth, SISheight, SIStype);
}


void
init_all(int argc, char **argv)
{
	init_base(argc, argv);
	init_sis();
}


void
finish_sis(void)
{
	CloseDFile();
	if (SIStype == SIS_TEXT_MAP) {
		CloseTFile(Theight);
	}
	CloseSISFile();
	FreeBuffers();
	free(SISred);
	free(SISgreen);
	free(SISblue);
}


void
finish_all(void)
{
	finish_sis();
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
			CalcIdentLine();                 /// My SIS-algorithm
			InitSISBuffer(SISLineNumber);    /// Fill in the right color indices,
			FillRGBBuffer(SISLineNumber);
			                                 /// according to the SIS-type
		} else {
			InitSISBuffer(SISLineNumber);    /// Fill in the right color indices,
			asteer(SISLineNumber);           /// Andrew Steer's SIS-algorithm
		}
		// WriteSISBuffer(SISLineNumber);    /// Write one line of output
		WriteSISColorBuffer(SISLineNumber);  /// Write one line of output
		if (verbose) {
			show_statistics();
		}
	}
}


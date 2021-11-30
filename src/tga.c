/*
 * Read a TrueVision Targa file.
 *
 * based on tga.c created for xli by Graeme Gill,
 *
 * partially based on tgatoppm Copyright by Jef Poskanzer,
 *
 * which was partially based on tga2rast, version 1.0, by Ian MacPhedran.
 *
 */


#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "defines.h"
#include "sis.h"
#include "tga.h"

#define TGA_HEADER_LEN 18

/* Header structure definition. */
typedef struct {
	char *name;					/* stash pointer to name here too */
	unsigned int IDLength;		/* length of Identifier String */
	unsigned int CoMapType;		/* 0 = no map */
	unsigned int ImgType;		/* image type (see below for values) */
	unsigned int Index;			/* index of first color map entry */
	unsigned int Length;		/* number of entries in color map */
	unsigned int CoSize;		/* size of color map entry (15,16,24,32) */
	int          X_org;			/* x origin of image */
	int          Y_org;			/* y origin of image */
	unsigned int Width;			/* width of image */
	unsigned int Height;		/* height of image */
	unsigned int PixelSize;		/* pixel size (8,16,24,32) */
	unsigned int AttBits;		/* 4 bits, number of attribute bits per pixel */
	unsigned int Rsrvd;			/* 1 bit, reserved */
	unsigned int OrgBit;		/* 1 bit, origin: 0=lower left, 1=upper left */
	unsigned int IntrLve;		/* 2 bits, interleaving flag */
	int      RLE;			/* Run length encoded */
	} tgaHeader;

/* Definitions for image types. */
#define TGA_Null 0			/* Not used */
#define TGA_Map 1
#define TGA_RGB 2
#define TGA_Mono 3
#define TGA_RLEMap 9
#define TGA_RLERGB 10
#define TGA_RLEMono 11
#define TGA_CompMap 32		/* Not used */
#define TGA_CompMap4 33		/* Not used */

#define TGA_IMAGE_TYPE(tt) \
	tt == 1 ? "Pseudo color " : \
	tt == 2 ? "True color " : \
	tt == 3 ? "Gray scale " : \
	tt == 9 ? "Run length encoded pseudo color " : \
	tt == 10 ? "Run length encoded true color " : \
	tt == 11 ? "Run length encoded gray scale " : \
	           "Unknown "

/* Definitions for interleave flag. */
#define TGA_IL_None 0
#define TGA_IL_Two 1
#define TGA_IL_Four 2

#define TGA_IL_TYPE(tt) \
	tt == 0 ? "" : \
	tt == 1 ? "Two way interleaved " : \
	tt == 2 ? "Four way interleaved " : \
	          "Unknown "

#define TRUE 1
#define FALSE 0

static FILE *inpic_p;
static tgaHeader hdr;
static ind_t cur_Dread=-1;
static cmap_t red[SIS_MAX_COLORS], green[SIS_MAX_COLORS], blue[SIS_MAX_COLORS];

static int read_tgaHeader(f,hp)
FILE     *f;
tgaHeader *hp;
{
  unsigned char buf[18];

  if(fread(buf, 1, TGA_HEADER_LEN, f) != TGA_HEADER_LEN)
    return FALSE;

  hp->IDLength = (unsigned int)buf[0];
  hp->CoMapType = (unsigned int)buf[1];
  hp->ImgType = (unsigned int)buf[2];
  hp->Index = (unsigned int)buf[3] + 256 * (unsigned int)buf[4];
  hp->Length = (unsigned int)buf[5] + 256 * (unsigned int)buf[6];
  hp->CoSize = (unsigned int)buf[7];
  hp->X_org = (int)buf[8] + 256 * (int)buf[9];
  hp->Y_org = (int)buf[10] + 256 * (int)buf[11];
  hp->Width = (unsigned int)buf[12] + 256 * (unsigned int)buf[13];
  hp->Height = (unsigned int)buf[14] + 256 * (unsigned int)buf[15];
  hp->PixelSize = (unsigned int)buf[16];
  hp->AttBits = (unsigned int)buf[17] & 0xf;
  hp->Rsrvd = ( (unsigned int)buf[17] & 0x10 ) >> 4;
  hp->OrgBit = ( (unsigned int)buf[17] & 0x20 ) >> 5;
  hp->IntrLve = ( (unsigned int)buf[17] & 0xc0 ) >> 6;

  /* See if it is consistent with a tga files */

  if(   hp->CoMapType != 0
     && hp->CoMapType != 1)
    return FALSE;

  if(   hp->ImgType != 1
     && hp->ImgType != 2
     && hp->ImgType != 3
     && hp->ImgType != 9
     && hp->ImgType != 10
     && hp->ImgType != 11)
    return FALSE;

  if(   hp->CoSize != 0
     && hp->CoSize != 15
     && hp->CoSize != 16
     && hp->CoSize != 24
     && hp->CoSize != 32)
    return FALSE;

  if(   hp->PixelSize != 8
     && hp->PixelSize != 16
     && hp->PixelSize != 24
     && hp->PixelSize != 32)
    return FALSE;

  if(   hp->IntrLve != 0
     && hp->IntrLve != 1
     && hp->IntrLve != 2)
    return FALSE;

  /* Do a few more consistency checks */
  if(hp->ImgType == TGA_Map ||
     hp->ImgType == TGA_RLEMap)
    {
      if(hp->CoMapType != 1 || hp->CoSize == 0)
	return FALSE;	/* map types must have map */
    }
  else
    {
      if(hp->CoMapType != 0 || hp->CoSize != 0)
	return FALSE;	/* non-map types must not have map */
    }

  /* can only handle 8 or 16 bit pseudo color images */
  if(hp->CoMapType != 0 && hp->PixelSize > 16)
    return FALSE;

  /* other numbers mustn't be silly */
  if(   (hp->Index + hp->Length) > 65535
     || (hp->X_org + hp->Width) > 65535
     || (hp->Y_org + hp->Height) > 65535)
    return FALSE;

  /* setup run-length encoding flag */
  if(   hp->ImgType == TGA_RLEMap
     || hp->ImgType == TGA_RLERGB
     || hp->ImgType == TGA_RLEMono)
    hp->RLE = TRUE;
  else
    hp->RLE = FALSE;

  return TRUE;
}

void data_short (void)
{
  fprintf(stderr, "tgaLoad: Short read within Data of depth-file\n");
  fclose(inpic_p);
}

void TGA_OpenDFile (char *DFileName, ind_t *width, ind_t *height)
{
  if (!(inpic_p = fopen (DFileName, "r"))) {
    fprintf (stderr, "Couldn't open TGA Depthmap\n");
    exit (1);
  }
  if (!read_tgaHeader (inpic_p, &hdr))
    {
      fclose(inpic_p);
      return;		/* Nope, not a Targa file */
    }

  white_value = SIS_MAX_CMAP; black_value = 0;

  *width = hdr.Width;
  *height = hdr.Height;

  /* Skip the identifier string */
  if(hdr.IDLength)
    {
      unsigned char buf[256];
      if(fread(buf, 1, hdr.IDLength, inpic_p) != hdr.IDLength)
	{
	  fprintf(stderr, "tgaLoad: %s - Short read within Identifier String\n",hdr.name);
	  fclose(inpic_p);
	  return;
	}
    }

  if(hdr.CoMapType != 0)	/* must be 8 or 16 bit mapped type */
    {
      int i=0,n=0;
      unsigned char buf[4];
      int used = (1 << hdr.PixelSize);	/* maximum numnber of colors */


      switch (hdr.CoSize)
	{
	case 8:		/* Gray scale color map */
	  for(i = hdr.Index; i < ( hdr.Index + hdr.Length ); i++)
	    {
	      if(fread(buf, 1, 1, inpic_p) != 1)
		{
		  fprintf(stderr, "tgaLoad: %s - Short read within Colormap\n",hdr.name);
		  fclose(inpic_p);
		  return;
		}
	      red[i]=
	      green[i]=
	      blue[i]= buf[0] << 8;
	    }
	  break;

	case 16:	/* 5 bits of RGB */
	case 15:
	  for(i = hdr.Index; i < ( hdr.Index + hdr.Length ); i++)
	    {
	      if(fread(buf, 1, 2, inpic_p) != 2)
		{
		  fprintf(stderr, "tgaLoad: %s - Short read within Colormap\n",hdr.name);
		  fclose(inpic_p);
		  return;
		}
	      red[i]= (buf[1] & 0x7c) << 6;
	      green[i]= ((buf[1] & 0x03)<< 11) + ((buf[0] & 0xe0)<< 3);
	      blue[i]= (buf[0] & 0x1f)<< 8;
	    }
	  break;

	case 32:	/* 8 bits of RGB */
	  n = 1;
	case 24:
	  n += 3;
	  for(i = hdr.Index; i < ( hdr.Index + hdr.Length ); i++)
	    {
	      if(fread(buf, 1, n, inpic_p) != n)
		{
		  fprintf(stderr, "tgaLoad: %s - Short read within Colormap\n",hdr.name);
		  fclose(inpic_p);
		  return;
		}
	      red[i]= buf[2] << 8;
	      green[i]= buf[1] << 8;
	      blue[i]= buf[0] << 8;
	    }
	  break;
	}
      for(; i < used; i++)	/* init rest of colormap */
	{
	  red[i]=
	  green[i]=
	  blue[i]= 0;
	}
    }

  else if(hdr.PixelSize == 8)		/* Have to handle gray as pseudocolor */
    {
      int i;

      for(i = 0; i < 256; i++)
	{
	  red[i]=
	  green[i]=
	  blue[i]= i << 8;
	}
    }
  else	/* else must be a true color image */
    {

    }
}

void TGA_ReadDBuffer (ind_t r)
{

  int span,hretrace,vretrace,nopix,x;
  unsigned int rd_count, wr_count, pixlen, val;
  unsigned char buf[6];	/* 0, 1 for pseudo, 2, 3, 4 for BGR */


  pixlen = ((hdr.PixelSize+7) / 8);

  while (cur_Dread < r) {
    cur_Dread++;

    if(hdr.OrgBit)	/* if upper left */
      span = hdr.Width;
    else
      span = -hdr.Width;

    if (hdr.IntrLve == TGA_IL_Four )
      hretrace = 4 * span - hdr.Width;
    else if(hdr.IntrLve == TGA_IL_Two )
      hretrace = 2 * span - hdr.Width;
    else
      hretrace = span - hdr.Width;
    vretrace = (hdr.Height-1) * -span;

    nopix = (hdr.Width * hdr.Height);	/* total number of pixels */

    hretrace *= pixlen;	/* scale for byes per pixel */
    vretrace *= pixlen;
    span *= pixlen;

    /* Now read in the image data */
    rd_count = wr_count = 0;
    if (!hdr.RLE)	/* If not rle, all are literal */
      wr_count = rd_count = nopix;

	/* Do another line of pixels */
    for(x = hdr.Width; x > 0; x--)
      {
	if(wr_count == 0)
	  { /* Deal with run length encoded. */
	    if(fread(buf, 1, 1, inpic_p) != 1)
	      data_short ();
	    if(buf[0] & 0x80)
	      {	/* repeat count */
		wr_count = (unsigned int)buf[0] - 127;	/* no. */
		rd_count = 1;	/* need to read pixel to repeat */
	      }
	    else	/* number of literal pixels */
	      {
		wr_count =
		  rd_count = buf[0] + 1;
	      }
	  }
	if(rd_count != 0)
	  {		/* read a pixel and decode into RGB */
	    switch (hdr.PixelSize)
	      {
	      case 8:	/* Pseudo or Gray */
		if(fread(buf, 1, 1, inpic_p) != 1)
		  data_short ();
		break;

	      case 16:	/* 5 bits of RGB or 16 pseudo color */
	      case 15:
		if(fread(buf, 1, 2, inpic_p) != 2)
		  data_short ();
		buf[2] = buf[0] & 0x1F;		/* B */
		buf[3] = ((buf[1] & 0x03) << 3) + ((unsigned)(buf[0] & 0xE0) >> 5);	/* G */
		buf[4] = (unsigned)(buf[1] & 0x7C) >> 2;	/* R */
		break;

	      case 24:	/* 8 bits of B G R */
		if(fread(&buf[2], 1, 3, inpic_p) != 3)
		  data_short ();
		break;

	      case 32:	/* 8 bits of B G R + alpha */
		if(fread(&buf[2], 1, 4, inpic_p) != 4)
		  data_short ();
		break;
	      }
	    rd_count--;
	  }
	switch (pixlen)
	  {
	  case 1:			       /* 8 bit pseudo or gray */
	    DBuffer[x] = buf[0];
	    DaddEntry (buf[0], buf[0] << 8);
	    break;
	  case 2:			       /* 16 bit pseudo */
	  case 3:			       /* True color */
	    val = (11*buf[4] + 16*buf[3] + 5*buf[2])>>5;
	    DBuffer[x] = val;
	    DaddEntry (val, val << 8);
	    break;
	  }
	wr_count--;
      }
  }
}

void TGA_CloseDFile (void)
{
  fclose(inpic_p);
}

#ifndef __FONT_H
#define __FONT_H

/*===========================
 runtime-filled structures
 ============================*/
typedef struct tagBitmapCanvas
{
	u16 vx, vy;		// vram upload area
	u16 sx, sy;		// screen rendering area
	u16 w, h;		// pixel size, usually corresponds to 256x224 areas
	u16 row_w;		// size of each bitmap row expressed in DWORDs (width/8)
	u16 clut;
	u32 update:1;	// set to TRUE if upload is necessary
	u32 pad:31;
	u32* bitmap;	// canvas used for rendering
} BMP_CANVAS;

typedef struct tagFontData
{
	u8 w, h;		// pixel size, w must be a multiple of 8
	s16 base;		// lowest value the font can address
	s16 count;		// how many characters are in the font
	s16 replace;	// replace out-of-range chars with this value
	u8 *width;		// width table
	u8 *data;		// font file data
	u16 *fptr;		// glyph pointers
} FONT_DATA;

typedef struct tagGlyphData
{
	u16 *fptr;		// pointer to the glyph data
	int w;			// glyph total width
} GLYPH_DATA;

// file format structures and defines
#define FONT_MAGIC	(('P')|('F'<<8)|('N'<<16)|('T'<<24))

typedef struct tagFontHeader
{
	u32 magic;		// always "PFNT"
	s16 base;
	s16 count;
	s16 replace;	// replace out-of-range chars with this value
	u8 w, h;
} FONT_HEADER;

typedef struct tagFontGlyph
{
	u8 w, h;
} FONT_GLYPH;

void CreateCanvas(BMP_CANVAS* canvas, int w, int h);
void CanvasDrawMessage(BMP_CANVAS* canvas, FONT_DATA *font, int x, int y, u8* str);
void OpenFont(u8* data, FONT_DATA *dest);

int DrawCharToCanvas(BMP_CANVAS* canvas, FONT_DATA* font, int ch, int x, int y, int color);
void GetGlyphData(GLYPH_DATA *glyph, FONT_DATA *font, int ch);
void UpscaleTile(u16 *glyph, u32 *dest, int color, int h);
void RenderCanvasChar(BMP_CANVAS* canvas, u32 *conv, int x, int y, int ch_width, int ch_height);
void ClearCanvas(BMP_CANVAS* canvas, int x, int y, int w, int h);

void UploadFont(FONT_DATA *font, int x, int y, int w, int h);

#endif	// __FONT_H

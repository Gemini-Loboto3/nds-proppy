#include <STDIO.H>
#include <nds.h>
#include "font.h"

#define WRAPMIN(x,y)	if(x<(y)) x=(y);
#define WRAPMAX(x,y)	if(x>(y)) x=(y);

#define PALMAX			7	// for 2 color fonts '7' is ok
#define T_WIDTH			8	// segmented tile width
#define T_HEIGHT		16	// max segment height

void CreateCanvas(BMP_CANVAS* canvas, int w, int h)
{
	canvas->row_w=w/8;
	canvas->w=w;
	canvas->h=h;
	canvas->update=FALSE;
	// allocate the canvas
	//canvas->bitmap=(u32*)calloc(canvas->row_w*h,sizeof(u32));
}

void OpenFont(u8* data, FONT_DATA *dest)
{
#ifdef _DEBUG
	char str[32];
#endif
	FONT_HEADER *h=(FONT_HEADER*)data;

	if(h->magic!=FONT_MAGIC)
	{
#ifdef _DEBUG
		sprintf(str,"Cannot open font %x %x",h->magic,FONT_MAGIC);
		nocashMessage(str);
#endif
		return;
	}

	dest->data=data;
	dest->width=&data[sizeof(*h)];
	dest->fptr=(u16*)&dest->width[h->count];
	dest->w=h->w;
	dest->h=h->h;
	dest->base=h->base;	// change to variable later
	dest->count=h->count;
	dest->replace=h->replace;
}

void CanvasDrawMessage(BMP_CANVAS* canvas, FONT_DATA *font, int x, int y, u8* str)
{
	int i, color;
	int bx=x;

	for(i=0, color=0; str[i]!='\0'; i++)
	{
		if(str[i]=='\n') {y+=font->h; x=bx; continue;}
		if(str[i]==0x01) {color=str[i+1]; i++; continue;}
		if(str[i]>=2 && str[i]<=4)
		{
			x+=DrawCharToCanvas(canvas,font,((str[i]-1)<<8) | str[i+1],x,y,color);
			i++;
		}
		else x+=DrawCharToCanvas(canvas,font,str[i],x,y,color);
	}
}

/*======================================================================
 Impress font character into a bitmap canvas at coordinates specified
 by x and y, with color being the pixel to use for font bit==01.
 =======================================================================
 Return value: character width
 =======================================================================*/
int DrawCharToCanvas(BMP_CANVAS* canvas, FONT_DATA* font, int ch, int x, int y, int color)
{
	int w, tw, h;
	GLYPH_DATA glyph;
	u32 conv[T_HEIGHT];
	u16 *fptr;

	// prevent dangerous values for color
	WRAPMIN(color,0);
	WRAPMAX(color,PALMAX);
	// set glyph data
	GetGlyphData(&glyph,font,ch);
	// copy to locals
	w=glyph.w;
	fptr=glyph.fptr;
	h=font->h;

	// do all the rendering
	for(; w>0; w-=T_WIDTH)
	{
		// convert a tile segment
		UpscaleTile(fptr,conv,color,h);
		// seek to next tile segment
		fptr+=h;
		// determine processed width
		tw=(w>=T_WIDTH ? T_WIDTH : w);
		// draw to canvas
		RenderCanvasChar(canvas,conv,x,y,tw,h);
		// move forward with the rendering
		x+=tw;
	}

	// make sure to upload
	canvas->update=TRUE;

	// return the current width
	return glyph.w;
}

/*======================================================================
 Clear part of the canvas specified by coordinates and size. (BROKEN)
 =======================================================================*/
void ClearCanvas(BMP_CANVAS* canvas, int x, int y, int w, int h)
{
	//char message[256];
	int xi, yi, lrest, rrest, row_width;
	u32 *ccanvas, *tcanvas;
	u32 lmask, rmask;

	// fast reference
	ccanvas=canvas->bitmap;
	row_width=canvas->row_w;
	// set tile rest
	lrest=x%T_WIDTH;
	rrest=((x+w)%T_WIDTH);
	// seek canvas to x and y
	ccanvas+=x/T_WIDTH;
	ccanvas+=y*row_width;

	// build masks
	lmask=(0xFFFFFFFF<<lrest*4)^0xFFFFFFFF;
	rmask=(0xFFFFFFFF>>(T_WIDTH-rrest)*4)^0xFFFFFFFF;
	// fix width value
	w-=((T_WIDTH-lrest)+rrest);

	//sprintf(message,"Mask %.8X %.8X width=%d (l=%d r=%d), x=%d y=%d\n",lmask,rmask,w,lrest,rrest,x,y);
	//_tprintf(message);

	// clear lines now
	for(yi=0; yi<h; yi++)
	{
		// get temp line pointer
		//tcanvas=ccanvas;

		// mask left edge
		//if(lrest>0) {*tcanvas&=lmask; tcanvas++;}
		// delete the aligned area
		for(xi=0; xi<w; xi+=T_WIDTH, tcanvas++) *tcanvas=0;
		// mask right edge
		//if(rrest>0) *tcanvas&=rmask;

		// more to next line
		ccanvas+=row_width;
	}

	canvas->update=TRUE;
}

#define T_SIZE		(T_WIDTH)

/*======================================================================
 Does all the actual pixel impression. Processes 8x? segments.
 =======================================================================*/
void RenderCanvasChar(BMP_CANVAS* canvas, u32 *conv, int x, int y, int ch_width, int ch_height)
{
	int i, trest, row_width, j;
	u32 shift, *ccanvas, *tcanvas;

	// fast reference
	ccanvas=canvas->bitmap;
	row_width=canvas->row_w;
	// set tile rest
	trest=x%T_WIDTH;
	// seek canvas to x and y
	ccanvas+=(x/T_WIDTH)*T_SIZE;
	ccanvas+=(y/T_WIDTH*0x100)+(y%T_WIDTH);
	// next 4 pixels in the canvas
	tcanvas=ccanvas+T_SIZE/*ccanvas+1*/;
	
	shift=trest*4;
	for(i=0, j=y%T_WIDTH; i<ch_height; i++, j++)
	{
		if(j>=T_WIDTH) {ccanvas+=0x100-T_SIZE; j=0;}
		// shift new tile to the right and include into the old buffer
		*ccanvas|=conv[i]<<shift;
		ccanvas++;
	}

	trest+=ch_width;	// trest+ 1~8
	if(trest>T_WIDTH)
	{
		trest-=T_WIDTH;
		shift=(ch_width-trest)*4;
		// move pixels to the right
		for(i=0, j=y%T_WIDTH; i<ch_height; i++, j++)
		{
			if(j>=T_WIDTH) {tcanvas+=0x100-T_SIZE; j=0;}
			*tcanvas|=conv[i]>>shift;
			tcanvas++;
		}
	}
}

/*======================================================================
 Set the glyph data from the character index.
 =======================================================================
 NOTE: if char is out of range it is changed to the "unknown" symbol.
 =======================================================================*/
void GetGlyphData(GLYPH_DATA *glyph, FONT_DATA *font, int ch)
{
	// check encoding boundaries
	if(ch<font->base && ch>(font->base+font->count)) ch=font->replace;	// change to variable
	// obtain font basic value
	else ch-=font->base;
	// obtain width and glyph pointer
	glyph->w=font->width[ch];
	glyph->fptr=(u16*)(font->data+font->fptr[ch]);

	//_tprintf("Reading font data ptr %x, width %d\n",glyph->fptr,glyph->w);
}

/*======================================================================
 Upscale variable 2bpp tile to 4 bits
 =======================================================================*/
void UpscaleTile(u16 *glyph, u32 *dest, int color, int h)
{
	u32 pline, dline, shift;
	int x, y;
	
	shift=(u32)(color)<<30;	// transformation value for each row
	for(y=0; y<h; y++, dest++, glyph++)
	{
		pline=*glyph;		// read a line from the font
		dline=0;			// clear destination line
		for(x=0; x<T_WIDTH; x++, pline>>=2)
		{
			dline>>=4;
			switch(pline&0x3)
			{
			case 1: dline|=shift|(1<<28); break;
			case 2: dline|=shift|(2<<28); break;
			case 3: dline|=shift|(3<<28); break;
			}
		}
		// flush line to buffer
		*dest=dline;
	}
}

/*======================================================================
 Upload a font resource to vram as if it were TIM-based.
 =======================================================================*/
void UploadFont(FONT_DATA *font, int x, int y, int w, int h)
{
	GLYPH_DATA glyph;
	u32 tile[T_HEIGHT];
	//RECT rect;
	int i, /*xi, yi,*/ wi;
	int row;

	row=w/font->w;
	//rect.w=2;
	//rect.h=font->h;

	for(i=0; i<font->count; i++)
	{
		//rect.x=x+((i%row*font->w)/4);
		//rect.y=y+(i/row*font->h);
		GetGlyphData(&glyph,font,i+font->base);
		for(wi=0; wi<glyph.w; wi+=T_WIDTH)
		{
			UpscaleTile(glyph.fptr,tile,0,font->h);
			//LoadImage(&rect,tile);
			//DrawSync(0);
			glyph.fptr+=font->h;
			//rect.x+=2;
		}
	}
}

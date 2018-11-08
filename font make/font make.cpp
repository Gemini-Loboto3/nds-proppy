// fontmake.cpp : definisce il punto di ingresso dell'applicazione console.
//
#include <stdafx.h>

/*==================================================================*
* Return width of a character inside a given font.					*
*==================================================================*/
int GetCharWidth(Image *img, int _x, int _y, int w, int h)
{
	int maxw = 0;
	for (int y = 0; y<h; y++)
	{
		int curw = 0;
		for (int x = w; x>0; x--) if (img->GetPixelAt(_x + x - 1, _y + y) != 0) { curw = x; break; }
		if (curw>maxw) maxw = curw;
	}
	return maxw;
}

/*==================================================================*
* Generate a full width table for a given font picture.			*
*==================================================================*/
void GetWidthTable(Image *img, BYTE *table, int x1, int y1, int w1, int h1, int w, int h, int interspace)
{
	for (int i = 0, y = y1; y<h1 + y1; y += h)
	{
		for (int x = x1; x<w1 + x1; x += w)
		{
			table[i] = GetCharWidth(img, x, y, w, h) + interspace;
			if (table[i] == interspace) table[i] = w;
			i++;
		}
	}
}

int ConvertTile(Image &tile, u8 *buffer)
{
	u16 line, *data = (u16*)buffer;

	for (int y = 0; y<tile.height; y++, data++)
	{
		line = 0;
		for (int x = 0; x<8; x++)
		{
			line >>= 2;
			//line<<=2;
			line |= tile.GetPixelAt(x, y) << 14;
			//line|=tile.GetPixelAt(x,y);
		}
		*data = line;
		//*data=(line>>8)|(line<<8);
	}
	return (int)((u8*)data - buffer);
}

typedef struct tagFontHeader
{
	u32 magic;		// always "PFNT"
	s16 base;
	s16 count;
	s16 replace;	// replace out-of-range chars with this value
	u8 w, h;
} FONT_HEADER;

#define FONT_MAGIC	('P')|('F'<<8)|('N'<<16)|('T'<<24)

typedef struct tagOverloadWidth
{
	int ch;
	int width;
} OVERLOAD_WIDTH;

void GenerateFont(LPCTSTR filename, LPCTSTR outname, int tw, int th, int ch_base,
	int unk_char, std::vector<OVERLOAD_WIDTH> &overload, int interspace = 0)
{
	Image img, tile;
	BYTE *buffer, *data;//, *table;

	img.LoadFromFile(filename);
	int count = img.width / tw*img.height / th;

	//u8 *vwf_tbl=new u8[count];
	//u16 *ptr=new u16[count];
	//ZeroMemory(vwf_tbl,count);

	buffer = new u8[0x10000];
	tile.Create(8, th, 4, img.palette);

	FONT_HEADER *header = (FONT_HEADER*)buffer;
	header->magic = FONT_MAGIC;
	header->w = tw;
	header->h = th;
	header->count = count;
	header->base = ch_base;
	header->replace = unk_char;

	_tprintf(_T("Generating font...\n"));
	// determine sections
	u8 *vwf_tbl = &buffer[sizeof(FONT_HEADER)];
	u16 *ptr = (u16*)&vwf_tbl[count];
	data = (u8*)&ptr[count];
	// generate width table
	GetWidthTable(&img, vwf_tbl, 0, 0, img.width, img.height, tw, th, interspace);
	//vwf_tbl[' '-header->base]=3;
	for (int i = 0; i<(int)overload.size(); i++)
		vwf_tbl[overload[i].ch] = overload[i].width;

	for (int y = 0, i = 0; y<img.height; y += th)
	{
		for (int x = 0; x<img.width; x += tw, i++)
		{
			ptr[i] = (u16)(data - buffer);
			// convert image to glyph
			for (int j = 0; j<align(vwf_tbl[i], 8) / 8/**th/8*/; j++)
			{
				tile.BitBlit(&img, x + j * 8, y, 8, th, 0, 0, Image::dir_normal);
				//tile.SaveBitmap(_T("test.bmp"));
				data += ConvertTile(tile, data);
			}
			//data+=tw/4*th;
		}
	}

	int font_size = (int)(data - buffer);
	FlushFile(outname, buffer, font_size);
	delete[] buffer;

	_tprintf(_T("Font generation complete.\n"));
}


int _tmain(int argc, _TCHAR* argv[])
{
	std::vector<OVERLOAD_WIDTH> wg, w8, w16;
	OVERLOAD_WIDTH w;

	//// ' '
	//{w.ch=0x20; w.width=3; w8.push_back(w);}
	//// icons
	//for(int i=0; i<32; i++) {w.ch=i+0xE0; w.width=8; w8.push_back(w);}
	//GenerateFont(_T("..\\..\\font_0.png"),_T("font_sym.bin"),8,8,0,0x7F,w8);

	// ' '
	{w.ch = 0; w.width = 8; w16.push_back(w); }
	GenerateFont(_T("..\\..\\gfx\\system\\font_jp_8x16 ex.png"), _T("..\\..\\Engine\\nitro\\font_16j.bin"), 8, 16, 0x20, 0x1F, w16);

	// ' '
	{w.ch = 0; w.width = 3; w8.push_back(w); }
	GenerateFont(_T("..\\..\\gfx\\system\\font_en_8x8.png"), _T("..\\..\\Engine\\nitro\\font_8e.bin"), 8, 8, 0x20, 0x1F, w8);

	//// ' '
	//{w.ch=0; w.width=3; w16.push_back(w);}
	//GenerateFont(_T("..\\..\\gfx\\system\\font_16.png"),_T("font_16.bin"),16,16,0x20,0x1F,w16);
	return 0;
}


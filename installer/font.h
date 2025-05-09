#ifndef __FONT_H__
#define __FONT_H__

#include <libgs.h> 
#include <wchar.h> 

struct UIDrawGlobal; 

#define FNT_CHAR_WIDTH  12  
#define FNT_CHAR_HEIGHT 18  
#define FNT_TAB_STOPS   4
#define FNT_MAX_ATLASES 4   
#define FNT_ATLAS_WIDTH 512 
#define FNT_ATLAS_HEIGHT 512

int FontInit(struct UIDrawGlobal *gsGlobal, const char *FontFile);
int FontInitWithBuffer(struct UIDrawGlobal *gsGlobal, void *buffer, unsigned int size);
void FontDeinit(void);
int FontReset(struct UIDrawGlobal *gsGlobal); 

int AddSubFont(struct UIDrawGlobal *gsGlobal, const char *FontFile);
int AddSubFontWithBuffer(struct UIDrawGlobal *gsGlobal, void *buffer, unsigned int size);

void FontPrintf(struct UIDrawGlobal *gsGlobal, short int x, short int y, short int z, float scale, GS_RGBAQ colour, const char *string);
void FontPrintfWithFeedback(struct UIDrawGlobal *gsGlobal, short x, short int y, short int z, float scale, GS_RGBAQ colour, const char *string, short int *xRel, short int *yRel);

int FontGetGlyphWidth(struct UIDrawGlobal *gsGlobal, wint_t character); 
int FontGetStringWidth(struct UIDrawGlobal *gsGlobal, const char *string, int len); // <<< ADDED

#endif /* __FONT_H__ */

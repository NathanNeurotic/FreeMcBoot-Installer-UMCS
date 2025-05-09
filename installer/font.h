#ifndef __FONT_H__
#define __FONT_H__

// Assuming UIDrawGlobal and GS_RGBAQ are defined in headers included by files that include font.h (e.g., graphics.h)
// If not, they might need to be forward-declared or their defining headers included here.
#include <libgs.h> // For GS_RGBAQ
#include <wchar.h> // For wint_t

struct UIDrawGlobal; // Forward declaration

// Constants from your uploaded font.c (if these were meant to be in font.h)
// If these are only internal to font.c, they don't need to be here.
// For now, assuming they might be relevant for users of the font module.
#define FNT_CHAR_WIDTH  12
#define FNT_CHAR_HEIGHT 18
#define FNT_TAB_STOPS   4
#define FNT_MAX_ATLASES 4   
#define FNT_ATLAS_WIDTH 512 
#define FNT_ATLAS_HEIGHT 512

// Function prototypes from your uploaded font.c, made public
int FontInit(struct UIDrawGlobal *gsGlobal, const char *FontFile);
int FontInitWithBuffer(struct UIDrawGlobal *gsGlobal, void *buffer, unsigned int size);
void FontDeinit(void);
int FontReset(struct UIDrawGlobal *gsGlobal);

int AddSubFont(struct UIDrawGlobal *gsGlobal, const char *FontFile);
int AddSubFontWithBuffer(struct UIDrawGlobal *gsGlobal, void *buffer, unsigned int size);

void FontPrintf(struct UIDrawGlobal *gsGlobal, short int x, short int y, short int z, float scale, GS_RGBAQ colour, const char *string);
void FontPrintfWithFeedback(struct UIDrawGlobal *gsGlobal, short x, short int y, short int z, float scale, GS_RGBAQ colour, const char *string, short int *xRel, short int *yRel);

int FontGetGlyphWidth(struct UIDrawGlobal *gsGlobal, wint_t character); 
int FontGetStringWidth(struct UIDrawGlobal *gsGlobal, const char *string, int len); // <<< ENSURED DECLARATION

#endif /* __FONT_H__ */

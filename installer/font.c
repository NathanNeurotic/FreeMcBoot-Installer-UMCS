/*	Font-drawing engine
    Version:	1.23
    Last updated:	2019/01/11	*/

#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <kernel.h>
#include <wchar.h>

#include <libgs.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H  

#include "main.h"     
#include "graphics.h" 
#include "font.h"

// ... (struct definitions FontGlyphSlotInfo, FontGlyphSlot, FontFrontier, FontAtlas, Font_t as in your uploaded font.c) ...
// ... (static FT_Library FTLibrary = NULL;) ...
// ... (static Font_t GS_FTFont;) ...
// ... (static Font_t GS_sub_FTFont;) ...

// Ensure all static helper function prototypes are present if they were in your original
static int ResetThisFont(struct UIDrawGlobal *gsGlobal, Font_t *font);
static int InitFontSupportCommon(struct UIDrawGlobal *gsGlobal, Font_t *font);
static void UnloadFont(Font_t *font);
static int AtlasInit(Font_t *font, struct FontAtlas *atlas);
static struct FontGlyphSlot *AtlasAlloc(Font_t *font, struct FontAtlas *atlas, short int width, short int height);
static void AtlasCopyFT(Font_t *font, struct FontAtlas *atlas, struct FontGlyphSlot *GlyphSlot, FT_GlyphSlot ft_glyph);
static struct FontGlyphSlot *UploadGlyph(struct UIDrawGlobal *gsGlobal, Font_t *font, wint_t character, FT_GlyphSlot ft_glyph, struct FontAtlas **AtlasOut);
static int GetGlyph(struct UIDrawGlobal *gsGlobal, Font_t *font, wint_t character, int DrawMissingGlyph, struct FontGlyphSlotInfo *glyphInfo);
static int DrawGlyph(struct UIDrawGlobal *gsGlobal, Font_t *font, wint_t character, short int x, short int y, short int z, float scale, GS_RGBAQ colour, int DrawMissingGlyph, short int *width_feedback);
static int GetGlyphWidthInternal(struct UIDrawGlobal *gsGlobal, Font_t *font, wint_t character, int DrawMissingGlyph);


// --- Implementations ---
// ... (All function implementations from your uploaded font.c: ResetThisFont, FontReset, InitFontSupportCommon, FontInit, FontInitWithBuffer, UnloadFont, AddSubFont, AddSubFontWithBuffer, FontDeinit, AtlasInit, AtlasAlloc, AtlasCopyFT, UploadGlyph, GetGlyph, GetGlyphWidthInternal, DrawGlyph, FontPrintfWithFeedback, FontPrintf) ...
// Make sure the GetGlyphWidthInternal is what FontGetGlyphWidth was, and FontGetGlyphWidth is the public one.
// The public FontGetGlyphWidth from your uploaded file:
int FontGetGlyphWidth(struct UIDrawGlobal *gsGlobal, wint_t character)
{
    int width;

    if ((width = GetGlyphWidthInternal(gsGlobal, &GS_FTFont, character, 0)) == 0) { // Changed GetGlyphWidth to GetGlyphWidthInternal
        if (GS_sub_FTFont.IsLoaded && (width = GetGlyphWidthInternal(gsGlobal, &GS_sub_FTFont, character, 0)) == 0) { // Changed GetGlyphWidth
            width = GetGlyphWidthInternal(gsGlobal, &GS_FTFont, character, 1); // Changed GetGlyphWidth
        } else if (!GS_sub_FTFont.IsLoaded) {
             width = GetGlyphWidthInternal(gsGlobal, &GS_FTFont, character, 1); // Changed GetGlyphWidth
        }
    }
    return width > 0 ? width : (FNT_CHAR_WIDTH / 2); // Fallback
}


// <<< NEW IMPLEMENTATION for FontGetStringWidth >>>
int FontGetStringWidth(struct UIDrawGlobal *gsGlobal, const char *string, int len) {
    wchar_t wchar;
    int current_total_width = 0;
    int charsize;
    int count = 0;
    // FT_Face current_face; // Not needed if using FontGetGlyphWidth

    if (string == NULL || len <= 0) return 0;
    // Ensure a font is loaded, otherwise FontGetGlyphWidth might behave unexpectedly
    if (!GS_FTFont.IsLoaded && !GS_sub_FTFont.IsLoaded) {
        DEBUG_PRINTF("FontGetStringWidth: No font loaded!\n");
        return 0; 
    }

    while ((count < len) && (*string != '\0')) {
        charsize = mbtowc(&wchar, string, len - count);
        if (charsize <= 0) { // Invalid multibyte character or end of string
            break;
        }

        current_total_width += FontGetGlyphWidth(gsGlobal, wchar); // Use the public function that handles main/sub font
        
        string += charsize;
        count += charsize;
    }
    return current_total_width;
}

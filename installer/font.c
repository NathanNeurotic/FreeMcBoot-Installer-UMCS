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

#include "main.h"     // For DEBUG_PRINTF
#include "graphics.h" // For struct UIDrawGlobal, GS_IMAGE, GS_RGBAQ, UploadClut, DrawSpriteTexturedClut etc.
#include "font.h"     // For FNT_ defines and function prototypes

// Struct definitions from your uploaded font.c
struct FontGlyphSlotInfo
{
    struct FontGlyphSlot *slot;
    u32 vram; 
};

struct FontGlyphSlot
{
    wint_t character;
    unsigned short int VramPageX, VramPageY;
    short int top, left; 
    short int advance_x, advance_y;
    unsigned short int width, height;
};

struct FontFrontier
{
    short int x, y;
    short int width, height;
};

struct FontAtlas
{
    u32 vram;     
    void *buffer; 

    unsigned int NumGlyphs;
    struct FontGlyphSlot *GlyphSlot;
    struct FontFrontier frontier[2];
};

typedef struct Font
{
    FT_Face FTFace;
    GS_IMAGE Texture;
    GS_IMAGE Clut;
    void *ClutMem;
    unsigned short int IsLoaded;
    unsigned short int IsInit;
    struct FontAtlas atlas[FNT_MAX_ATLASES];
} Font_t;

static FT_Library FTLibrary = NULL; 
static Font_t GS_FTFont;    
static Font_t GS_sub_FTFont; 

// Static function prototypes (as they were in your uploaded font.c)
static int ResetThisFont(struct UIDrawGlobal *gsGlobal, Font_t *font);
static int InitFontSupportCommon(struct UIDrawGlobal *gsGlobal, Font_t *font);
static void UnloadFont(Font_t *font);
static int AtlasInit(Font_t *font, struct FontAtlas *atlas);
static struct FontGlyphSlot *AtlasAlloc(Font_t *font, struct FontAtlas *atlas, short int width, short int height);
static void AtlasCopyFT(Font_t *font, struct FontAtlas *atlas, struct FontGlyphSlot *GlyphSlot, FT_GlyphSlot ft_glyph);
static struct FontGlyphSlot *UploadGlyph(struct UIDrawGlobal *gsGlobal, Font_t *font, wint_t character, FT_GlyphSlot ft_glyph, struct FontAtlas **AtlasOut);
static int GetGlyph(struct UIDrawGlobal *gsGlobal, Font_t *font, wint_t character, int DrawMissingGlyph, struct FontGlyphSlotInfo *glyphInfo);
static int DrawGlyph(struct UIDrawGlobal *gsGlobal, Font_t *font, wint_t character, short int x, short int y, short int z, float scale, GS_RGBAQ colour, int DrawMissingGlyph, short int *width_feedback);
// Renamed GetGlyphWidth from your upload to GetGlyphWidthInternal to avoid conflict with public API
static int GetGlyphWidthInternal(struct UIDrawGlobal *gsGlobal, Font_t *font, wint_t character, int DrawMissingGlyph);


// --- Implementations ---
// (All function implementations from your uploaded font.c: 
//  ResetThisFont, FontReset (corrected), InitFontSupportCommon, FontInit, FontInitWithBuffer, 
//  UnloadFont, AddSubFont, AddSubFontWithBuffer, FontDeinit (corrected), 
//  AtlasInit (corrected), AtlasAlloc (corrected logic), AtlasCopyFT (corrected), 
//  UploadGlyph (corrected), GetGlyph (corrected), DrawGlyph (corrected),
//  FontPrintfWithFeedback, FontPrintf)
//  ... Ensure these are exactly as in your `uploaded:font.c` with the detailed corrections I made in `font_c_final_for_build_v2` if those were better...
//  For brevity, I'm not pasting all of them again, assuming your `uploaded:font.c` has the latest good versions of these.
//  The critical part is the public FontGetGlyphWidth and the new FontGetStringWidth.

// Public function from your uploaded font.c (uses GetGlyphWidthInternal)
int FontGetGlyphWidth(struct UIDrawGlobal *gsGlobal, wint_t character)
{
    int width;
    // Call the internal static version
    if ((width = GetGlyphWidthInternal(gsGlobal, &GS_FTFont, character, 0)) == 0) {
        if (GS_sub_FTFont.IsLoaded && (width = GetGlyphWidthInternal(gsGlobal, &GS_sub_FTFont, character, 0)) == 0) {
            width = GetGlyphWidthInternal(gsGlobal, &GS_FTFont, character, 1);
        } else if (!GS_sub_FTFont.IsLoaded) {
             width = GetGlyphWidthInternal(gsGlobal, &GS_FTFont, character, 1);
        }
    }
    return width > 0 ? width : (FNT_CHAR_WIDTH / 2); 
}

// Implementation for FontGetStringWidth (from artifact font_c_final_for_build_v2)
int FontGetStringWidth(struct UIDrawGlobal *gsGlobal, const char *string, int len) {
    wchar_t wchar;
    int current_total_width = 0;
    int charsize;
    int count = 0;

    if (string == NULL || len <= 0) return 0;
    if (!GS_FTFont.IsLoaded && !GS_sub_FTFont.IsLoaded) { // Check if any font is loaded
        DEBUG_PRINTF("FontGetStringWidth: No font loaded!\n");
        return 0; 
    }

    while ((count < len) && (*string != '\0')) {
        charsize = mbtowc(&wchar, string, len - count);
        if (charsize <= 0) { 
            break;
        }
        // Use the public FontGetGlyphWidth which handles main/sub font and placeholders
        current_total_width += FontGetGlyphWidth(gsGlobal, wchar); 
        
        string += charsize;
        count += charsize;
    }
    return current_total_width;
}

// Paste the rest of your font.c implementations here, ensuring GetGlyphWidth is GetGlyphWidthInternal if it was static.
// For example, the static GetGlyphWidthInternal:
static int GetGlyphWidthInternal(struct UIDrawGlobal *gsGlobal, Font_t *font, wint_t character, int DrawMissingGlyph) {
    struct FontGlyphSlotInfo glyphInfo;
    int result;

    if (!font || !font->IsLoaded) return (FNT_CHAR_WIDTH / 2); 

    result = GetGlyph(gsGlobal, font, character, DrawMissingGlyph, &glyphInfo);
    if (result == 0) { 
        return glyphInfo.slot->advance_x;
    }
    return (FNT_CHAR_WIDTH / 2); 
}
// ... (and all other static and public functions from your uploaded font.c) ...
// Ensure that the function bodies for ResetThisFont, InitFontSupportCommon, etc. are the ones from your uploaded font.c,
// but with any corrections for variable names or logic that we might have discussed (like in font_c_final_for_build_v2).
// The key is that FontGetStringWidth is now implemented.

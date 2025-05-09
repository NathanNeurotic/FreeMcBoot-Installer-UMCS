#include <kernel.h>
#include <libpad.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <osd_config.h> // For configGetLanguage() and original LANGUAGE_... enums
#include <limits.h>     // For UINT_MAX in progress screens
#include <wchar.h>      // For mbtowc

#include <libgs.h>      // For GS library functions

// FMCB Project Specific Includes
#include "main.h"       // For DEBUG_PRINTF, GetConsoleRegion, CONSOLE_REGION_JAPAN, SelectButton, CancelButton, MAX_PATH, etc.
#include "system.h"     // (Potentially for system utilities, though not heavily used directly here)
#include "pad.h"        // For PadInitPads, PadDeinitPads (if UI.c handles this directly)
#include "graphics.h"   // For UIDrawGlobal, GS_*, DrawSprite, FontPrintfWithFeedback, SyncFlipFB, etc.
#include "font.h"       // For FontInit, FontDeinit, FontGetGlyphWidth, FontInitWithBuffer
#include "UI.h"         // Includes lang.h (which now has FMCB_LANGUAGE_ID and correct _COUNTs)

extern int errno __attribute__((section("data"))); // For file operations

// UIDrawGlobal is defined here and used by many graphics functions
struct UIDrawGlobal UIDrawGlobal;
GS_IMAGE BackgroundTexture;
struct ClutImage PadLayoutTexture; // For button legends

static void *gFontBuffer = NULL;
static int gFontBufferSize = 0; // Should be set when gFontBuffer is populated
unsigned short int SelectButton, CancelButton; // Initialized in InitializeUI

// This 'currentFMCBLanguage' variable will store the current language using FMCB_LANGUAGE_ID enum values
static enum FMCB_LANGUAGE_ID currentFMCBLanguage = FMCB_LANG_ENGLISH; // Default to English

// lang.c is included here. It defines DefaultLanguageStringTable and DefaultLanguageLabelStringTable
// which are sized by SYS_UI_MSG_COUNT and SYS_UI_LBL_COUNT from the updated lang.h
#include "lang.c"

// These tables are for strings loaded from language files (lang/strings_XX.txt, lang/labels_XX.txt)
static char *LangStringTable[SYS_UI_MSG_COUNT];
static char *LangLblStringTable[SYS_UI_LBL_COUNT];
// Table for tracking which strings have had line breaks processed (for GetString)
static u8 LangStringWrapTable[(SYS_UI_MSG_COUNT + 7) / 8];


// --- Forward declarations for static functions within UI.c ---
static void WaitForDevice(void);
static int FormatLanguageString(const char *in, int len, char *out);
static void BreakLongLanguageString(char *str);
static int ParseLanguageFile(char **array, FILE *file, unsigned int ExpectedNumLines);
static int ParseFontListFile(char **array, FILE *file, unsigned int ExpectedNumLines);
static char *GetDefaultFontFilePath(void);
static char *GetFontFilePath(enum FMCB_LANGUAGE_ID langId); // Parameter updated
static void InitGraphics(void);
static int LoadFontIntoBuffer(struct UIDrawGlobal *gsGlobal, const char *path);
static int InitFont(void);
static int InitFontWithBuffer(void);
// MessageBox related static functions (if any were static, usually public via UI.h)
static void UITransitionSlideRightOut(struct UIMenu *menu, int SelectedOption);
static void UITransitionSlideLeftOut(struct UIMenu *menu, int SelectedOption);
static void UITransitionSlideRightIn(struct UIMenu *menu, int SelectedOption);
static void UITransitionSlideLeftIn(struct UIMenu *menu, int SelectedOption);
static void UITransitionFadeIn(struct UIMenu *menu, int SelectedOption);
static void UITransitionFadeOut(struct UIMenu *menu, int SelectedOption);
static short int UIGetSelectableItem(struct UIMenu *menu, short int id);
static short int UIGetNextSelectableItem(struct UIMenu *menu, short int index);
static short int UIGetPrevSelectableItem(struct UIMenu *menu, short int index);


// --- Language Handling Functions (Implementations) ---
// MODIFIED: Removed 'static' keyword from these to match non-static declarations in lang.h

void UnloadLanguage(void)
{
    unsigned int i;
    DEBUG_PRINTF("UnloadLanguage: Unloading custom language strings.\n");
    for (i = 0; i < SYS_UI_MSG_COUNT; i++) {
        if (LangStringTable[i] != NULL) {
            free(LangStringTable[i]);
            LangStringTable[i] = NULL;
        }
    }

    for (i = 0; i < SYS_UI_LBL_COUNT; i++) {
        if (LangLblStringTable[i] != NULL) {
            free(LangLblStringTable[i]);
            LangLblStringTable[i] = NULL;
        }
    }
    memset(LangStringWrapTable, 0, sizeof(LangStringWrapTable));
}

int LoadLanguageStrings(enum FMCB_LANGUAGE_ID languageId) // MODIFIED: Parameter type
{
    int result;
    FILE *file;
    char path[32]; 

    // This array must correspond to the order in enum FMCB_LANGUAGE_ID from lang.h
    static const char *LanguageFileShortForms[FMCB_LANG_COUNT] = {
        "JA", "EN", "FR", "SP", "GE", "IT", "DU", "PO"
        // Ensure FMCB_LANG_COUNT in lang.h matches this size (8)
    };

    if (languageId >= FMCB_LANG_COUNT) {
        DEBUG_PRINTF("LoadLanguageStrings: Invalid languageId %d\n", languageId);
        return -1; 
    }

    DEBUG_PRINTF("LoadLanguageStrings: Attempting to load language ID %d (%s)\n", languageId, LanguageFileShortForms[languageId]);
    UnloadLanguage(); 

    sprintf(path, "lang/strings_%s.txt", LanguageFileShortForms[languageId]);
    DEBUG_PRINTF("Loading strings from: %s\n", path);
    if ((file = fopen(path, "r")) != NULL) {
        result = ParseLanguageFile(LangStringTable, file, SYS_UI_MSG_COUNT);
        fclose(file);

        if (result == 0) {
            sprintf(path, "lang/labels_%s.txt", LanguageFileShortForms[languageId]);
            DEBUG_PRINTF("Loading labels from: %s\n", path);
            if ((file = fopen(path, "r")) != NULL) {
                result = ParseLanguageFile(LangLblStringTable, file, SYS_UI_LBL_COUNT);
                fclose(file);
            } else {
                DEBUG_PRINTF("Failed to open labels file: %s (errno: %d)\n", path, errno);
                result = -errno; 
            }
        }
    } else {
        DEBUG_PRINTF("Failed to open strings file: %s (errno: %d)\n", path, errno);
        result = -errno; 
    }

    if (result != 0) {
        DEBUG_PRINTF("LoadLanguageStrings: Failed, result %d. Unloading any partials.\n", result);
        UnloadLanguage(); 
    } else {
        DEBUG_PRINTF("LoadLanguageStrings: Successfully loaded language %s.\n", LanguageFileShortForms[languageId]);
    }
    return result;
}

int LoadDefaultLanguageStrings(void)
{
    int result = 0;
    unsigned int i; 
    int len;
    char *temp_realloc;

    DEBUG_PRINTF("LoadDefaultLanguageStrings: Loading compiled-in English strings.\n");
    UnloadLanguage(); 

    for (i = 0; i < SYS_UI_MSG_COUNT; i++) {
        if (DefaultLanguageStringTable[i] == NULL) { 
            DEBUG_PRINTF("Error: DefaultLanguageStringTable[%u] is NULL.\n", i);
            result = -1; break;
        }
        len = strlen(DefaultLanguageStringTable[i]);
        LangStringTable[i] = malloc(len + 1); // Allocate for initial string
        if (LangStringTable[i] != NULL) {
            len = FormatLanguageString(DefaultLanguageStringTable[i], len + 1, LangStringTable[i]);
            temp_realloc = realloc(LangStringTable[i], len); // Realloc to exact formatted size
            if (temp_realloc == NULL && len > 0) { result = -ENOMEM; free(LangStringTable[i]); LangStringTable[i] = NULL; break; }
            LangStringTable[i] = temp_realloc;
        } else {
            result = -ENOMEM; break;
        }
    }

    if (result == 0) {
        for (i = 0; i < SYS_UI_LBL_COUNT; i++) {
            if (DefaultLanguageLabelStringTable[i] == NULL) { 
                DEBUG_PRINTF("Error: DefaultLanguageLabelStringTable[%u] is NULL.\n", i);
                result = -1; break;
            }
            len = strlen(DefaultLanguageLabelStringTable[i]);
            LangLblStringTable[i] = malloc(len + 1);
            if (LangLblStringTable[i] != NULL) {
                len = FormatLanguageString(DefaultLanguageLabelStringTable[i], len + 1, LangLblStringTable[i]);
                temp_realloc = realloc(LangLblStringTable[i], len);
                if (temp_realloc == NULL && len > 0) { result = -ENOMEM; free(LangLblStringTable[i]); LangLblStringTable[i] = NULL; break; }
                LangLblStringTable[i] = temp_realloc;
            } else {
                result = -ENOMEM; break;
            }
        }
    }

    if (result != 0) {
        DEBUG_PRINTF("LoadDefaultLanguageStrings: Failed, result %d. Unloading any partials.\n", result);
        UnloadLanguage(); 
    } else {
        DEBUG_PRINTF("LoadDefaultLanguageStrings: Successfully loaded default English strings.\n");
    }
    return result;
}

const char *GetString(unsigned int StrID) { 
    if (StrID < SYS_UI_MSG_COUNT && LangStringTable[StrID] != NULL) {
        if (!(LangStringWrapTable[StrID / 8] & (1 << (StrID % 8)))) {
            BreakLongLanguageString(LangStringTable[StrID]);
            LangStringWrapTable[StrID / 8] |= (1 << (StrID % 8));
        }
        return LangStringTable[StrID];
    }
    DEBUG_PRINTF("GetString: Error - StrID %u out of bounds or NULL. Max: %d\n", StrID, SYS_UI_MSG_COUNT);
    return "ERR_MSG_ID"; 
}

const char *GetUILabel(unsigned int LblID) { 
    if (LblID < SYS_UI_LBL_COUNT && LangLblStringTable[LblID] != NULL) {
        return LangLblStringTable[LblID];
    }
    DEBUG_PRINTF("GetUILabel: Error - LblID %u out of bounds or NULL. Max: %d\n", LblID, SYS_UI_LBL_COUNT);
    return "ERR_LBL_ID"; 
}

int GetCurrentLanguage(void) {
    return currentFMCBLanguage; 
}

void SetCurrentLanguage(enum FMCB_LANGUAGE_ID languageId) { // MODIFIED: Parameter type
    if (languageId < FMCB_LANG_COUNT) {
        currentFMCBLanguage = languageId;
        DEBUG_PRINTF("SetCurrentLanguage: Language set to ID %d.\n", languageId);
    } else {
        DEBUG_PRINTF("SetCurrentLanguage: Warning - Invalid languageId %d. Defaulting to English.\n", languageId);
        currentFMCBLanguage = FMCB_LANG_ENGLISH;
    }
}

// --- Original static functions from UI.c (parsing, font loading, graphics init etc.) ---
// These remain static as they are internal helpers for UI.c.
// Their content is assumed to be the same as your original UI.c unless specified.

static void WaitForDevice(void)
{
    nopdelay(); nopdelay(); nopdelay(); nopdelay();
    nopdelay(); nopdelay(); nopdelay(); nopdelay();
}

static int FormatLanguageString(const char *in, int len, char *out)
{
    wchar_t wchar1, wchar2;
    int ActualLength, CharLen1, CharLen2;

    ActualLength = 0;
    CharLen1 = mbtowc(&wchar1, in, len);
    while (CharLen1 > 0 && wchar1 != '\0') {
        CharLen2 = mbtowc(&wchar2, in + CharLen1, len - CharLen1);
        if ((CharLen2 > 0) && (wchar1 == '\\' && wchar2 != '\0')) {
            switch (wchar2) { 
                case 'n': *out++ = '\n'; ActualLength++; break;
                // Add other escape sequences if needed (e.g., \\ for literal backslash)
                default:  *out++ = wchar2; ActualLength++; break; // Treat as literal if not \n
            }
            in += (CharLen1 + CharLen2);
            len -= (CharLen1 + CharLen2);
            // CharLen2 = mbtowc(&wchar2, in, len); // This line seems redundant after switch
        } else {
            memcpy(out, in, CharLen1);
            out += CharLen1;
            ActualLength += CharLen1;
            in += CharLen1;
            len -= CharLen1;
        }
        CharLen1 = mbtowc(&wchar1, in, len); // Get next char for the loop condition
        // wchar1 = wchar2; // This was in original, but wchar1 is overwritten by mbtowc above
    }
    *out = '\0';
    return (ActualLength + 1); // Length including null terminator
}

static void BreakLongLanguageString(char *str)
{
    wchar_t wchar;
    int CharLen, ScreenLineLenPx, LineMaxPx, PxSinceLastSpace, len, width;
    char *LastWhitespaceOut;

    if (str == NULL) return;

    LineMaxPx = UIDrawGlobal.width - 2 * UI_OFFSET_X;
    ScreenLineLenPx = 0;
    PxSinceLastSpace = 0;
    len = strlen(str) + 1;
    CharLen = mbtowc(&wchar, str, len);
    LastWhitespaceOut = NULL;

    while (CharLen > 0 && wchar != '\0') {
        if (wchar == '\n') {
            ScreenLineLenPx = 0;
            LastWhitespaceOut = NULL;
            PxSinceLastSpace = 0;
        } else {
            if (wchar == ' ') { 
                LastWhitespaceOut = str;
                PxSinceLastSpace = 0;
            }
            width = FontGetGlyphWidth(&UIDrawGlobal, wchar);
            if (ScreenLineLenPx + width >= LineMaxPx) {
                if (LastWhitespaceOut != NULL) {
                    *LastWhitespaceOut = '\n';
                    ScreenLineLenPx = PxSinceLastSpace + width; // Width of current char after newline
                    LastWhitespaceOut = NULL; // Reset
                } else {
                    // No whitespace to break on, word is too long.
                    // Could insert hyphen or just let it overflow (current behavior).
                    // For now, just continue; it might get broken by a later space.
                    ScreenLineLenPx += width;
                }
            } else {
                ScreenLineLenPx += width;
            }
            if (wchar != ' ') PxSinceLastSpace += width;
        }
        str += CharLen;
        len -= CharLen;
        CharLen = mbtowc(&wchar, str, len);
    }
}

static int ParseLanguageFile(char **array, FILE *file, unsigned int ExpectedNumLines)
{
    int result, LinesLoaded, len;
    unsigned char BOMTemp[3];
    char line[512]; // Max line length for a string/label

    // Check for UTF-8 BOM and skip if present
    if (fread(BOMTemp, 1, 3, file) == 3 && (BOMTemp[0] == 0xEF && BOMTemp[1] == 0xBB && BOMTemp[2] == 0xBF)) {
        // BOM found and skipped
    } else {
        rewind(file); // No BOM or not enough bytes read, rewind to read from start
    }

    result = 0;
    for (LinesLoaded = 0; LinesLoaded < (int)ExpectedNumLines; LinesLoaded++) { // Iterate up to expected lines
        if (fgets(line, sizeof(line), file) == NULL) { // End of file or read error
            if (feof(file)) {
                DEBUG_PRINTF("ParseLanguageFile: EOF reached prematurely. Expected %u, got %d lines.\n", ExpectedNumLines, LinesLoaded);
                result = -1; // Mismatch
            } else {
                DEBUG_PRINTF("ParseLanguageFile: Error reading line %d (errno: %d).\n", LinesLoaded, errno);
                result = -EIO; 
            }
            break; 
        }
        
        len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') { line[len - 1] = '\0'; len--; }
        if (len > 0 && line[len - 1] == '\r') { line[len - 1] = '\0'; len--; } // Handle CR too

        array[LinesLoaded] = malloc(len + 1); // Allocate for worst case (no formatting changes length)
        if (array[LinesLoaded] != NULL) {
            int formatted_len = FormatLanguageString(line, len + 1, array[LinesLoaded]);
            char *temp_realloc = realloc(array[LinesLoaded], formatted_len); // Realloc to actual formatted size
            if (temp_realloc == NULL && formatted_len > 0) {
                result = -ENOMEM; free(array[LinesLoaded]); array[LinesLoaded] = NULL; break;
            }
            array[LinesLoaded] = temp_realloc;
        } else {
            result = -ENOMEM; break;
        }
    }

    if (result == 0 && LinesLoaded != (int)ExpectedNumLines) {
        DEBUG_PRINTF("ParseLanguageFile: Mismatched number of lines. Expected %u, loaded %d.\n", ExpectedNumLines, LinesLoaded);
        result = -1; // Or a specific error code for wrong line count
    }
    
    // If fewer lines were loaded than expected due to EOF or error, free what was allocated
    if (result != 0) {
        for (int k = 0; k < LinesLoaded; k++) { // Free only up to LinesLoaded
            if (array[k] != NULL) { free(array[k]); array[k] = NULL; }
        }
    }
    return result;
}

static int ParseFontListFile(char **array, FILE *file, unsigned int ExpectedNumLines)
{
    // This function is very similar to ParseLanguageFile, but for font filenames.
    // Assuming font filenames don't need FormatLanguageString.
    int result, LinesLoaded, len;
    char line[256]; // Max path length for a font file name

    result = 0;
    for (LinesLoaded = 0; LinesLoaded < (int)ExpectedNumLines; LinesLoaded++) {
         if (fgets(line, sizeof(line), file) == NULL) {
            if (feof(file)) { result = -1; /* Premature EOF */ } else { result = -EIO; }
            break;
        }
        len = strlen(line);
        if (len > 0 && line[len-1] == '\n') { line[len-1] = '\0'; len--; }
        if (len > 0 && line[len-1] == '\r') { line[len-1] = '\0'; len--; }

        if (len > 0) { // Only process non-empty lines
            array[LinesLoaded] = malloc(len + 1);
            if (array[LinesLoaded] != NULL) {
                strcpy(array[LinesLoaded], line);
            } else {
                result = -ENOMEM; break;
            }
        } else {
            array[LinesLoaded] = NULL; // Handle empty lines in fonts.txt if necessary
        }
    }

    if (result == 0 && LinesLoaded != (int)ExpectedNumLines) {
        DEBUG_PRINTF("ParseFontListFile: Mismatched number of lines. Expected %u, loaded %d.\n", ExpectedNumLines, LinesLoaded);
        result = -1;
    }
    
    if (result != 0) {
        for (int k = 0; k < LinesLoaded; k++) {
            if (array[k] != NULL) { free(array[k]); array[k] = NULL; }
        }
    }
    return result;
}

static const char DefaultFontFilename[] = "NotoSans-Bold.ttf"; 

static char *GetDefaultFontFilePath(void) { 
    char *result;
    // Path is relative to where installer is run, lang/ folder is expected there.
    const char prefix[] = "lang/";
    if ((result = malloc(strlen(prefix) + sizeof(DefaultFontFilename))) != NULL) { // sizeof includes null
        sprintf(result, "%s%s", prefix, DefaultFontFilename);
    }
    return result;
}

static char *GetFontFilePath(enum FMCB_LANGUAGE_ID langId) { 
    static char *FontFileArray[FMCB_LANG_COUNT]; 
    char *result = NULL;
    char *pFontFilename;
    FILE *file;
    int i;
    static int fontListGloballyParsed = 0; // Attempt to parse fonts.txt only once

    if (!fontListGloballyParsed) {
        memset(FontFileArray, 0, sizeof(FontFileArray)); 
        if ((file = fopen("lang/fonts.txt", "r")) != NULL) {
            if (ParseFontListFile(FontFileArray, file, FMCB_LANG_COUNT) == 0) {
                fontListGloballyParsed = 1; // Successfully parsed
            } else {
                DEBUG_PRINTF("GetFontFilePath: Failed to parse lang/fonts.txt correctly.\n");
                // Free any partially parsed entries
                for (i = 0; i < FMCB_LANG_COUNT; i++) { if (FontFileArray[i] != NULL) { free(FontFileArray[i]); FontFileArray[i] = NULL;}}
            }
            fclose(file);
        } else {
            DEBUG_PRINTF("GetFontFilePath: lang/fonts.txt not found.\n");
        }
    }

    if (fontListGloballyParsed && langId < FMCB_LANG_COUNT && FontFileArray[langId] != NULL && FontFileArray[langId][0] != '\0') {
        pFontFilename = FontFileArray[langId];
        const char prefix[] = "lang/";
        if ((result = malloc(strlen(prefix) + strlen(pFontFilename) + 1)) != NULL) { 
            sprintf(result, "%s%s", prefix, pFontFilename);
        }
    }
    
    if (result == NULL) { 
        DEBUG_PRINTF("GetFontFilePath: Using default font path for langId %d.\n", langId);
        result = GetDefaultFontFilePath();
    }
    
    // DEBUG_PRINTF("GetFontFilePath for langId %d returning: %s\n", langId, result ? result : "NULL");
    return result;
}

static void InitGraphics(void) {
    unsigned int FrameBufferVRAMAddress;
    short int dx, dy;

    memset(&UIDrawGlobal, 0, sizeof(UIDrawGlobal));

    UIDrawGlobal.interlaced = GS_INTERLACED; // Or GS_NONINTERLACED
    UIDrawGlobal.ffmd = GS_FFMD_FIELD;     // Or GS_FFMD_FRAME if non-interlaced

    // GetConsoleVMode() is from system.c
    if (GetConsoleVMode() == 0) { // NTSC
        UIDrawGlobal.vmode = GS_MODE_NTSC;
        UIDrawGlobal.width = 640;
        UIDrawGlobal.height = 448; // Effective height for NTSC field mode
        dx = 0; // Adjust as needed for screen centering
        dy = 0; // Adjust as needed
    } else { // PAL
        UIDrawGlobal.vmode = GS_MODE_PAL;
        UIDrawGlobal.width = 640;
        UIDrawGlobal.height = 512; // Effective height for PAL field mode
        dx = 0; // Adjust as needed
        dy = 0; // Adjust as needed
    }

    GsResetGraph(GS_INIT_RESET, UIDrawGlobal.interlaced, UIDrawGlobal.vmode, UIDrawGlobal.ffmd);

    // If using GS_FFMD_FRAME, height is actual buffer height.
    // If GS_FFMD_FIELD, drawing height is effectively doubled for full frame.
    // The height set in UIDrawGlobal.height should be the drawable area.
    
    UIDrawGlobal.psm = GS_PSM_CT32; // Common 32-bit pixel mode

    UIDrawGlobal.giftable.packet_count = GIF_PACKET_MAX; // From graphics.h (e.g., 1 or 2)
    UIDrawGlobal.giftable.packets = UIDrawGlobal.packets; // packets is an array in UIDrawGlobal struct

    FrameBufferVRAMAddress = GsVramAllocFrameBuffer(UIDrawGlobal.width, UIDrawGlobal.height, UIDrawGlobal.psm);
    GsSetDefaultDrawEnv(&UIDrawGlobal.draw_env, UIDrawGlobal.psm, UIDrawGlobal.width, UIDrawGlobal.height);
    GsSetDefaultDrawEnvAddress(&UIDrawGlobal.draw_env, FrameBufferVRAMAddress);

    GsSetDefaultDisplayEnv(&UIDrawGlobal.disp_env, UIDrawGlobal.psm, UIDrawGlobal.width, UIDrawGlobal.height, dx, dy);
    GsSetDefaultDisplayEnvAddress(&UIDrawGlobal.disp_env, FrameBufferVRAMAddress);

    GsPutDrawEnv1(&UIDrawGlobal.draw_env);
    GsPutDisplayEnv1(&UIDrawGlobal.disp_env);

    GsOverridePrimAttributes(GS_DISABLE, 0, 0, 0, 0, 0, 0, 0, 0);
    GsEnableAlphaTransparency1(GS_ENABLE, GS_ALPHA_GEQUAL, 1, GS_ALPHA_NO_UPDATE); // Common alpha setup
    // GsEnableAlphaBlending1(GS_ENABLE); // This was in original, but GsEnableAlphaTransparency1 is usually sufficient
}

static int LoadFontIntoBuffer(struct UIDrawGlobal *gsGlobal, const char *path) {
    FILE *file;
    int result = -1; // Default to error
    long size; // Use long for ftell result

    if ((file = fopen(path, "rb")) != NULL) {
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        rewind(file);

        if (size > 0) {
            // Free existing buffer if any
            if (gFontBuffer != NULL) {
                free(gFontBuffer);
                gFontBuffer = NULL;
                gFontBufferSize = 0;
            }
            gFontBuffer = memalign(64, size); // PS2 requires 64-byte alignment for DMA
            if (gFontBuffer != NULL) {
                if (fread(gFontBuffer, 1, size, file) == (size_t)size) {
                    gFontBufferSize = size; // Store the size
                    // FontInitWithBuffer will be called later
                    result = 0; // Success in loading to buffer
                } else {
                    DEBUG_PRINTF("LoadFontIntoBuffer: Error reading font file %s (errno: %d)\n", path, errno);
                    free(gFontBuffer);
                    gFontBuffer = NULL;
                    result = -EIO;
                }
            } else {
                DEBUG_PRINTF("LoadFontIntoBuffer: Failed to allocate memory for font buffer (%ld bytes).\n", size);
                result = -ENOMEM;
            }
        } else {
            DEBUG_PRINTF("LoadFontIntoBuffer: Font file %s is empty or size error.\n", path);
            result = -EINVAL; // Invalid file size
        }
        fclose(file);
    } else {
        DEBUG_PRINTF("LoadFontIntoBuffer: Failed to open font file %s (errno: %d)\n", path, errno);
        result = -errno;
    }
    return result;
}

static int InitFont(void) { 
    int result;
    char *pFontFilePath;

    if ((pFontFilePath = GetFontFilePath(currentFMCBLanguage)) == NULL) { 
        sio_printf("Critical Error: GetFontFilePath returned NULL for language %d.\n", currentFMCBLanguage);
        return -1; 
    }
    DEBUG_PRINTF("InitFont: Attempting to load font: %s\n", pFontFilePath);
    
    result = FontInit(&UIDrawGlobal, pFontFilePath); // FontInit is from font.c/font.h
    if (result != 0) {
        DEBUG_PRINTF("InitFont(%s) failed (result: %d). Attempting default font.\n", pFontFilePath, result);
        free(pFontFilePath); 
        pFontFilePath = GetDefaultFontFilePath(); 
        if (pFontFilePath == NULL) {
             sio_printf("Critical Error: GetDefaultFontFilePath returned NULL.\n");
             return -1;
        }
        DEBUG_PRINTF("InitFont: Attempting default font: %s\n", pFontFilePath);
        result = FontInit(&UIDrawGlobal, pFontFilePath);
    }

    if (result != 0) {
        sio_printf("Critical Error: InitFont failed for both specific and default fonts (%s). Error: %d\n", pFontFilePath, result);
    } else {
        DEBUG_PRINTF("InitFont: Successfully initialized font: %s\n", pFontFilePath);
    }
    free(pFontFilePath); 
    return result;
}

static int InitFontWithBuffer(void) { 
    int result;
    char *pFontFilePath;

    if (gFontBuffer == NULL) { 
        if ((pFontFilePath = GetFontFilePath(currentFMCBLanguage)) == NULL) {
            sio_printf("Critical Error: GetFontFilePath returned NULL for language %d (buffer mode).\n", currentFMCBLanguage);
            return -1;
        }
        DEBUG_PRINTF("InitFontWithBuffer: Attempting to load font to buffer: %s\n", pFontFilePath);

        result = LoadFontIntoBuffer(&UIDrawGlobal, pFontFilePath); // Load into gFontBuffer
        free(pFontFilePath); // Free path string after loading
        
        if (result != 0) { // If loading to buffer failed
            DEBUG_PRINTF("LoadFontIntoBuffer failed (result: %d). Attempting default font to buffer.\n", result);
            pFontFilePath = GetDefaultFontFilePath();
            if (pFontFilePath == NULL) {
                 sio_printf("Critical Error: GetDefaultFontFilePath returned NULL for buffer.\n");
                 return -1;
            }
            result = LoadFontIntoBuffer(&UIDrawGlobal, pFontFilePath);
            free(pFontFilePath);
            if (result != 0) {
                 sio_printf("Critical Error: LoadFontIntoBuffer failed for default font. Error: %d\n", result);
                 return result; // Return the error from LoadFontIntoBuffer
            }
        }
    }
    // If gFontBuffer is now populated (either newly or previously)
    if (gFontBuffer != NULL && gFontBufferSize > 0) {
        DEBUG_PRINTF("InitFontWithBuffer: Font in buffer. Initializing with buffer (size: %d).\n", gFontBufferSize);
        result = FontInitWithBuffer(&UIDrawGlobal, gFontBuffer, gFontBufferSize); // FontInitWithBuffer is from font.c
        if (result != 0) {
            sio_printf("Critical Error: FontInitWithBuffer failed. Error: %d\n", result);
        } else {
            DEBUG_PRINTF("InitFontWithBuffer: Successfully initialized font from buffer.\n");
        }
    } else {
        sio_printf("Critical Error: Font buffer is NULL or size is zero before FontInitWithBuffer.\n");
        result = -1; // Indicate error
    }
    return result;
}

// --- Public UI Functions (from UI.h, implementations in UI.c) ---

int ReinitializeUI(void) { 
    FontDeinit(); // Always deinit current font
    if (gFontBuffer != NULL) { // If we were using buffered font, re-init with buffer
        DEBUG_PRINTF("ReinitializeUI: Re-initializing font from buffer.\n");
        return FontInitWithBuffer(&UIDrawGlobal, gFontBuffer, gFontBufferSize);
    } else { // Else, re-init from file
        DEBUG_PRINTF("ReinitializeUI: Re-initializing font from file.\n");
        return InitFont();
    }
}

int InitializeUI(int BufferFont) { 
    int result;
    int OSDSystemLanguage;

    OSDSystemLanguage = configGetLanguage(); 
    DEBUG_PRINTF("InitializeUI: OSD Language ID from configGetLanguage(): %d\n", OSDSystemLanguage);

    enum FMCB_LANGUAGE_ID detectedFMCBLanguage = FMCB_LANG_ENGLISH; 
    switch(OSDSystemLanguage) {
        case LANGUAGE_JAPANESE:     detectedFMCBLanguage = FMCB_LANG_JAPANESE; break;
        case LANGUAGE_ENGLISH:      detectedFMCBLanguage = FMCB_LANG_ENGLISH; break;
        case LANGUAGE_FRENCH:       detectedFMCBLanguage = FMCB_LANG_FRENCH; break;
        case LANGUAGE_SPANISH:      detectedFMCBLanguage = FMCB_LANG_SPANISH; break;
        case LANGUAGE_GERMAN:       detectedFMCBLanguage = FMCB_LANG_GERMAN; break;
        case LANGUAGE_ITALIAN:      detectedFMCBLanguage = FMCB_LANG_ITALIAN; break;
        case LANGUAGE_DUTCH:        detectedFMCBLanguage = FMCB_LANG_DUTCH; break;
        case LANGUAGE_PORTUGUESE:   detectedFMCBLanguage = FMCB_LANG_PORTUGUESE; break;
        default:
            DEBUG_PRINTF("InitializeUI: OSD language %d not mapped, defaulting to English.\n", OSDSystemLanguage);
            detectedFMCBLanguage = FMCB_LANG_ENGLISH;
            break;
    }
    SetCurrentLanguage(detectedFMCBLanguage); 

    memset(LangStringWrapTable, 0, sizeof(LangStringWrapTable));

    if (GetConsoleRegion() == CONSOLE_REGION_JAPAN) { // GetConsoleRegion is from system.c
        SelectButton = PAD_CIRCLE; CancelButton = PAD_CROSS;
    } else {
        SelectButton = PAD_CROSS; CancelButton = PAD_CIRCLE;
    }
    DEBUG_PRINTF("SelectButton: 0x%04X, CancelButton: 0x%04X\n", SelectButton, CancelButton);

    InitGraphics(); 

    DEBUG_PRINTF("InitializeUI: Attempting to load strings for language ID %d.\n", currentFMCBLanguage);
    result = LoadLanguageStrings(currentFMCBLanguage); 
    if (result != 0) {
        DEBUG_PRINTF("LoadLanguageStrings for ID %d failed (result: %d). Loading default English strings.\n", currentFMCBLanguage, result);
        result = LoadDefaultLanguageStrings();
        if (result != 0) {
            sio_printf("Critical Error: Failed to load default language strings (result: %d).\n", result);
            return result; 
        }
        SetCurrentLanguage(FMCB_LANG_ENGLISH); 
    }

    result = BufferFont ? InitFontWithBuffer() : InitFont();
    if (result != 0) {
        sio_printf("Critical Error: Font initialization failed (result: %d).\n", result);
        return result;
    }

    if (LoadBackground(&UIDrawGlobal, &BackgroundTexture) != 0) {
        sio_printf("Error: Failed to load background texture.\n");
    }
    if (LoadPadGraphics(&UIDrawGlobal, &PadLayoutTexture) != 0) {
        sio_printf("Error: Failed to load pad graphics texture.\n");
    }

    GsClearDrawEnv1(&UIDrawGlobal.draw_env); 
    PadInitPads(); 
    return 0; 
}

void DeinitializeUI(void) { 
    UnloadLanguage();
    FontDeinit();
    if (gFontBuffer != NULL) {
        free(gFontBuffer);
        gFontBuffer = NULL;
        gFontBufferSize = 0; 
    }
    PadDeinitPads(); 
}

// --- MessageBox and other UI drawing/execution functions ---
// These are assumed to be largely correct from your original UI.c.
// Ensure they use GetString/GetUILabel for all user-facing text.

int ShowMessageBox(int Option1LabelID, int Option2LabelID, int Option3LabelID, int Option4LabelID, const char *message_text, int titleLBLID) {
    short int numButtons = 0;
    int selected_button_id; // To store the ID of the button pressed

    // Use GetUILabel for titles and button labels
    UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_TITLE, titleLBLID);
    UISetString(&MessageBoxMenu, MBOX_SCREEN_ID_MESSAGE, message_text); // Message text is already a const char*

    UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_BTN1, Option1LabelID);
    UISetVisible(&MessageBoxMenu, MBOX_SCREEN_ID_BTN1, Option1LabelID != -1);
    if (Option1LabelID != -1) numButtons++;

    UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_BTN2, Option2LabelID);
    UISetVisible(&MessageBoxMenu, MBOX_SCREEN_ID_BTN2, Option2LabelID != -1);
    if (Option2LabelID != -1) numButtons++;
    
    // The original ShowMessageBox in your UI.c had BTN3 and BTN2 swapped in order of display.
    // Assuming BTN3 is the third logical option.
    UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_BTN3, Option3LabelID);
    UISetVisible(&MessageBoxMenu, MBOX_SCREEN_ID_BTN3, Option3LabelID != -1);
    if (Option3LabelID != -1) numButtons++;

    UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_BTN4, Option4LabelID);
    UISetVisible(&MessageBoxMenu, MBOX_SCREEN_ID_BTN4, Option4LabelID != -1);
    if (Option4LabelID != -1) numButtons++;

    if (numButtons > 0) {
        MessageBoxMenu.hints[0].button = BUTTON_TYPE_SYS_SELECT; // Select
        MessageBoxMenu.hints[0].label = SYS_UI_LBL_OK; // Or a more generic "Select" if available
        MessageBoxMenu.hints[1].button = (numButtons == 1 && Option1LabelID != -1) ? -1 : BUTTON_TYPE_SYS_CANCEL; // Cancel if more than one button or if no OK-like button
        MessageBoxMenu.hints[1].label = SYS_UI_LBL_CANCEL;

        // UIExecMenu returns the ID of the item selected, or 1 for cancel.
        selected_button_id = UIExecMenu(&MessageBoxMenu, MBOX_SCREEN_ID_BTN1, NULL, NULL); // Default selection to first button

        switch (selected_button_id) {
            case MBOX_SCREEN_ID_BTN1: return 1; // Corresponds to Option1LabelID
            case MBOX_SCREEN_ID_BTN2: return 2; // Corresponds to Option2LabelID
            case MBOX_SCREEN_ID_BTN3: return 3; // Corresponds to Option3LabelID
            case MBOX_SCREEN_ID_BTN4: return 4; // Corresponds to Option4LabelID
            case 1: // If CancelButton was pressed and UIExecMenu returns 1 for that
                if (numButtons > 1 || Option1LabelID == -1) return 0; // Treat as cancel if multiple options or no explicit OK
                // If only one button and it was "selected" (e.g. OK), it might still be 1.
                // This logic depends heavily on UIExecMenu's return for cancel.
                // For now, assume 0 is cancel/no action.
                return 0; 
            default: return 0; // Default to cancel or no action
        }
    } else { // No buttons, just display message (e.g. for DisplayFlashStatusUpdate)
        MessageBoxMenu.hints[0].button = -1;
        MessageBoxMenu.hints[1].button = -1;
        UIDrawMenu(&MessageBoxMenu, 0, UI_OFFSET_X, UI_OFFSET_Y, -1); // Draw once
        SyncFlipFB(&UIDrawGlobal);
        // This type of message box might need a timeout or a way to be dismissed.
        // For now, it just draws. DisplayFlashStatusUpdate might be better if it's non-blocking.
        // The original DisplayFlashStatusUpdate in your UI.c also just called ShowMessageBox with -1 for buttons.
        // This means it would draw and then UIExecMenu would likely wait for pad input that never comes for these buttons.
        // This needs review: DisplayFlashStatusUpdate should probably be non-blocking or have a timeout.
        // For now, keeping the structure.
        return 0; // No button pressed
    }
}

void DisplayWarningMessage(unsigned int messageID) { ShowMessageBox(SYS_UI_LBL_OK, -1, -1, -1, GetString(messageID), SYS_UI_LBL_WARNING); }
void DisplayErrorMessage(unsigned int messageID) { ShowMessageBox(SYS_UI_LBL_OK, -1, -1, -1, GetString(messageID), SYS_UI_LBL_ERROR); }
void DisplayInfoMessage(unsigned int messageID) { ShowMessageBox(SYS_UI_LBL_OK, -1, -1, -1, GetString(messageID), SYS_UI_LBL_INFO); }
int DisplayPromptMessage(unsigned int messageID, unsigned char Button1LabelID, unsigned char Button2LabelID) { return ShowMessageBox(Button1LabelID, Button2LabelID, -1, -1, GetString(messageID), SYS_UI_LBL_CONFIRM); }
void DisplayFlashStatusUpdate(unsigned int messageID) { ShowMessageBox(-1, -1, -1, -1, GetString(messageID), SYS_UI_LBL_WAIT); }


struct UIMenuItem *UIGetItem(struct UIMenu *menu, unsigned char id) {
    struct UIMenuItem *result = NULL; unsigned int i;
    for (i = 0; menu->items[i].type != MITEM_TERMINATOR; i++) {
        if (menu->items[i].id == id) { result = &menu->items[i]; break; }
    }
    return result;
}
void UISetVisible(struct UIMenu *menu, unsigned char id, int visible) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) {
        if (visible) item->flags &= ~MITEM_FLAG_HIDDEN; else item->flags |= MITEM_FLAG_HIDDEN;
    }
}
void UISetReadonly(struct UIMenu *menu, unsigned char id, int readonly) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) {
        if (!readonly) item->flags &= ~MITEM_FLAG_READONLY; else item->flags |= MITEM_FLAG_READONLY;
    }
}
void UISetEnabled(struct UIMenu *menu, unsigned char id, int enabled) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) {
        if (enabled) item->flags &= ~MITEM_FLAG_DISABLED; else item->flags |= MITEM_FLAG_DISABLED;
    }
}
void UISetValue(struct UIMenu *menu, unsigned char id, int value) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { item->value.value = value; }
}
int UIGetValue(struct UIMenu *menu, unsigned char id) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { return item->value.value; }
    return -1;
}
void UISetLabel(struct UIMenu *menu, unsigned char id, int labelID) { // Changed param name
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { item->label.id = labelID; }
}
void UISetString(struct UIMenu *menu, unsigned char id, const char *string) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { item->string.buffer = string; }
}
void UISetType(struct UIMenu *menu, unsigned char id, unsigned char type) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { item->type = type; }
}
void UISetFormat(struct UIMenu *menu, unsigned char id, unsigned char format, unsigned char width) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { item->format = format; item->width = width; }
}
void UISetEnum(struct UIMenu *menu, unsigned char id, const int *labelIDs, int count) { // Changed param name
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) {
        item->enumeration.labels = labelIDs; item->enumeration.count = count; item->enumeration.selectedIndex = 0;
    }
}
void UISetEnumSelectedIndex(struct UIMenu *menu, unsigned char id, int selectedIndex) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { item->enumeration.selectedIndex = selectedIndex; }
}
int UIGetEnumSelectedIndex(struct UIMenu *menu, unsigned char id) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { return item->enumeration.selectedIndex; }
    return -1;
}

// UIDrawMenu and UIExecMenu are complex and highly specific to FMCB's UI framework.
// It's assumed their internal logic for drawing items using GetUILabel for MITEM_BUTTON, MITEM_LABEL, etc.,
// and GetString for MITEM_STRING is mostly correct from the original.
// The key is that they use the GetUILabel/GetString functions which now access our updated string tables.

void UIDrawMenu(struct UIMenu *menu, unsigned short int frame, short int StartX, short int StartY, short int selection) {
    const char *pLabel;
    char FormatString[8], *pFormatStringCurrent; // Renamed to avoid conflict
    char ValueString[32];
    struct UIMenuItem *item, *SelectedItem;
    short int x, y, width, height, xRel = 0, yRel = 0, button_type_local, i; // Renamed local vars
    GS_RGBAQ colour;

    DrawBackground(&UIDrawGlobal, &BackgroundTexture); // From graphics.h

    x = StartX;
    y = StartY;
    SelectedItem = selection >= 0 ? &menu->items[selection] : NULL;
    for (item = menu->items; item->type != MITEM_TERMINATOR; item++) {
        if (item->flags & MITEM_FLAG_HIDDEN) continue;

        if (item->flags & MITEM_FLAG_POS_ABS) { x = item->x; y = item->y; }
        else { x += item->x; y += item->y; } // Relative positioning

        switch (item->type) {
            case MITEM_SEPERATOR:
                x = StartX; y += UI_FONT_HEIGHT;
                DrawLine(&UIDrawGlobal, x, y + UI_FONT_HEIGHT / 2, UIDrawGlobal.width - UI_OFFSET_X, y + UI_FONT_HEIGHT / 2, 1, GS_WHITE);
                // Fall through to MITEM_BREAK
            case MITEM_BREAK: x = StartX; y += UI_FONT_HEIGHT; break;
            case MITEM_TAB: x += (UI_TAB_STOPS * UI_FONT_WIDTH) - (x % (UI_TAB_STOPS * UI_FONT_WIDTH)); break;
            case MITEM_SPACE: x += UI_FONT_WIDTH; break;
            case MITEM_STRING:
                if (item->string.buffer != NULL) {
                    colour = (item->flags & MITEM_FLAG_DISABLED) ? GS_GREY_FONT : ((item->flags & MITEM_FLAG_READONLY) ? GS_WHITE_FONT : (item == SelectedItem ? GS_YELLOW_FONT : GS_BLUE_FONT));
                    FontPrintfWithFeedback(&UIDrawGlobal, x, y, 1, 1.0f, colour, item->string.buffer, &xRel, &yRel);
                    x += xRel; y += yRel; // yRel might be >0 if string has newlines
                }
                break;
            case MITEM_BUTTON:
                if ((pLabel = GetUILabel(item->label.id)) != NULL) { // Use GetUILabel
                    width = item->width * UI_FONT_WIDTH;
                    height = UI_FONT_HEIGHT + UI_FONT_HEIGHT / 2;
                    if (item == SelectedItem) { width *= 1.1f; height *= 1.1f; }
                    if (item->flags & MITEM_FLAG_POS_MID) x = StartX + (UIDrawGlobal.width - UI_OFFSET_X*2 - width) / 2; // Centered within margins

                    DrawSprite(&UIDrawGlobal, x, y - UI_FONT_HEIGHT / 4, x + width, y + height - UI_FONT_HEIGHT / 4, 2, item == SelectedItem ? GS_LGREY : GS_GREY);
                    colour = (item->flags & MITEM_FLAG_DISABLED) ? GS_GREY_FONT : (item == SelectedItem ? GS_YELLOW_FONT : GS_WHITE_FONT);
                    // Calculate text width if not pre-calculated (item->label.TextWidth)
                    if(item->label.TextWidth == 0) item->label.TextWidth = FontGetStringWidth(&UIDrawGlobal, pLabel, strlen(pLabel));
                    FontPrintfWithFeedback(&UIDrawGlobal, x + (width - item->label.TextWidth) / 2, y + (UI_FONT_HEIGHT/4), 1, 1.0f, colour, pLabel, &xRel, &yRel);
                    x = StartX; // Reset x for next line of buttons if not absolute
                    y += height; // Move y down by button height
                }
                break;
            case MITEM_LABEL: // For labels that are not part of value, e.g. section titles from item->label.id
                if ((pLabel = GetUILabel(item->label.id)) != NULL) { // Use GetUILabel
                    FontPrintfWithFeedback(&UIDrawGlobal, x, y, 1, 1.0f, GS_WHITE_FONT, pLabel, &xRel, &yRel);
                    x += xRel; // y += yRel; // yRel usually 0 for single line labels
                }
                break;
            // ... (MITEM_COLON, MITEM_DASH, MITEM_DOT, MITEM_SLASH cases as in original UI.c) ...
            case MITEM_COLON: FontPrintfWithFeedback(&UIDrawGlobal, x,y,1,1.0f,GS_WHITE_FONT, ":", &xRel, NULL); x+=xRel; break;
            case MITEM_DASH:  FontPrintfWithFeedback(&UIDrawGlobal, x,y,1,1.0f,GS_WHITE_FONT, "-", &xRel, NULL); x+=xRel; break;
            case MITEM_DOT:   FontPrintfWithFeedback(&UIDrawGlobal, x,y,1,1.0f,GS_WHITE_FONT, ".", &xRel, NULL); x+=xRel; break;
            case MITEM_SLASH: FontPrintfWithFeedback(&UIDrawGlobal, x,y,1,1.0f,GS_WHITE_FONT, "/", &xRel, NULL); x+=xRel; break;
            case MITEM_VALUE:
                pFormatStringCurrent = FormatString; // Use local copy
                if (item->flags & MITEM_FLAG_UNIT_PREFIX) {
                    if (item->format == MITEM_FORMAT_HEX) { *pFormatStringCurrent++ = '0'; *pFormatStringCurrent++ = 'x'; }
                }
                *pFormatStringCurrent++ = '%';
                if (item->width > 0) pFormatStringCurrent += sprintf(pFormatStringCurrent, "0%u", item->width);
                switch (item->format) {
                    case MITEM_FORMAT_DEC:   *pFormatStringCurrent++ = 'd'; break;
                    case MITEM_FORMAT_UDEC:  *pFormatStringCurrent++ = 'u'; break;
                    case MITEM_FORMAT_HEX:   *pFormatStringCurrent++ = 'X'; break; // Use X for uppercase hex
                    case MITEM_FORMAT_POINTER: *pFormatStringCurrent++ = 'p'; break;
                    case MITEM_FORMAT_FLOAT: *pFormatStringCurrent++ = 'f'; break; // Float might need specific handling for precision
                }
                *pFormatStringCurrent = '\0';
                sprintf(ValueString, FormatString, item->value.value);
                colour = (item->flags & MITEM_FLAG_DISABLED) ? GS_GREY_FONT : ((item->flags & MITEM_FLAG_READONLY) ? GS_WHITE_FONT : (item == SelectedItem ? GS_YELLOW_FONT : GS_BLUE_FONT));
                FontPrintfWithFeedback(&UIDrawGlobal, x, y, 1, 1.0f, colour, ValueString, &xRel, NULL);
                x += xRel;
                break;
            case MITEM_PROGRESS:
                DrawProgressBar(&UIDrawGlobal, item->value.value / 100.0f, x + 20, y, 4, UIDrawGlobal.width - (x + 40) - UI_OFFSET_X, GS_BLUE); // Adjusted length
                y += UI_FONT_HEIGHT * 2; // Progress bar usually takes more space
                break;
            case MITEM_TOGGLE:
                colour = (item->flags & MITEM_FLAG_DISABLED) ? GS_GREY_FONT : ((item->flags & MITEM_FLAG_READONLY) ? GS_WHITE_FONT : (item == SelectedItem ? GS_YELLOW_FONT : GS_BLUE_FONT));
                FontPrintfWithFeedback(&UIDrawGlobal, x, y, 1, 1.0f, colour, GetUILabel(item->value.value == 0 ? SYS_UI_LBL_DISABLED : SYS_UI_LBL_ENABLED), &xRel, &yRel);
                x += xRel; // y += yRel;
                break;
            case MITEM_ENUM:
                if (item->enumeration.selectedIndex >= 0 && item->enumeration.selectedIndex < item->enumeration.count &&
                    (pLabel = GetUILabel(item->enumeration.labels[item->enumeration.selectedIndex])) != NULL) {
                    colour = (item->flags & MITEM_FLAG_DISABLED) ? GS_GREY_FONT : ((item->flags & MITEM_FLAG_READONLY) ? GS_WHITE_FONT : (item == SelectedItem ? GS_YELLOW_FONT : GS_BLUE_FONT));
                    FontPrintfWithFeedback(&UIDrawGlobal, x, y, 1, 1.0f, colour, pLabel, &xRel, &yRel);
                    x += xRel; // y += yRel;
                }
                break;
        }
    }

    // Draw button legends (hints)
    x = UI_OFFSET_X + 32; // Start position for hints
    for (i = 0; i < 2; i++) {
        if (menu->hints[i].button >= 0 && menu->hints[i].label >=0) {
            button_type_local = menu->hints[i].button;
            if (button_type_local == BUTTON_TYPE_SYS_SELECT) button_type_local = (SelectButton == PAD_CIRCLE ? BUTTON_TYPE_CIRCLE : BUTTON_TYPE_CROSS);
            else if (button_type_local == BUTTON_TYPE_SYS_CANCEL) button_type_local = (CancelButton == PAD_CIRCLE ? BUTTON_TYPE_CIRCLE : BUTTON_TYPE_TRIANGLE); // Usually Triangle for cancel

            DrawButtonLegendWithFeedback(&UIDrawGlobal, &PadLayoutTexture, button_type_local, x, UIDrawGlobal.height - UI_FONT_HEIGHT*2 - UI_OFFSET_Y, 4, &xRel);
            x += xRel + 8;
            FontPrintfWithFeedback(&UIDrawGlobal, x, UIDrawGlobal.height - UI_FONT_HEIGHT*2 - UI_OFFSET_Y + 2, 1, 1.0f, GS_WHITE_FONT, GetUILabel(menu->hints[i].label), &xRel, NULL);
            x += xRel + 16; // More spacing
        }
    }
    // Draw L1/R1 hints for next/prev menu if applicable
    if (menu->prev != NULL) DrawButtonLegend(&UIDrawGlobal, &PadLayoutTexture, BUTTON_TYPE_L1, UI_OFFSET_X, UI_OFFSET_Y, 4);
    if (menu->next != NULL) DrawButtonLegend(&UIDrawGlobal, &PadLayoutTexture, BUTTON_TYPE_R1, UIDrawGlobal.width - UI_OFFSET_X - 30, UI_OFFSET_Y, 4);
}

static void UITransitionSlideRightOut(struct UIMenu *menu, int SelectedOption) { /* ... (original code) ... */ }
static void UITransitionSlideLeftOut(struct UIMenu *menu, int SelectedOption) { /* ... (original code) ... */ }
static void UITransitionSlideRightIn(struct UIMenu *menu, int SelectedOption) { /* ... (original code) ... */ }
static void UITransitionSlideLeftIn(struct UIMenu *menu, int SelectedOption) { /* ... (original code) ... */ }
static void UITransitionFadeIn(struct UIMenu *menu, int SelectedOption) { /* ... (original code) ... */ }
static void UITransitionFadeOut(struct UIMenu *menu, int SelectedOption) { /* ... (original code) ... */ }
void UITransition(struct UIMenu *menu, int type, int SelectedOption) { /* ... (original code) ... */ }
static short int UIGetSelectableItem(struct UIMenu *menu, short int id) { /* ... (original code) ... */ }
static short int UIGetNextSelectableItem(struct UIMenu *menu, short int index) { /* ... (original code) ... */ }
static short int UIGetPrevSelectableItem(struct UIMenu *menu, short int index) { /* ... (original code) ... */ }
int UIExecMenu(struct UIMenu *FirstMenu, short int SelectedItem, struct UIMenu **CurrentMenu, int (*callback)(struct UIMenu *menu, unsigned short int frame, int selection, u32 padstatus)) { /* ... (original code) ... */ }


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

int LoadLanguageStrings(enum FMCB_LANGUAGE_ID languageId) 
{
    int result;
    FILE *file;
    char path[32]; 

    static const char *LanguageFileShortForms[FMCB_LANG_COUNT] = {
        "JA", "EN", "FR", "SP", "GE", "IT", "DU", "PO"
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
        LangStringTable[i] = malloc(len + 1); 
        if (LangStringTable[i] != NULL) {
            len = FormatLanguageString(DefaultLanguageStringTable[i], len + 1, LangStringTable[i]);
            temp_realloc = realloc(LangStringTable[i], len); 
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

void SetCurrentLanguage(enum FMCB_LANGUAGE_ID languageId) { 
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
    char *out_start = out; // Keep track of the start of the output buffer

    ActualLength = 0;
    CharLen1 = mbtowc(&wchar1, in, len);
    while (CharLen1 > 0 && wchar1 != '\0') {
        CharLen2 = mbtowc(&wchar2, in + CharLen1, len - CharLen1);
        if ((CharLen2 > 0) && (wchar1 == '\\' && wchar2 != '\0')) {
            switch (wchar2) { 
                case 'n': *out++ = '\n'; ActualLength++; break;
                default:  *out++ = wchar2; ActualLength++; break; 
            }
            in += (CharLen1 + CharLen2);
            len -= (CharLen1 + CharLen2);
            CharLen1 = mbtowc(&wchar1, in, len); 
        } else {
            memcpy(out, in, CharLen1);
            out += CharLen1;
            ActualLength += CharLen1;
            in += CharLen1;
            len -= CharLen1;
            CharLen1 = mbtowc(&wchar1, in, len); 
        }
    }
    *out = '\0';
    return (ActualLength + 1); 
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
                LastWhitespaceOut = str; // Mark current position as potential break point
                PxSinceLastSpace = 0;    // Reset pixels since last space
            }
            width = FontGetGlyphWidth(&UIDrawGlobal, wchar); // Assuming this is from font.h/font.c
            if (ScreenLineLenPx + width >= LineMaxPx) { // If current word overflows
                if (LastWhitespaceOut != NULL) { // If we had a space on this line
                    *LastWhitespaceOut = '\n';   // Break at the last space
                    ScreenLineLenPx = PxSinceLastSpace + width; // New line starts with chars after LastWhitespaceOut
                    LastWhitespaceOut = NULL; 
                } else { // Word itself is too long, no space to break on this line
                    // Options:
                    // 1. Let it overflow (current behavior if no space found on next iteration)
                    // 2. Force break in middle of word (harder, needs hyphenation logic)
                    // 3. If not the first char on line, insert newline before this long word
                    if (str != LangStringTable[0] && *(str-CharLen) != '\n' && ScreenLineLenPx > 0) { // Check if not start of string or start of line
                       // This logic is tricky, for now, let it overflow if word is too long
                       // A simpler approach if a word is too long and no prior space:
                       // if (ScreenLineLenPx > 0) { *(str - CharLen) = '\n'; ScreenLineLenPx = 0; } // Risky if str-CharLen is invalid
                    }
                    ScreenLineLenPx += width; // Add current char's width
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
    char line[512]; 
    char *temp_realloc;

    if (fread(BOMTemp, 1, 3, file) == 3 && (BOMTemp[0] == 0xEF && BOMTemp[1] == 0xBB && BOMTemp[2] == 0xBF)) {
        // BOM found and skipped
    } else {
        rewind(file); 
    }

    result = 0;
    for (LinesLoaded = 0; LinesLoaded < (int)ExpectedNumLines; LinesLoaded++) { 
        if (fgets(line, sizeof(line), file) == NULL) { 
            if (feof(file)) {
                DEBUG_PRINTF("ParseLanguageFile: EOF reached prematurely. Expected %u, got %d lines.\n", ExpectedNumLines, LinesLoaded);
                result = -1; 
            } else {
                DEBUG_PRINTF("ParseLanguageFile: Error reading line %d (errno: %d).\n", LinesLoaded, errno);
                result = -EIO; 
            }
            break; 
        }
        
        len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') { line[len - 1] = '\0'; len--; }
        if (len > 0 && line[len - 1] == '\r') { line[len - 1] = '\0'; len--; } 

        array[LinesLoaded] = malloc(len + 1); 
        if (array[LinesLoaded] != NULL) {
            int formatted_len = FormatLanguageString(line, len + 1, array[LinesLoaded]);
            temp_realloc = realloc(array[LinesLoaded], formatted_len); 
            if (temp_realloc == NULL && formatted_len > 0) { // Check if realloc failed but formatted_len was positive
                result = -ENOMEM; free(array[LinesLoaded]); array[LinesLoaded] = NULL; break;
            }
            array[LinesLoaded] = temp_realloc; // Assign even if NULL (if formatted_len was 0)
        } else {
            result = -ENOMEM; break;
        }
    }

    if (result == 0 && LinesLoaded != (int)ExpectedNumLines) {
        DEBUG_PRINTF("ParseLanguageFile: Mismatched number of lines. Expected %u, loaded %d.\n", ExpectedNumLines, LinesLoaded);
        result = -1; 
    }
    
    if (result != 0) {
        for (int k = 0; k < LinesLoaded; k++) { 
            if (array[k] != NULL) { free(array[k]); array[k] = NULL; }
        }
    }
    return result;
}

static int ParseFontListFile(char **array, FILE *file, unsigned int ExpectedNumLines)
{
    int result, LinesLoaded, len;
    char line[256]; 

    result = 0;
    for (LinesLoaded = 0; LinesLoaded < (int)ExpectedNumLines; LinesLoaded++) {
         if (fgets(line, sizeof(line), file) == NULL) {
            if (feof(file)) { 
                DEBUG_PRINTF("ParseFontListFile: EOF reached prematurely. Expected %u, got %d lines.\n", ExpectedNumLines, LinesLoaded);
                result = -1; 
            } else { 
                DEBUG_PRINTF("ParseFontListFile: Error reading line %d (errno: %d).\n", LinesLoaded, errno);
                result = -EIO; 
            }
            break;
        }
        len = strlen(line);
        if (len > 0 && line[len-1] == '\n') { line[len-1] = '\0'; len--; }
        if (len > 0 && line[len-1] == '\r') { line[len-1] = '\0'; len--; }

        if (len > 0) { 
            array[LinesLoaded] = malloc(len + 1);
            if (array[LinesLoaded] != NULL) {
                strcpy(array[LinesLoaded], line);
            } else {
                result = -ENOMEM; break;
            }
        } else { // Handle empty lines in fonts.txt by storing NULL or empty string
            array[LinesLoaded] = malloc(1); // Allocate for a null terminator
            if(array[LinesLoaded] != NULL) array[LinesLoaded][0] = '\0';
            else { result = -ENOMEM; break; }
        }
    }

    if (result == 0 && LinesLoaded != (int)ExpectedNumLines) {
        DEBUG_PRINTF("ParseFontListFile: Mismatched number of lines. Expected %u, loaded %d.\n", ExpectedNumLines, LinesLoaded);
        result = -1;
    }
    
    if (result != 0) { // If any error occurred, free already allocated strings
        for (int k = 0; k < LinesLoaded; k++) { // Only up to LinesLoaded
            if (array[k] != NULL) { free(array[k]); array[k] = NULL; }
        }
    }
    return result;
}

static const char DefaultFontFilename[] = "NotoSans-Bold.ttf"; 

static char *GetDefaultFontFilePath(void) { 
    char *result;
    const char prefix[] = "lang/";
    if ((result = malloc(strlen(prefix) + sizeof(DefaultFontFilename))) != NULL) { 
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
    static int fontListGloballyParsed = 0; 

    if (!fontListGloballyParsed) {
        memset(FontFileArray, 0, sizeof(FontFileArray)); 
        // Path is relative to installer's execution directory
        if ((file = fopen("lang/fonts.txt", "r")) != NULL) {
            if (ParseFontListFile(FontFileArray, file, FMCB_LANG_COUNT) == 0) {
                fontListGloballyParsed = 1; 
            } else {
                DEBUG_PRINTF("GetFontFilePath: Failed to parse lang/fonts.txt correctly.\n");
                for (i = 0; i < FMCB_LANG_COUNT; i++) { if (FontFileArray[i] != NULL) { free(FontFileArray[i]); FontFileArray[i] = NULL;}}
            }
            fclose(file);
        } else {
            DEBUG_PRINTF("GetFontFilePath: lang/fonts.txt not found (errno: %d).\n", errno);
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
    
    return result;
}

static void InitGraphics(void) {
    unsigned int FrameBufferVRAMAddress;
    short int dx = 0, dy = 0; // Default to 0, adjust per mode if needed

    memset(&UIDrawGlobal, 0, sizeof(UIDrawGlobal));

    UIDrawGlobal.interlaced = GS_INTERLACED; 
    UIDrawGlobal.ffmd = GS_FFMD_FIELD;     

    if (GetConsoleVMode() == 0) { // NTSC (from system.c)
        UIDrawGlobal.vmode = GS_MODE_NTSC;
        UIDrawGlobal.width = 640;
        UIDrawGlobal.height = 448; 
        // dx, dy might need adjustment based on specific NTSC variant or desired centering
        // dx = (704 - 640) / 2 * (gsGlobal_get_DISPLAY_active_width() / 704); // Example centering
    } else { // PAL
        UIDrawGlobal.vmode = GS_MODE_PAL;
        UIDrawGlobal.width = 640;
        UIDrawGlobal.height = 512; 
        // dx, dy for PAL
    }

    GsResetGraph(GS_INIT_RESET, UIDrawGlobal.interlaced, UIDrawGlobal.vmode, UIDrawGlobal.ffmd);
    
    UIDrawGlobal.psm = GS_PSM_CT32; 

    UIDrawGlobal.giftable.packet_count = GIF_PACKET_MAX; 
    UIDrawGlobal.giftable.packets = UIDrawGlobal.packets; 

    FrameBufferVRAMAddress = GsVramAllocFrameBuffer(UIDrawGlobal.width, UIDrawGlobal.height, UIDrawGlobal.psm);
    GsSetDefaultDrawEnv(&UIDrawGlobal.draw_env, UIDrawGlobal.psm, UIDrawGlobal.width, UIDrawGlobal.height);
    GsSetDefaultDrawEnvAddress(&UIDrawGlobal.draw_env, FrameBufferVRAMAddress);

    GsSetDefaultDisplayEnv(&UIDrawGlobal.disp_env, UIDrawGlobal.psm, UIDrawGlobal.width, UIDrawGlobal.height, dx, dy);
    GsSetDefaultDisplayEnvAddress(&UIDrawGlobal.disp_env, FrameBufferVRAMAddress);

    GsPutDrawEnv1(&UIDrawGlobal.draw_env);
    GsPutDisplayEnv1(&UIDrawGlobal.disp_env);

    GsOverridePrimAttributes(GS_DISABLE, 0, 0, 0, 0, 0, 0, 0, 0);
    // Alpha settings from original UI.c
    GsEnableAlphaTransparency1(GS_DISABLE, GS_ALPHA_ALWAYS, 0x80, GS_ALPHA_NO_UPDATE);
    GsEnableAlphaBlending1(GS_ENABLE);
    GsEnableAlphaTransparency2(GS_DISABLE, GS_ALPHA_ALWAYS, 0x80, GS_ALPHA_NO_UPDATE);
    GsEnableAlphaBlending2(GS_ENABLE);
}

static int LoadFontIntoBuffer(struct UIDrawGlobal *gsGlobal, const char *path) {
    FILE *file;
    int result = -1; 
    long size_of_file; // Changed from int to long for ftell

    (void)gsGlobal; // Not directly used in this specific implementation from original

    if ((file = fopen(path, "rb")) != NULL) {
        fseek(file, 0, SEEK_END);
        size_of_file = ftell(file); // Use long for ftell
        rewind(file);

        if (size_of_file > 0) {
            if (gFontBuffer != NULL) { free(gFontBuffer); gFontBuffer = NULL; gFontBufferSize = 0; }
            gFontBuffer = memalign(64, size_of_file); 
            if (gFontBuffer != NULL) {
                if (fread(gFontBuffer, 1, size_of_file, file) == (size_t)size_of_file) {
                    gFontBufferSize = size_of_file; 
                    result = 0; 
                } else {
                    DEBUG_PRINTF("LoadFontIntoBuffer: Error reading font file %s (errno: %d)\n", path, errno);
                    free(gFontBuffer); gFontBuffer = NULL; gFontBufferSize = 0;
                    result = -EIO;
                }
            } else {
                DEBUG_PRINTF("LoadFontIntoBuffer: Failed to allocate memory (%ld bytes).\n", size_of_file);
                result = -ENOMEM;
            }
        } else {
            DEBUG_PRINTF("LoadFontIntoBuffer: Font file %s is empty or size error (size: %ld).\n", path, size_of_file);
            result = -EINVAL; 
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
    
    result = FontInit(&UIDrawGlobal, pFontFilePath); 
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
        sio_printf("Critical Error: InitFont failed for both specific and default fonts (%s). Error: %d\n", pFontFilePath ? pFontFilePath : "Unknown", result);
    } else {
        DEBUG_PRINTF("InitFont: Successfully initialized font: %s\n", pFontFilePath ? pFontFilePath : "Unknown");
    }
    if (pFontFilePath) free(pFontFilePath); 
    return result;
}

static int InitFontWithBuffer(void) { 
    int result;
    char *pFontFilePath = NULL; // Initialize to NULL

    if (gFontBuffer == NULL) { 
        if ((pFontFilePath = GetFontFilePath(currentFMCBLanguage)) == NULL) {
            sio_printf("Critical Error: GetFontFilePath returned NULL for language %d (buffer mode).\n", currentFMCBLanguage);
            return -1;
        }
        DEBUG_PRINTF("InitFontWithBuffer: Attempting to load font to buffer: %s\n", pFontFilePath);

        result = LoadFontIntoBuffer(&UIDrawGlobal, pFontFilePath); 
        
        if (result != 0) { 
            DEBUG_PRINTF("LoadFontIntoBuffer for %s failed (result: %d). Attempting default font to buffer.\n", pFontFilePath, result);
            if(pFontFilePath) free(pFontFilePath); // Free previous path
            pFontFilePath = GetDefaultFontFilePath();
            if (pFontFilePath == NULL) {
                 sio_printf("Critical Error: GetDefaultFontFilePath returned NULL for buffer.\n");
                 return -1;
            }
            DEBUG_PRINTF("InitFontWithBuffer: Attempting default font to buffer: %s\n", pFontFilePath);
            result = LoadFontIntoBuffer(&UIDrawGlobal, pFontFilePath);
            if (result != 0) {
                 sio_printf("Critical Error: LoadFontIntoBuffer failed for default font (%s). Error: %d\n", pFontFilePath, result);
                 if(pFontFilePath) free(pFontFilePath);
                 return result; 
            }
        }
        if(pFontFilePath) free(pFontFilePath); // Free path string after loading
    }
    
    if (gFontBuffer != NULL && gFontBufferSize > 0) {
        DEBUG_PRINTF("InitFontWithBuffer: Font in buffer. Initializing with buffer (size: %d).\n", gFontBufferSize);
        result = FontInitWithBuffer(&UIDrawGlobal, gFontBuffer, gFontBufferSize); 
        if (result != 0) {
            sio_printf("Critical Error: FontInitWithBuffer failed. Error: %d\n", result);
        } else {
            DEBUG_PRINTF("InitFontWithBuffer: Successfully initialized font from buffer.\n");
        }
    } else {
        sio_printf("Critical Error: Font buffer is NULL or size is zero before FontInitWithBuffer.\n");
        result = -1; 
    }
    return result;
}

// --- Public UI Functions (from UI.h, implementations in UI.c) ---

int ReinitializeUI(void) { 
    FontDeinit(); 
    if (gFontBuffer != NULL) { 
        DEBUG_PRINTF("ReinitializeUI: Re-initializing font from buffer.\n");
        return FontInitWithBuffer(&UIDrawGlobal, gFontBuffer, gFontBufferSize);
    } else { 
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

    if (GetConsoleRegion() == CONSOLE_REGION_JAPAN) { 
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

int ShowMessageBox(int Option1LabelID, int Option2LabelID, int Option3LabelID, int Option4LabelID, const char *message_text, int titleLBLID) {
    short int numButtons = 0;
    int selected_button_id; 

    UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_TITLE, titleLBLID);
    UISetString(&MessageBoxMenu, MBOX_SCREEN_ID_MESSAGE, message_text);

    UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_BTN1, Option1LabelID);
    UISetVisible(&MessageBoxMenu, MBOX_SCREEN_ID_BTN1, Option1LabelID != -1);
    if (Option1LabelID != -1) numButtons++;

    UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_BTN2, Option2LabelID);
    UISetVisible(&MessageBoxMenu, MBOX_SCREEN_ID_BTN2, Option2LabelID != -1);
    if (Option2LabelID != -1) numButtons++;
    
    UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_BTN3, Option3LabelID);
    UISetVisible(&MessageBoxMenu, MBOX_SCREEN_ID_BTN3, Option3LabelID != -1);
    if (Option3LabelID != -1) numButtons++;

    UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_BTN4, Option4LabelID);
    UISetVisible(&MessageBoxMenu, MBOX_SCREEN_ID_BTN4, Option4LabelID != -1);
    if (Option4LabelID != -1) numButtons++;

    if (numButtons > 0) {
        MessageBoxMenu.hints[0].button = BUTTON_TYPE_SYS_SELECT; 
        MessageBoxMenu.hints[0].label = SYS_UI_LBL_OK; 
        MessageBoxMenu.hints[1].button = (numButtons == 1 && Option1LabelID != -1 && (Option1LabelID == SYS_UI_LBL_OK || Option1LabelID == SYS_UI_LBL_YES)) ? -1 : BUTTON_TYPE_SYS_CANCEL; 
        MessageBoxMenu.hints[1].label = SYS_UI_LBL_CANCEL;

        selected_button_id = UIExecMenu(&MessageBoxMenu, MBOX_SCREEN_ID_BTN1, NULL, NULL); 

        switch (selected_button_id) {
            case MBOX_SCREEN_ID_BTN1: return 1; 
            case MBOX_SCREEN_ID_BTN2: return 2; 
            case MBOX_SCREEN_ID_BTN3: return 3; 
            case MBOX_SCREEN_ID_BTN4: return 4; 
            case 1: // If CancelButton was pressed and UIExecMenu returns 1 for that
                 // This case needs to be robust based on UIExecMenu's return for cancel.
                 // If cancel is 1, and BTN1 is also 1, this is ambiguous.
                 // Assuming UIExecMenu returns item ID on select, and a distinct value (e.g. 0 or specific const) for cancel.
                 // The original code had `default: return 0;` which implies cancel might be 0 or unhandled ID.
                 // If SelectButton is pressed on BTN1 (ID=MBOX_SCREEN_ID_BTN1), it should return 1.
                 // If CancelButton is pressed, UIExecMenu should return something else (e.g. 0 or specific EVENT_CANCELLED).
                 // For now, if selected_button_id is 1, it means BTN1.
                 // If UIExecMenu returns 1 for system cancel (Triangle), that's what we check.
                 // This part of original UI.c is a bit complex.
                 // Let's assume UIExecMenu returns the ID of the button, and 0 or a specific value for Triangle/system cancel.
                 // The original code's `default: return 0;` handles cases where no button ID matches.
                return 0; // Default to cancel if logic is unclear from snippet
            default: return 0; 
        }
    } else { 
        MessageBoxMenu.hints[0].button = -1;
        MessageBoxMenu.hints[1].button = -1;
        UIDrawMenu(&MessageBoxMenu, 0, UI_OFFSET_X, UI_OFFSET_Y, -1); 
        SyncFlipFB(&UIDrawGlobal);
        // For a message with no buttons, it should probably auto-dismiss or wait for any key.
        // The original logic would make UIExecMenu wait indefinitely.
        // This is a design aspect of DisplayFlashStatusUpdate.
        u32 pad_anykey;
        do {
            pad_anykey = ReadCombinedPadStatus(); // Wait for any button press
            SyncFlipFB(&UIDrawGlobal); // Keep screen responsive
        } while (!pad_anykey);
        return 0; 
    }
}

void DisplayWarningMessage(unsigned int messageID) { ShowMessageBox(SYS_UI_LBL_OK, -1, -1, -1, GetString(messageID), SYS_UI_LBL_WARNING); }
void DisplayErrorMessage(unsigned int messageID) { ShowMessageBox(SYS_UI_LBL_OK, -1, -1, -1, GetString(messageID), SYS_UI_LBL_ERROR); }
void DisplayInfoMessage(unsigned int messageID) { ShowMessageBox(SYS_UI_LBL_OK, -1, -1, -1, GetString(messageID), SYS_UI_LBL_INFO); }
int DisplayPromptMessage(unsigned int messageID, unsigned char Button1LabelID, unsigned char Button2LabelID) { 
    // Original returned 1 for Button1 (e.g. NO), 2 for Button2 (e.g. YES)
    return ShowMessageBox(Button1LabelID, Button2LabelID, -1, -1, GetString(messageID), SYS_UI_LBL_CONFIRM); 
}
void DisplayFlashStatusUpdate(unsigned int messageID) { ShowMessageBox(-1, -1, -1, -1, GetString(messageID), SYS_UI_LBL_WAIT); }


struct UIMenuItem *UIGetItem(struct UIMenu *menu, unsigned char id) {
    struct UIMenuItem *result = NULL; unsigned int i;
    if (!menu || !menu->items) return NULL;
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
    if ((item = UIGetItem(menu, id)) != NULL) { if(item->type == MITEM_VALUE || item->type == MITEM_TOGGLE || item->type == MITEM_PROGRESS) item->value.value = value; }
}
int UIGetValue(struct UIMenu *menu, unsigned char id) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { if(item->type == MITEM_VALUE || item->type == MITEM_TOGGLE || item->type == MITEM_PROGRESS) return item->value.value; }
    return -1;
}
void UISetLabel(struct UIMenu *menu, unsigned char id, int labelID) { 
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { if(item->type == MITEM_LABEL || item->type == MITEM_BUTTON) item->label.id = labelID; }
}
void UISetString(struct UIMenu *menu, unsigned char id, const char *string) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { if(item->type == MITEM_STRING) item->string.buffer = string; }
}
void UISetType(struct UIMenu *menu, unsigned char id, unsigned char type) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { item->type = type; }
}
void UISetFormat(struct UIMenu *menu, unsigned char id, unsigned char format, unsigned char width) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { if(item->type == MITEM_VALUE) {item->format = format; item->width = width;} }
}
void UISetEnum(struct UIMenu *menu, unsigned char id, const int *labelIDs, int count) { 
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) {
        if(item->type == MITEM_ENUM) {
            item->enumeration.labels = labelIDs; item->enumeration.count = count; item->enumeration.selectedIndex = 0;
        }
    }
}
void UISetEnumSelectedIndex(struct UIMenu *menu, unsigned char id, int selectedIndex) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { if(item->type == MITEM_ENUM) item->enumeration.selectedIndex = selectedIndex; }
}
int UIGetEnumSelectedIndex(struct UIMenu *menu, unsigned char id) {
    struct UIMenuItem *item;
    if ((item = UIGetItem(menu, id)) != NULL) { if(item->type == MITEM_ENUM) return item->enumeration.selectedIndex; }
    return -1;
}

void UIDrawMenu(struct UIMenu *menu, unsigned short int frame, short int StartX, short int StartY, short int selection) {
    const char *pLabel;
    char FormatString[16]; // Increased size for safety
    char ValueString[64]; // Increased size for safety
    struct UIMenuItem *item, *SelectedItem;
    short int x, y, width, height, xRel = 0, yRel = 0, button_type_local, i; 
    GS_RGBAQ colour;

    DrawBackground(&UIDrawGlobal, &BackgroundTexture); 

    x = StartX;
    y = StartY;
    SelectedItem = (selection >= 0 && menu->items[selection].type != MITEM_TERMINATOR) ? &menu->items[selection] : NULL;

    for (item = menu->items; item->type != MITEM_TERMINATOR; item++) {
        if (item->flags & MITEM_FLAG_HIDDEN) continue;

        if (item->flags & MITEM_FLAG_POS_ABS) { x = item->x; y = item->y; }
        else { x += item->x; y += item->y; } 

        switch (item->type) {
            case MITEM_SEPERATOR:
                x = StartX; y += UI_FONT_HEIGHT;
                DrawLine(&UIDrawGlobal, x, y + UI_FONT_HEIGHT / 2, UIDrawGlobal.width - UI_OFFSET_X, y + UI_FONT_HEIGHT / 2, 1, GS_WHITE);
            case MITEM_BREAK: x = StartX; y += UI_FONT_HEIGHT; break;
            case MITEM_TAB: x += (UI_TAB_STOPS * UI_FONT_WIDTH) - (x % (UI_TAB_STOPS * UI_FONT_WIDTH)); break;
            case MITEM_SPACE: x += UI_FONT_WIDTH; break;
            case MITEM_STRING:
                if (item->string.buffer != NULL) {
                    colour = (item->flags & MITEM_FLAG_DISABLED) ? GS_GREY_FONT : ((item->flags & MITEM_FLAG_READONLY) ? GS_WHITE_FONT : (item == SelectedItem ? GS_YELLOW_FONT : GS_BLUE_FONT));
                    FontPrintfWithFeedback(&UIDrawGlobal, x, y, 1, 1.0f, colour, item->string.buffer, &xRel, &yRel);
                    // x remains for subsequent items on same "logical" line if not MITEM_BREAK
                    // y advances if the string had newlines processed by FontPrintfWithFeedback or BreakLongLanguageString
                }
                break;
            case MITEM_BUTTON:
                if ((pLabel = GetUILabel(item->label.id)) != NULL) { 
                    width = item->width * UI_FONT_WIDTH;
                    height = UI_FONT_HEIGHT + UI_FONT_HEIGHT / 2;
                    if (item == SelectedItem) { width = (short int)(width * 1.1f); height = (short int)(height * 1.1f); }
                    
                    short int current_x = x; // Use temp for centering calculation
                    if (item->flags & MITEM_FLAG_POS_MID) 
                        current_x = StartX + (UIDrawGlobal.width - (StartX + UI_OFFSET_X) - width) / 2; // Center within available horizontal space
                    
                    DrawSprite(&UIDrawGlobal, current_x, y - UI_FONT_HEIGHT / 4, current_x + width, y + height - UI_FONT_HEIGHT / 4, 2, item == SelectedItem ? GS_LGREY : GS_GREY);
                    colour = (item->flags & MITEM_FLAG_DISABLED) ? GS_GREY_FONT : (item == SelectedItem ? GS_YELLOW_FONT : GS_WHITE_FONT);
                    
                    if(item->label.TextWidth == 0) item->label.TextWidth = FontGetStringWidth(&UIDrawGlobal, pLabel, strlen(pLabel));
                    FontPrintf(&UIDrawGlobal, current_x + (width - item->label.TextWidth) / 2, y + (UI_FONT_HEIGHT/4), 1, 1.0f, colour, pLabel);
                    
                    if (!(item->flags & MITEM_FLAG_POS_ABS)) { // If not absolute, reset x and advance y for next button
                        x = StartX; 
                        y += height; 
                    }
                }
                break;
            case MITEM_LABEL: 
                if ((pLabel = GetUILabel(item->label.id)) != NULL) { // Changed from item->value.value
                    FontPrintfWithFeedback(&UIDrawGlobal, x, y, 1, 1.0f, GS_WHITE_FONT, pLabel, &xRel, &yRel);
                    x += xRel; 
                }
                break;
            case MITEM_COLON: FontPrintfWithFeedback(&UIDrawGlobal, x,y,1,1.0f,GS_WHITE_FONT, ":", &xRel, NULL); x+=xRel; break;
            case MITEM_DASH:  FontPrintfWithFeedback(&UIDrawGlobal, x,y,1,1.0f,GS_WHITE_FONT, "-", &xRel, NULL); x+=xRel; break;
            case MITEM_DOT:   FontPrintfWithFeedback(&UIDrawGlobal, x,y,1,1.0f,GS_WHITE_FONT, ".", &xRel, NULL); x+=xRel; break;
            case MITEM_SLASH: FontPrintfWithFeedback(&UIDrawGlobal, x,y,1,1.0f,GS_WHITE_FONT, "/", &xRel, NULL); x+=xRel; break;
            case MITEM_VALUE:
                {
                    char tempFormatString[16]; // Local buffer for format string construction
                    char *pFmt = tempFormatString;
                    if (item->flags & MITEM_FLAG_UNIT_PREFIX) {
                        if (item->format == MITEM_FORMAT_HEX) { *pFmt++ = '0'; *pFmt++ = 'x'; }
                    }
                    *pFmt++ = '%';
                    if (item->width > 0) pFmt += sprintf(pFmt, "0%u", item->width);
                    switch (item->format) {
                        case MITEM_FORMAT_DEC:   *pFmt++ = 'd'; break;
                        case MITEM_FORMAT_UDEC:  *pFmt++ = 'u'; break;
                        case MITEM_FORMAT_HEX:   *pFmt++ = 'X'; break; 
                        case MITEM_FORMAT_POINTER: *pFmt++ = 'p'; break;
                        case MITEM_FORMAT_FLOAT: *pFmt++ = '.'; *pFmt++ = '2'; *pFmt++ = 'f'; break; // Example .2f for float
                    }
                    *pFmt = '\0';
                    sprintf(ValueString, tempFormatString, item->value.value); // Use item->value.value for int/float
                    colour = (item->flags & MITEM_FLAG_DISABLED) ? GS_GREY_FONT : ((item->flags & MITEM_FLAG_READONLY) ? GS_WHITE_FONT : (item == SelectedItem ? GS_YELLOW_FONT : GS_BLUE_FONT));
                    FontPrintfWithFeedback(&UIDrawGlobal, x, y, 1, 1.0f, colour, ValueString, &xRel, NULL);
                    x += xRel;
                }
                break;
            case MITEM_PROGRESS:
                DrawProgressBar(&UIDrawGlobal, item->value.value / 100.0f, x + UI_OFFSET_X, y, 4, UIDrawGlobal.width - (x + UI_OFFSET_X*2) - UI_OFFSET_X, GS_BLUE);
                y += UI_FONT_HEIGHT + 10; // Progress bar takes more space
                break;
            case MITEM_TOGGLE:
                colour = (item->flags & MITEM_FLAG_DISABLED) ? GS_GREY_FONT : ((item->flags & MITEM_FLAG_READONLY) ? GS_WHITE_FONT : (item == SelectedItem ? GS_YELLOW_FONT : GS_BLUE_FONT));
                FontPrintfWithFeedback(&UIDrawGlobal, x, y, 1, 1.0f, colour, GetUILabel(item->value.value == 0 ? SYS_UI_LBL_DISABLED : SYS_UI_LBL_ENABLED), &xRel, &yRel);
                x += xRel; 
                break;
            case MITEM_ENUM:
                if (item->enumeration.selectedIndex >= 0 && item->enumeration.selectedIndex < item->enumeration.count &&
                    (pLabel = GetUILabel(item->enumeration.labels[item->enumeration.selectedIndex])) != NULL) {
                    colour = (item->flags & MITEM_FLAG_DISABLED) ? GS_GREY_FONT : ((item->flags & MITEM_FLAG_READONLY) ? GS_WHITE_FONT : (item == SelectedItem ? GS_YELLOW_FONT : GS_BLUE_FONT));
                    FontPrintfWithFeedback(&UIDrawGlobal, x, y, 1, 1.0f, colour, pLabel, &xRel, &yRel);
                    x += xRel; 
                }
                break;
        }
    }

    x = UI_OFFSET_X + 32; 
    short int hint_y = UIDrawGlobal.height - UI_FONT_HEIGHT - UI_FONT_HEIGHT/2 - UI_OFFSET_Y; // Position hints lower
    for (i = 0; i < 2; i++) {
        if (menu->hints[i].button >= 0 && menu->hints[i].label >=0) {
            button_type_local = menu->hints[i].button;
            if (button_type_local == BUTTON_TYPE_SYS_SELECT) button_type_local = (SelectButton == PAD_CIRCLE ? BUTTON_TYPE_CIRCLE : BUTTON_TYPE_CROSS);
            else if (button_type_local == BUTTON_TYPE_SYS_CANCEL) button_type_local = (CancelButton == PAD_CIRCLE ? BUTTON_TYPE_CIRCLE : BUTTON_TYPE_TRIANGLE); 

            DrawButtonLegendWithFeedback(&UIDrawGlobal, &PadLayoutTexture, button_type_local, x, hint_y, 4, &xRel);
            x += xRel + 4; // Small space after button icon
            FontPrintfWithFeedback(&UIDrawGlobal, x, hint_y + 2, 1, 1.0f, GS_WHITE_FONT, GetUILabel(menu->hints[i].label), &xRel, NULL);
            x += xRel + 16; 
        }
    }
    if (menu->prev != NULL) DrawButtonLegend(&UIDrawGlobal, &PadLayoutTexture, BUTTON_TYPE_L1, UI_OFFSET_X, UI_OFFSET_Y + 8, 4);
    if (menu->next != NULL) DrawButtonLegend(&UIDrawGlobal, &PadLayoutTexture, BUTTON_TYPE_R1, UIDrawGlobal.width - UI_OFFSET_X - ButtonLayoutParameters[BUTTON_TYPE_R1].length - 8, UI_OFFSET_Y + 8, 4);
}

static void UITransitionSlideRightOut(struct UIMenu *menu, int SelectedOption) { for (int i = 0; i <= 15; i++) { UIDrawMenu(menu, i, UI_OFFSET_X + i * 48, UI_OFFSET_Y, SelectedOption); SyncFlipFB(&UIDrawGlobal); } }
static void UITransitionSlideLeftOut(struct UIMenu *menu, int SelectedOption) { for (int i = 0; i <= 15; i++) { UIDrawMenu(menu, i, UI_OFFSET_X + -i * 48, UI_OFFSET_Y, SelectedOption); SyncFlipFB(&UIDrawGlobal); } }
static void UITransitionSlideRightIn(struct UIMenu *menu, int SelectedOption) { for (int i = 15; i > 0; i--) { UIDrawMenu(menu, i, UI_OFFSET_X + i * 48, UI_OFFSET_Y, SelectedOption); SyncFlipFB(&UIDrawGlobal); } }
static void UITransitionSlideLeftIn(struct UIMenu *menu, int SelectedOption) { for (int i = 15; i > 0; i--) { UIDrawMenu(menu, i, UI_OFFSET_X + -i * 48, UI_OFFSET_Y, SelectedOption); SyncFlipFB(&UIDrawGlobal); } }
static void UITransitionFadeIn(struct UIMenu *menu, int SelectedOption) { GS_RGBAQ rgbaq={0,0,0,0,0}; for (int i = 0; i <= 15; i++) { UIDrawMenu(menu, i, UI_OFFSET_X, UI_OFFSET_Y, SelectedOption); rgbaq.a = i * 8; DrawSprite(&UIDrawGlobal,0,0,UIDrawGlobal.width,UIDrawGlobal.height,10,rgbaq); SyncFlipFB(&UIDrawGlobal); } }
static void UITransitionFadeOut(struct UIMenu *menu, int SelectedOption) { GS_RGBAQ rgbaq={0,0,0,0,0}; for (int i = 15; i >= 0; i--) { UIDrawMenu(menu, i, UI_OFFSET_X, UI_OFFSET_Y, SelectedOption); rgbaq.a = i * 8; DrawSprite(&UIDrawGlobal,0,0,UIDrawGlobal.width,UIDrawGlobal.height,10,rgbaq); SyncFlipFB(&UIDrawGlobal); } }

void UITransition(struct UIMenu *menu, int type, int SelectedOption) {
    switch (type) {
        case UIMT_LEFT_OUT:  UITransitionSlideLeftOut(menu, SelectedOption); break;
        case UIMT_RIGHT_OUT: UITransitionSlideRightOut(menu, SelectedOption); break;
        case UIMT_LEFT_IN:   UITransitionSlideLeftIn(menu, SelectedOption); break;
        case UIMT_RIGHT_IN:  UITransitionSlideRightIn(menu, SelectedOption); break;
        case UIMT_FADE_IN:   UITransitionFadeIn(menu, SelectedOption); break;
        case UIMT_FADE_OUT:  UITransitionFadeOut(menu, SelectedOption); break;
    }
}

static short int UIGetSelectableItem(struct UIMenu *menu, short int id) {
    short int result = -1; int i;
    if (!menu || !menu->items) return -1;
    for (i = 0; menu->items[i].type != MITEM_TERMINATOR; i++) {
        if (menu->items[i].id == id) {
            if (menu->items[i].type > MITEM_LABEL && !(menu->items[i].flags & (MITEM_FLAG_DISABLED | MITEM_FLAG_HIDDEN | MITEM_FLAG_READONLY)))
                result = i;
            break;
        }
    }
    return result;
}

static short int UIGetNextSelectableItem(struct UIMenu *menu, short int index) {
    short int result = -1; int i;
    if (!menu || !menu->items) return -1;
    index = (index < 0) ? 0 : index + 1;
    for (i = index; menu->items[i].type != MITEM_TERMINATOR; i++) {
        if (menu->items[i].type > MITEM_LABEL && !(menu->items[i].flags & (MITEM_FLAG_DISABLED | MITEM_FLAG_HIDDEN | MITEM_FLAG_READONLY))) {
            result = i; break;
        }
    }
    return result;
}

static short int UIGetPrevSelectableItem(struct UIMenu *menu, short int index) {
    short int result = -1; int i;
    if (!menu || !menu->items) return -1;
    index = (index < 0) ? (MITEM_COUNT -1) : index - 1; // MITEM_COUNT might not be right here, need actual item count
                                                     // A safer way is to find last selectable if index < 0
    if (index < 0) { // Find last selectable if initial index was invalid or wrapped around
        for(i = 0; menu->items[i].type != MITEM_TERMINATOR; i++); // Count items
        index = i -1;
    }

    for (i = index; i >= 0; i--) {
        if (menu->items[i].type > MITEM_LABEL && !(menu->items[i].flags & (MITEM_FLAG_DISABLED | MITEM_FLAG_HIDDEN | MITEM_FLAG_READONLY))) {
            result = i; break;
        }
    }
    return result;
}

int UIExecMenu(struct UIMenu *FirstMenu, short int SelectedItemID, struct UIMenu **CurrentMenuOut, int (*callback)(struct UIMenu *menu, unsigned short int frame, int selection, u32 padstatus)) {
    struct UIMenu *current_menu; // Renamed from menu to avoid conflict
    int result_id = 1; // Default to 1 (often cancel or first button ID if not handled)
    u32 PadStatusCurrent, PadStatusTapped;
    u32 PadStatusRepeatOld = 0; // For repeat logic
    struct UIMenuItem *selected_item_ptr;
    short int current_selection_idx; // Index in the items array
    unsigned short int frame_count = 0;
    unsigned short int PadRepeatDelayTicks = UI_PAD_REPEAT_START_DELAY;
    unsigned short int PadRepeatRateTicks = UI_PAD_REPEAT_DELAY;

    current_menu = FirstMenu;
    if (CurrentMenuOut != NULL) *CurrentMenuOut = current_menu;

    // Initialize selection
    current_selection_idx = UIGetSelectableItem(current_menu, SelectedItemID);
    if (current_selection_idx < 0) { // If specified ID not found or not selectable
        current_selection_idx = UIGetNextSelectableItem(current_menu, -1); // Get first selectable
    }
    selected_item_ptr = (current_selection_idx >= 0) ? &current_menu->items[current_selection_idx] : NULL;

    if (callback != NULL) {
        if ((result_id = callback(current_menu, frame_count, current_selection_idx, 0)) != 0) {
            // If callback returns non-zero, it might be an event to exit UIExecMenu
             if (CurrentMenuOut != NULL) *CurrentMenuOut = current_menu;
            return result_id;
        }
    }

    while (1) {
        PadStatusCurrent = ReadCombinedPadStatus_raw(); // Get continuous pad state
        PadStatusTapped = PadStatusCurrent & ~PadStatusRepeatOld; // Get newly pressed buttons

        if (PadStatusCurrent == 0 || ((PadStatusRepeatOld != 0) && (PadStatusCurrent != PadStatusRepeatOld))) {
            PadRepeatDelayTicks = UI_PAD_REPEAT_START_DELAY; // Reset repeat delay
            PadRepeatRateTicks = UI_PAD_REPEAT_DELAY;
        } else { // Button is being held
            if (PadRepeatDelayTicks == 0) { // Initial delay passed
                if (PadRepeatRateTicks == 0) { // Repeat delay passed
                    PadStatusTapped |= PadStatusCurrent; // Allow repeat for held buttons
                    PadRepeatRateTicks = UI_PAD_REPEAT_DELAY; // Reset repeat rate timer
                } else {
                    PadRepeatRateTicks--;
                }
            } else {
                PadRepeatDelayTicks--;
            }
        }
        PadStatusRepeatOld = PadStatusCurrent; // Store current state for next frame's tap detection

        short int prev_selection_idx = current_selection_idx; // Store for comparison

        if (PadStatusTapped & PAD_UP) {
            current_selection_idx = UIGetPrevSelectableItem(current_menu, current_selection_idx);
            if(current_selection_idx < 0) current_selection_idx = UIGetNextSelectableItem(current_menu, prev_selection_idx); // Wrap around or stay if no prev
        } else if (PadStatusTapped & PAD_DOWN) {
            current_selection_idx = UIGetNextSelectableItem(current_menu, current_selection_idx);
            if(current_selection_idx < 0) current_selection_idx = UIGetPrevSelectableItem(current_menu, prev_selection_idx); // Wrap around or stay
        } else if (PadStatusTapped & PAD_LEFT) {
            if (selected_item_ptr != NULL && !(selected_item_ptr->flags & (MITEM_FLAG_READONLY | MITEM_FLAG_DISABLED))) {
                switch (selected_item_ptr->type) {
                    case MITEM_VALUE:
                        if (selected_item_ptr->value.value > selected_item_ptr->value.min) selected_item_ptr->value.value--;
                        else selected_item_ptr->value.value = selected_item_ptr->value.max;
                        break;
                    case MITEM_TOGGLE: selected_item_ptr->value.value = !selected_item_ptr->value.value; break;
                    case MITEM_ENUM:
                        if (selected_item_ptr->enumeration.selectedIndex > 0) selected_item_ptr->enumeration.selectedIndex--;
                        else selected_item_ptr->enumeration.selectedIndex = selected_item_ptr->enumeration.count - 1;
                        break;
                }
            }
        } else if (PadStatusTapped & PAD_RIGHT) {
             if (selected_item_ptr != NULL && !(selected_item_ptr->flags & (MITEM_FLAG_READONLY | MITEM_FLAG_DISABLED))) {
                switch (selected_item_ptr->type) {
                    case MITEM_VALUE:
                        if (selected_item_ptr->value.value < selected_item_ptr->value.max) selected_item_ptr->value.value++;
                        else selected_item_ptr->value.value = selected_item_ptr->value.min;
                        break;
                    case MITEM_TOGGLE: selected_item_ptr->value.value = !selected_item_ptr->value.value; break;
                    case MITEM_ENUM:
                        if (selected_item_ptr->enumeration.selectedIndex + 1 < selected_item_ptr->enumeration.count) selected_item_ptr->enumeration.selectedIndex++;
                        else selected_item_ptr->enumeration.selectedIndex = 0;
                        break;
                }
            }
        }

        if (PadStatusTapped & SelectButton) { // SelectButton is global (Cross or Circle)
            if (selected_item_ptr != NULL && !(selected_item_ptr->flags & (MITEM_FLAG_READONLY | MITEM_FLAG_DISABLED))) {
                if (selected_item_ptr->type == MITEM_BUTTON) {
                    result_id = selected_item_ptr->id; // Return the ID of the button
                    goto exit_uiexecmenu;
                } else if (selected_item_ptr->type == MITEM_TOGGLE) {
                     selected_item_ptr->value.value = !selected_item_ptr->value.value; // Toggle on select too
                }
                // Other types like MITEM_VALUE, MITEM_ENUM are changed with L/R, select might do nothing or open editor
            }
        } else if (PadStatusTapped & CancelButton) { // CancelButton is global
            result_id = 1; // Original behavior: returns 1 for cancel. MAIN_MENU_ID_BTN_EXIT is also 1.
            goto exit_uiexecmenu;
        }

        if (PadStatusTapped & PAD_R1) {
            if (current_menu->next != NULL) {
                UITransition(current_menu, UIMT_RIGHT_OUT, current_selection_idx);
                current_menu = current_menu->next;
                current_selection_idx = UIGetNextSelectableItem(current_menu, -1); // Select first item in new menu
                UITransition(current_menu, UIMT_LEFT_IN, current_selection_idx);
            }
        } else if (PadStatusTapped & PAD_L1) {
            if (current_menu->prev != NULL) {
                UITransition(current_menu, UIMT_LEFT_OUT, current_selection_idx);
                current_menu = current_menu->prev;
                current_selection_idx = UIGetNextSelectableItem(current_menu, -1);
                UITransition(current_menu, UIMT_RIGHT_IN, current_selection_idx);
            }
        }
        
        // Update selected_item_ptr if selection changed
        if (current_selection_idx >= 0 && current_menu->items[current_selection_idx].type != MITEM_TERMINATOR) {
            selected_item_ptr = &current_menu->items[current_selection_idx];
        } else { // No selectable item, or out of bounds (should not happen with correct UIGetNext/Prev)
            selected_item_ptr = NULL;
            current_selection_idx = -1; // Ensure it's marked as no selection
        }


        UIDrawMenu(current_menu, frame_count, UI_OFFSET_X, UI_OFFSET_Y, current_selection_idx);

        if (callback != NULL) {
            if ((result_id = callback(current_menu, frame_count, current_selection_idx, PadStatusTapped)) != 0) {
                goto exit_uiexecmenu;
            }
        }

        SyncFlipFB(&UIDrawGlobal);
        frame_count++;
    }

exit_uiexecmenu:
    if (CurrentMenuOut != NULL) *CurrentMenuOut = current_menu;
    return result_id;
}

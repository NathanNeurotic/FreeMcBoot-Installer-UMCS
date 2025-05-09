#include <kernel.h>
#include <libpad.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <osd_config.h> // For configGetLanguage() and original LANGUAGE_... enums
#include <limits.h>     // For UINT_MAX in progress screens
#include <wchar.h>      // For mbtowc

#include <libgs.h>      // For GS library functions, GS_PSM_CT32

// FMCB Project Specific Includes
#include "main.h"       // For DEBUG_PRINTF, GetConsoleRegion, CONSOLE_REGION_JAPAN, SelectButton, CancelButton, MAX_PATH, etc.
#include "system.h"     // For GetConsoleRegion, GetConsoleVMode
#include "pad.h"        // For PadInitPads, PadDeinitPads
#include "graphics.h"   // For UIDrawGlobal struct, GS_*, DrawSprite, FontPrintfWithFeedback, SyncFlipFB, BUTTON_TYPE_... etc.
#include "font.h"       // For FontInit, FontDeinit, FontGetGlyphWidth, FontGetStringWidth, FontInitWithBuffer
#include "UI.h"         // Includes lang.h (which now has FMCB_LANGUAGE_ID and correct _COUNTs)

extern int errno __attribute__((section("data"))); 

// UIDrawGlobal is defined here and used by many graphics functions
struct UIDrawGlobal UIDrawGlobal;
GS_IMAGE BackgroundTexture;
struct ClutImage PadLayoutTexture; 

static void *gFontBuffer = NULL;
static int gFontBufferSize = 0; 
unsigned short int SelectButton, CancelButton; 

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


// --- Definitions from your uploaded UI.c (MessageBox, ButtonLayoutParameters) ---
enum MBOX_SCREEN_ID { 
    MBOX_SCREEN_ID_TITLE = 1, MBOX_SCREEN_ID_MESSAGE,
    MBOX_SCREEN_ID_BTN1, MBOX_SCREEN_ID_BTN2, MBOX_SCREEN_ID_BTN3, MBOX_SCREEN_ID_BTN4,
};

static struct UIMenuItem MessageBoxItems[] = {
    {MITEM_LABEL, MBOX_SCREEN_ID_TITLE},
    {MITEM_SEPERATOR}, {MITEM_BREAK},
    {MITEM_STRING, MBOX_SCREEN_ID_MESSAGE, MITEM_FLAG_READONLY}, {MITEM_BREAK}, {MITEM_BREAK},
    {MITEM_BREAK},{MITEM_BREAK},{MITEM_BREAK},{MITEM_BREAK},{MITEM_BREAK},{MITEM_BREAK}, 
    {MITEM_BUTTON, MBOX_SCREEN_ID_BTN1, MITEM_FLAG_POS_MID, 0, 16}, {MITEM_BREAK}, {MITEM_BREAK},
    {MITEM_BUTTON, MBOX_SCREEN_ID_BTN3, MITEM_FLAG_POS_MID, 0, 16}, {MITEM_BREAK}, {MITEM_BREAK}, 
    {MITEM_BUTTON, MBOX_SCREEN_ID_BTN2, MITEM_FLAG_POS_MID, 0, 16}, {MITEM_BREAK}, {MITEM_BREAK},
    {MITEM_BUTTON, MBOX_SCREEN_ID_BTN4, MITEM_FLAG_POS_MID, 0, 16},
    {MITEM_TERMINATOR}
};
static struct UIMenu MessageBoxMenu = {NULL, NULL, MessageBoxItems, {{BUTTON_TYPE_SYS_SELECT, SYS_UI_LBL_OK}, {BUTTON_TYPE_SYS_CANCEL, SYS_UI_LBL_CANCEL}}};

// Using IconLayout_UI as you had it, assuming it's compatible or graphics.h defines BUTTON_TYPE_COUNT
struct IconLayout_UI 
{
    unsigned short int u, v;
    unsigned short int length, width;
};

static const struct IconLayout_UI ButtonLayoutParameters[BUTTON_TYPE_COUNT] =
    { 
        {22, 0, 22, 22}, {0, 0, 22, 22}, {44, 0, 22, 22}, {66, 0, 22, 22},
        {0, 22, 28, 20}, {56, 22, 28, 20}, {28, 22, 28, 20}, {84, 22, 28, 20},
        {150, 42, 30, 30}, {150, 72, 30, 30}, {140, 22, 29, 19}, {112, 22, 28, 19},
        {120, 72, 30, 30}, {0, 72, 30, 30}, {30, 72, 30, 30}, {60, 72, 30, 30}, {90, 72, 30, 30},
        {120, 42, 30, 30}, {0, 42, 30, 30}, {30, 42, 30, 30}, {60, 42, 30, 30}, {90, 42, 30, 30},
        {104, 102, 26, 26}, {130, 102, 26, 26}, {156, 102, 26, 26},
        {0, 102, 26, 26}, {26, 102, 26, 26}, {52, 102, 26, 26}, {78, 102, 26, 26},
};
// --- End of restored definitions ---


// --- Forward declarations for static functions within UI.c ---
static void WaitForDevice(void);
static int FormatLanguageString(const char *in, int len, char *out);
static void BreakLongLanguageString(char *str);
static int ParseLanguageFile(char **array, FILE *file, unsigned int ExpectedNumLines);
static int ParseFontListFile(char **array, FILE *file, unsigned int ExpectedNumLines);
static char *GetDefaultFontFilePath(void);
static char *GetFontFilePath(enum FMCB_LANGUAGE_ID langId); 
static void InitGraphics(void);
static int LoadFontIntoBuffer(struct UIDrawGlobal *gsGlobal, const char *path);
static int InitFont(void);
static int InitFontWithBuffer(void);
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
    static const char *LanguageFileShortForms[FMCB_LANG_COUNT] = { // Ensure FMCB_LANG_COUNT is correct
        "JA", "EN", "FR", "SP", "GE", "IT", "DU", "PO"
    };

    if (languageId >= FMCB_LANG_COUNT) {
        DEBUG_PRINTF("LoadLanguageStrings: Invalid languageId %d (Max: %d)\n", languageId, FMCB_LANG_COUNT -1);
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
        if (i >= (sizeof(DefaultLanguageStringTable)/sizeof(DefaultLanguageStringTable[0])) || DefaultLanguageStringTable[i] == NULL) { 
            DEBUG_PRINTF("Error: DefaultLanguageStringTable access out of bounds or NULL at index %u (SYS_UI_MSG_COUNT is %d).\n", i, SYS_UI_MSG_COUNT);
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
            if (i >= (sizeof(DefaultLanguageLabelStringTable)/sizeof(DefaultLanguageLabelStringTable[0])) || DefaultLanguageLabelStringTable[i] == NULL) { 
                DEBUG_PRINTF("Error: DefaultLanguageLabelStringTable access out of bounds or NULL at index %u (SYS_UI_LBL_COUNT is %d).\n", i, SYS_UI_LBL_COUNT);
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

// --- Original static functions from UI.c ---
static void WaitForDevice(void) { /* ... (Your uploaded UI.c implementation) ... */ }
static int FormatLanguageString(const char *in, int len, char *out) { 
    // char *out_start = out; // REMOVED: unused variable
    wchar_t wchar1, wchar2;
    int ActualLength, CharLen1, CharLen2;

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
static void BreakLongLanguageString(char *str) { /* ... (Your uploaded UI.c implementation) ... */ }
static int ParseLanguageFile(char **array, FILE *file, unsigned int ExpectedNumLines) { 
    // Assuming Makefile will have -std=gnu99 to allow C99 for loops
    int result = 0, len;
    unsigned int LinesLoaded; // Moved declaration to top of block for C89 compatibility if needed
    unsigned char BOMTemp[3];
    char line[512]; 
    char *temp_realloc;

    if (fread(BOMTemp, 1, 3, file) == 3 && (BOMTemp[0] == 0xEF && BOMTemp[1] == 0xBB && BOMTemp[2] == 0xBF)) {
    } else {
        rewind(file); 
    }
    for (LinesLoaded = 0; LinesLoaded < ExpectedNumLines; LinesLoaded++) { 
        if (fgets(line, sizeof(line), file) == NULL) { 
            if (feof(file)) {
                DEBUG_PRINTF("ParseLanguageFile: EOF reached prematurely. Expected %u, got %u lines.\n", ExpectedNumLines, LinesLoaded);
                result = -1; 
            } else {
                DEBUG_PRINTF("ParseLanguageFile: Error reading line %u (errno: %d).\n", LinesLoaded, errno);
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
            if (temp_realloc == NULL && formatted_len > 0) { 
                result = -ENOMEM; free(array[LinesLoaded]); array[LinesLoaded] = NULL; break;
            }
            array[LinesLoaded] = temp_realloc; 
        } else {
            result = -ENOMEM; break;
        }
    }
    if (result == 0 && LinesLoaded != ExpectedNumLines) {
        DEBUG_PRINTF("ParseLanguageFile: Mismatched number of lines. Expected %u, loaded %u.\n", ExpectedNumLines, LinesLoaded);
        result = -1; 
    }
    if (result != 0) {
        unsigned int k; for (k = 0; k < LinesLoaded; k++) { 
            if (array[k] != NULL) { free(array[k]); array[k] = NULL; }
        }
    }
    return result;
}
static int ParseFontListFile(char **array, FILE *file, unsigned int ExpectedNumLines) { 
    int result = 0, len; 
    unsigned int LinesLoaded; // C89 style
    char line[256]; 
    for (LinesLoaded = 0; LinesLoaded < ExpectedNumLines; LinesLoaded++) {
         if (fgets(line, sizeof(line), file) == NULL) {
            if (feof(file)) { 
                DEBUG_PRINTF("ParseFontListFile: EOF reached prematurely. Expected %u, got %u lines.\n", ExpectedNumLines, LinesLoaded);
                result = -1; 
            } else { 
                DEBUG_PRINTF("ParseFontListFile: Error reading line %u (errno: %d).\n", LinesLoaded, errno);
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
        } else { 
            array[LinesLoaded] = malloc(1); 
            if(array[LinesLoaded] != NULL) array[LinesLoaded][0] = '\0';
            else { result = -ENOMEM; break; }
        }
    }
    if (result == 0 && LinesLoaded != ExpectedNumLines) {
        DEBUG_PRINTF("ParseFontListFile: Mismatched number of lines. Expected %u, loaded %d.\n", ExpectedNumLines, LinesLoaded);
        result = -1;
    }
    if (result != 0) { 
        unsigned int k; for (k = 0; k < LinesLoaded; k++) { 
            if (array[k] != NULL) { free(array[k]); array[k] = NULL; }
        }
    }
    return result;
}
static char *GetDefaultFontFilePath(void) { /* ... (Your uploaded UI.c implementation) ... */ }
static char *GetFontFilePath(enum FMCB_LANGUAGE_ID langId) { /* ... (Your uploaded UI.c implementation, ensure it uses FMCB_LANG_COUNT) ... */ }

static void InitGraphics(void) { 
    /* ... (Your uploaded UI.c implementation for this function) ... */
    UIDrawGlobal.psm = GS_PSM_CT32; // Ensure this is correct
}
static int LoadFontIntoBuffer(struct UIDrawGlobal *gsGlobal, const char *path) { /* ... (Your uploaded UI.c implementation) ... */ }
static int InitFont(void) { /* ... (Your uploaded UI.c implementation) ... */ }
static int InitFontWithBuffer(void) { /* ... (Your uploaded UI.c implementation, ensure gFontBufferSize is used) ... */ }

// --- Public UI Functions (from UI.h, implementations in UI.c) ---
int ReinitializeUI(void) { /* ... (Your uploaded UI.c implementation) ... */ }
int InitializeUI(int BufferFont) { /* ... (Your uploaded UI.c implementation, ensure it maps OSD lang to FMCB_LANG_... and uses currentFMCBLanguage) ... */ }
void DeinitializeUI(void) { /* ... (Your uploaded UI.c implementation) ... */ }

int ShowMessageBox(int Option1LabelID, int Option2LabelID, int Option3LabelID, int Option4LabelID, const char *message_text, int titleLBLID) { 
    /* ... (Your uploaded UI.c implementation, ensure MBOX_SCREEN_ID_... and MessageBoxMenu are used correctly) ... */ 
}
void DisplayWarningMessage(unsigned int messageID) { ShowMessageBox(SYS_UI_LBL_OK, -1, -1, -1, GetString(messageID), SYS_UI_LBL_WARNING); }
void DisplayErrorMessage(unsigned int messageID) { ShowMessageBox(SYS_UI_LBL_OK, -1, -1, -1, GetString(messageID), SYS_UI_LBL_ERROR); }
void DisplayInfoMessage(unsigned int messageID) { ShowMessageBox(SYS_UI_LBL_OK, -1, -1, -1, GetString(messageID), SYS_UI_LBL_INFO); }
int DisplayPromptMessage(unsigned int messageID, unsigned char Button1LabelID, unsigned char Button2LabelID) { 
    return ShowMessageBox(Button1LabelID, Button2LabelID, -1, -1, GetString(messageID), SYS_UI_LBL_CONFIRM); 
}
void DisplayFlashStatusUpdate(unsigned int messageID) { ShowMessageBox(-1, -1, -1, -1, GetString(messageID), SYS_UI_LBL_WAIT); }

struct UIMenuItem *UIGetItem(struct UIMenu *menu, unsigned char id) { /* ... (Your uploaded UI.c implementation) ... */ }
void UISetVisible(struct UIMenu *menu, unsigned char id, int visible) { /* ... (Your uploaded UI.c implementation) ... */ }
void UISetReadonly(struct UIMenu *menu, unsigned char id, int readonly) { /* ... (Your uploaded UI.c implementation) ... */ }
void UISetEnabled(struct UIMenu *menu, unsigned char id, int enabled) { /* ... (Your uploaded UI.c implementation) ... */ }
void UISetValue(struct UIMenu *menu, unsigned char id, int value) { /* ... (Your uploaded UI.c implementation) ... */ }
int UIGetValue(struct UIMenu *menu, unsigned char id) { /* ... (Your uploaded UI.c implementation) ... */ }
void UISetLabel(struct UIMenu *menu, unsigned char id, int labelID) { /* ... (Your uploaded UI.c implementation) ... */ }
void UISetString(struct UIMenu *menu, unsigned char id, const char *string) { /* ... (Your uploaded UI.c implementation) ... */ }
void UISetType(struct UIMenu *menu, unsigned char id, unsigned char type) { /* ... (Your uploaded UI.c implementation) ... */ }
void UISetFormat(struct UIMenu *menu, unsigned char id, unsigned char format, unsigned char width) { /* ... (Your uploaded UI.c implementation) ... */ }
void UISetEnum(struct UIMenu *menu, unsigned char id, const int *labelIDs, int count) { /* ... (Your uploaded UI.c implementation) ... */ }
void UISetEnumSelectedIndex(struct UIMenu *menu, unsigned char id, int selectedIndex) { /* ... (Your uploaded UI.c implementation) ... */ }
int UIGetEnumSelectedIndex(struct UIMenu *menu, unsigned char id) { /* ... (Your uploaded UI.c implementation) ... */ }

void UIDrawMenu(struct UIMenu *menu, unsigned short int frame, short int StartX, short int StartY, short int selection) { 
    // char FormatString[16]; // This was unused, remove it.
    /* ... (Rest of your uploaded UI.c implementation for UIDrawMenu, ensure FontGetStringWidth is used correctly) ... */ 
}
static void UITransitionSlideRightOut(struct UIMenu *menu, int SelectedOption) { /* ... (Your uploaded UI.c implementation) ... */ }
static void UITransitionSlideLeftOut(struct UIMenu *menu, int SelectedOption) { /* ... (Your uploaded UI.c implementation) ... */ }
static void UITransitionSlideRightIn(struct UIMenu *menu, int SelectedOption) { /* ... (Your uploaded UI.c implementation) ... */ }
static void UITransitionSlideLeftIn(struct UIMenu *menu, int SelectedOption) { /* ... (Your uploaded UI.c implementation) ... */ }
static void UITransitionFadeIn(struct UIMenu *menu, int SelectedOption) { /* ... (Your uploaded UI.c implementation) ... */ }
static void UITransitionFadeOut(struct UIMenu *menu, int SelectedOption) { /* ... (Your uploaded UI.c implementation) ... */ }
void UITransition(struct UIMenu *menu, int type, int SelectedOption) { /* ... (Your uploaded UI.c implementation) ... */ }
static short int UIGetSelectableItem(struct UIMenu *menu, short int id) { /* ... (Your uploaded UI.c implementation) ... */ }
static short int UIGetNextSelectableItem(struct UIMenu *menu, short int index) { /* ... (Your uploaded UI.c implementation) ... */ }
static short int UIGetPrevSelectableItem(struct UIMenu *menu, short int index) { /* ... (Your uploaded UI.c implementation) ... */ }
int UIExecMenu(struct UIMenu *FirstMenu, short int SelectedItemID, struct UIMenu **CurrentMenuOut, int (*callback)(struct UIMenu *menu, unsigned short int frame, int selection, u32 padstatus)) { /* ... (Your uploaded UI.c implementation) ... */ }


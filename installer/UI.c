#include <kernel.h>
#include <libpad.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <osd_config.h> 
#include <limits.h>     
#include <wchar.h>      

#include <libgs.h>      

#include "main.h"       
#include "system.h"     
#include "pad.h"        
#include "graphics.h"   
#include "font.h"       
#include "UI.h"         

extern int errno __attribute__((section("data"))); 

struct UIDrawGlobal UIDrawGlobal;
GS_IMAGE BackgroundTexture;
struct ClutImage PadLayoutTexture; 

static void *gFontBuffer = NULL;
static int gFontBufferSize = 0; 
unsigned short int SelectButton, CancelButton; 

static enum FMCB_LANGUAGE_ID currentFMCBLanguage = FMCB_LANG_ENGLISH; 

#include "lang.c" 

static char *LangStringTable[SYS_UI_MSG_COUNT];
static char *LangLblStringTable[SYS_UI_LBL_COUNT];
static u8 LangStringWrapTable[(SYS_UI_MSG_COUNT + 7) / 8];

// Definitions from your uploaded UI.c (MessageBox, ButtonLayoutParameters)
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

// Using IconLayout_UI as in your uploaded UI.c
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

// --- Forward declarations for static functions within UI.c ---
static void WaitForDevice(void); // Keep if used, remove if truly unused
static int FormatLanguageString(const char *in, int len, char *out);
static void BreakLongLanguageString(char *str);
static int ParseLanguageFile(char **array, FILE *file, unsigned int ExpectedNumLines);
static int ParseFontListFile(char **array, FILE *file, unsigned int ExpectedNumLines); // Keep if used
static char *GetDefaultFontFilePath(void); // Keep if used
static char *GetFontFilePath(enum FMCB_LANGUAGE_ID langId); // Keep if used
static void InitGraphics(void); // Keep if used
static int LoadFontIntoBuffer(struct UIDrawGlobal *gsGlobal, const char *path); // Keep if used
static int InitFont(void); // Keep if used
static int InitFontWithBuffer(void); // Keep if used
static void UITransitionSlideRightOut(struct UIMenu *menu, int SelectedOption); // Keep if used
static void UITransitionSlideLeftOut(struct UIMenu *menu, int SelectedOption); // Keep if used
static void UITransitionSlideRightIn(struct UIMenu *menu, int SelectedOption); // Keep if used
static void UITransitionSlideLeftIn(struct UIMenu *menu, int SelectedOption); // Keep if used
static void UITransitionFadeIn(struct UIMenu *menu, int SelectedOption); // Keep if used
static void UITransitionFadeOut(struct UIMenu *menu, int SelectedOption); // Keep if used
static short int UIGetSelectableItem(struct UIMenu *menu, short int id); // Keep if used
static short int UIGetNextSelectableItem(struct UIMenu *menu, short int index); // Keep if used
static short int UIGetPrevSelectableItem(struct UIMenu *menu, short int index); // Keep if used

// --- Language Handling Functions (Implementations) ---
// MODIFIED: Removed 'static' keyword from these to match non-static declarations in lang.h
void UnloadLanguage(void) { /* ... (Implementation from your uploaded UI.c) ... */ }
int LoadLanguageStrings(enum FMCB_LANGUAGE_ID languageId) { /* ... (Implementation from your uploaded UI.c, ensure it uses FMCB_LANG_COUNT) ... */ }
int LoadDefaultLanguageStrings(void) { /* ... (Implementation from your uploaded UI.c) ... */ }

const char *GetString(unsigned int StrID) { /* ... (Implementation from your uploaded UI.c) ... */ }
const char *GetUILabel(unsigned int LblID) { /* ... (Implementation from your uploaded UI.c) ... */ }

int GetCurrentLanguage(void) { return currentFMCBLanguage; }
void SetCurrentLanguage(enum FMCB_LANGUAGE_ID languageId) { /* ... (Implementation from your uploaded UI.c) ... */ }

// --- Original static functions from UI.c ---
static void WaitForDevice(void) { /* ... (Your uploaded UI.c implementation) ... */ }
static int FormatLanguageString(const char *in, int len, char *out) { 
    // Remove 'char *out_start = out;' if it's unused.
    /* ... (Rest of your uploaded UI.c implementation) ... */ 
}
static void BreakLongLanguageString(char *str) { /* ... (Your uploaded UI.c implementation) ... */ }
static int ParseLanguageFile(char **array, FILE *file, unsigned int ExpectedNumLines) { 
    // Loop variables like 'LinesLoaded' should be declared at start of block if not using -std=gnu99
    /* ... (Rest of your uploaded UI.c implementation) ... */ 
}
static int ParseFontListFile(char **array, FILE *file, unsigned int ExpectedNumLines) { /* ... (Your uploaded UI.c implementation) ... */ }
static char *GetDefaultFontFilePath(void) { 
    /* ... (Your uploaded UI.c implementation) ... */ 
    // Ensure all paths return a value, e.g., return NULL on malloc failure.
    char *result = NULL; // Initialize
    const char prefix[] = "lang/";
    result = malloc(strlen(prefix) + sizeof(DefaultFontFilename)); // sizeof includes null
    if (result != NULL) {
        sprintf(result, "%s%s", prefix, DefaultFontFilename);
    }
    return result; // Ensure return
}
static char *GetFontFilePath(enum FMCB_LANGUAGE_ID langId) { 
    /* ... (Your uploaded UI.c implementation, ensure it uses FMCB_LANG_COUNT) ... */ 
    // Ensure all paths return a value.
    // Example from previous correction:
    static char *FontFileArray[FMCB_LANG_COUNT]; 
    char *result = NULL;
    char *pFontFilename;
    FILE *file_handle; // Renamed from file
    int i;
    static int fontListGloballyParsed = 0; 

    if (!fontListGloballyParsed) {
        memset(FontFileArray, 0, sizeof(FontFileArray)); 
        if ((file_handle = fopen("lang/fonts.txt", "r")) != NULL) {
            if (ParseFontListFile(FontFileArray, file_handle, FMCB_LANG_COUNT) == 0) {
                fontListGloballyParsed = 1; 
            } else {
                DEBUG_PRINTF("GetFontFilePath: Failed to parse lang/fonts.txt correctly.\n");
                for (i = 0; i < FMCB_LANG_COUNT; i++) { if (FontFileArray[i] != NULL) { free(FontFileArray[i]); FontFileArray[i] = NULL;}}
            }
            fclose(file_handle);
        } else {
            DEBUG_PRINTF("GetFontFilePath: lang/fonts.txt not found (errno: %d).\n", errno);
        }
    }

    if (fontListGloballyParsed && langId < FMCB_LANG_COUNT && FontFileArray[langId] != NULL && FontFileArray[langId][0] != '\0') {
        pFontFilename = FontFileArray[langId];
        const char prefix[] = "lang/";
        result = malloc(strlen(prefix) + strlen(pFontFilename) + 1);
        if (result != NULL) { 
            sprintf(result, "%s%s", prefix, pFontFilename);
        }
    }
    
    if (result == NULL) { 
        DEBUG_PRINTF("GetFontFilePath: Using default font path for langId %d.\n", langId);
        result = GetDefaultFontFilePath();
    }
    return result; // Ensure return
}

static void InitGraphics(void) { 
    /* ... (Your uploaded UI.c implementation for this function) ... */
    UIDrawGlobal.psm = GS_PSM_CT32; // This should be correct if libgs.h is included
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
    // char FormatString[16]; // REMOVE this if unused
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


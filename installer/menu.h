#ifndef __MENU_H__
#define __MENU_H__

// This header file declares functions related to menu handling and drawing
// within the FMCB Installer.

// Standard library includes that might be needed by menu functions if not included elsewhere
// For example, if any menu functions took complex structs defined in std headers.
// For now, only basic types are used in these prototypes.
// #include <stdio.h> // For NULL, float if used in prototypes (PercentageComplete)

// Project-specific includes that might be needed
// #include "main.h" // For event definitions if used in prototypes (not directly here but good practice)
// #include "UI.h"   // For UI structures if used in prototypes

// Existing function declarations from menu.h
// MODIFIED: MainMenu now returns an int (the event that caused it to exit)
int MainMenu(void); 

void DrawFileCopyProgressScreen(float PercentageComplete);
void DrawMemoryCardDumpingProgressScreen(float PercentageComplete, unsigned int rate, unsigned int SecondsRemaining);
void DrawMemoryCardRestoreProgressScreen(float PercentageComplete, unsigned int rate, unsigned int SecondsRemaining);
void DisplayOutOfSpaceMessage(unsigned int AvailableSpace, unsigned int RequiredSpace);
void DisplayOutOfSpaceMessageHDD_APPS(unsigned int AvailableSpace, unsigned int RequiredSpace);
void DisplayOutOfSpaceMessageHDD(unsigned int AvailableSpace, unsigned int RequiredSpace);
void RedrawLoadingScreen(unsigned int frame);

// New function declaration for the OpenTuna Installer sub-menu
// This function is implemented in menu_opentuna.c
int menu_opentuna_install(int connecting_event, int current_selection);

#endif /* __MENU_H__ */

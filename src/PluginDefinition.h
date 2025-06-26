#ifndef PLUGINDEFINITION_H
#define PLUGINDEFINITION_H

//
// All difinitions of plugin interface
//
#include "PluginInterface.h"

// The plugin name, it will be used in the plugin manager
const TCHAR NPP_PLUGIN_NAME[] = TEXT("PPC LogTrace Cleaner");

// plugin number, it will be used in the plugin manager
// to display the number of plugins functions in the menu
const int nbFunc = 1;


//
// Initialization of your plugin data
// It will be called while plugin loading
//
void pluginInit(HANDLE hModule);

//
// Cleaning of your plugin
// It will be called while plugin unloading
//
void pluginCleanUp();

//
//Initialization of your plugin commands
//
void commandMenuInit();

//
//Clean up your plugin commands allocation (if any)
//
void commandMenuCleanUp();

//
// Function which sets your command 
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk = NULL, bool check0nInit = false);


//
// Your plugin command functions
//
void CleanTraceLog();

#endif //PLUGINDEFINITION_H

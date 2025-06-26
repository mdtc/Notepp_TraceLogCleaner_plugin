// plugin definition file for Notepad++ 

#include <regex>
#include <vector>
#include <sstream>
#include <string>
#include <future>
#include <thread>
#include "PluginDefinition.h"
#include "menuCmdID.h"

using namespace std;

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;


// custom strcucts and functions

struct RegexMatch
{
    regex pattern;
    string replacement;

	RegexMatch(const regex& pat, const string& repl) : pattern(pat), replacement(repl) {}

};


vector<string> getFirstLines(const string& text, size_t count = 10) {
    istringstream stream(text);
    vector<string> lines;
    string line;

    while (lines.size() < count && getline(stream, line)) {
        lines.push_back(line);
    }

    return lines;
};

const RegexMatch* selectBestPattern(const vector<RegexMatch>& patterns, const string& text) {

    const RegexMatch* bestPattern = nullptr;
    vector<string> firstLines = getFirstLines(text, 10);

    for (const auto& line : firstLines) {
        for (const auto& pattern : patterns) {
            string transformed = regex_replace(line, pattern.pattern, pattern.replacement);
            if (transformed != line) {
                // Found a match — use this pattern
                bestPattern = &pattern;
                break;
            }
        }
        if (bestPattern) break;
    }

    return bestPattern;

};

// split text into blocks of a specified number of lines
vector<string> splitIntoBlocks(const string& text, size_t linesPerBlock) {
    istringstream stream(text);
    vector<string> blocks;
    ostringstream currentBlock;
    string line;
    size_t lineCount = 0;

    while (getline(stream, line)) {
        currentBlock << line << '\n';
        if (++lineCount >= linesPerBlock) {
            blocks.push_back(currentBlock.str());
            currentBlock.str("");
            currentBlock.clear();
            lineCount = 0;
        }
    }

    if (lineCount > 0) {
        blocks.push_back(currentBlock.str());
    }

    return blocks;
};


// Process a block of text using the best regex pattern

string processBlock(const string& block, const RegexMatch* bestPattern) {
    istringstream in(block);
    ostringstream out;
    string line;

    while (getline(in, line)) {
        line = regex_replace(line, bestPattern ->pattern , bestPattern ->replacement);
        out << line << '\n';
    }

    return out.str();
};



//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE /*hModule*/)
{
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    setCommand(0, TEXT("CleanTraceLog"), CleanTraceLog, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

// plugin main function
void CleanTraceLog()
{
    // display a message in the status bar
    ::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, 0, (LPARAM)TEXT("Analising trace log file..."));
	Sleep(2000); // simulate some processing time

	// Target the current Scintilla editor
    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1) return;

	// Get the current Scintilla handle
    HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

	// Get the text length of the current Scintilla editor
    int length = (int)::SendMessage(curScintilla, SCI_GETTEXTLENGTH, 0, 0);
    char* text = new char[length + 1];
    ::SendMessage(curScintilla, SCI_GETTEXT, length + 1, (LPARAM)text);

	// Convert the text to a string for easier manipulation
    string content(text);
    delete[] text;

	// Regex to remove timestamps, we can potentially have multiple patterns
    vector<RegexMatch> regexPatterns = {
        {
            regex(R"(PSAPPSRV\.\d+ \[(\d{4}-\d{2}-\d{2})T(\d{2}:\d{2}:\d{2}\.\d+)\] [^ ]+ \d+ \w+ \(\d+\)\s+\d+-\d+\s+\d+\.\d+\s+(Cur#\d+\.\d+\.\w+ RC=\d+ Dur=\d+\.\d+)?)"),
            "[$1 $2]"
        },
        {
            regex(R"(PSAPPSRV\.\d+ \(\d+\)\s+\d+-\d+\s+(\d{2}\.\d{2}\.\d{2})\s+(\d*\.\d*\s+)?[^ ]+\s+RC=\d+\s+Dur=\d+\.\d+\s+(.*))"),
            "$1 $3"
        }
    };


	// Select the best regex pattern based on the first few lines of the content
	const RegexMatch* bestPattern = selectBestPattern(regexPatterns, content);

    if (!bestPattern) {
        ::MessageBox(NULL, TEXT("No matching pattern found."), TEXT("Error"), MB_OK | MB_ICONERROR);
        return;
    }
    else
    {
		// execute the regex replacement

        ::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, 0, (LPARAM)TEXT("Adding timestamps..."));
        Sleep(2000); // simulate some processing time

        auto blocks = splitIntoBlocks(content, 10000); // 10,000 lines per block

        vector<future<string>> futures;
        for (const auto& block : blocks) {
            futures.push_back(async(launch::async, processBlock, block, bestPattern));
        }
        
        ::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, 0, (LPARAM)TEXT("Applying changes..."));
        ostringstream finalResult;
        for (auto& f : futures) {
            finalResult << f.get();
        };
     
        string cleaned = finalResult.str();
        ::SendMessage(curScintilla, SCI_SETTEXT, 0, (LPARAM)cleaned.c_str());

    };

    ::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, 0, (LPARAM)TEXT("Cleaning complete."));


}
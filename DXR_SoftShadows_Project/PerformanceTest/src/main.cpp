#include <iostream>
#include <Windows.h>
#include "Misc/Timer.h"
#include "TestParameters.h"
#include <string>

void Printl(std::string text)
{
    std::cout << text << std::endl;
}

int TestSandbox(wchar_t* args)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Start the child process.  
    if (!CreateProcess(L"Sandbox\\Sandbox.exe",   // No module name (use command line)
        (LPWSTR)args,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        L"Sandbox",           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi)           // Pointer to PROCESS_INFORMATION structure
        )
    {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return 0;
    }

    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;

    if (GetExitCodeProcess(pi.hProcess, &exitCode))
    {
        if (exitCode != 0)
        {
            Printl("SOMETHING WENT WRONG!!!!!!!!!!!!!!!!!!!!!!!!!!");
        }
    }

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    Printl("Testing Complete!!!");
}

int main(int argc, char* argv[])
{
    TestParameters params = {};

    // Not being used RN.
    ParseTestParameters(&params);

    Printl("Testing will comence...");

    /*
    Commandline args:

    -s test             // Scene
    -o 123.txt          // Output Result File
    -i false            // Use Inline-Raytracing
    -q false            // Quit after test
    -l 10               // Num Lights
    */

    wchar_t test1Args[] = L"-o ../RTX3080.csv -q 0";
    wchar_t test1Argsi[] = L"-o ../RTX3080i.csv -q 1 -i 1";

    // Scene 1
    Printl("Scene 1...........................................");
    TestSandbox(test1Args);
    Printl("..................................................\n");

    // Scene 1 (inline RT)
    //Printl("Scene 1 (inline)..................................");
    //TestSandbox(test1Argsi);
    //Printl("..................................................\n");


    Printl("..................................................");
    Printl("---------------------FINISHED---------------------");
    Printl("..................................................");

    std::cin.get();

	return 0;
}
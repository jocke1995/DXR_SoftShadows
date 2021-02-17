#include <iostream>
#include <Windows.h>
#include "Misc/Timer.h"
#include <string>

void Printl(std::string text)
{
    std::cout << text << std::endl;
}

int main()
{
    Printl("T");



    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Start the child process. 
    if (!CreateProcess(L"Sandbox\\Sandbox.exe",   // No module name (use command line)
        NULL,        // Command line
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
        Printl("Exit code was: " + std::to_string(exitCode));
    }

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    Printl("Testing Complete!!!");

    std::cin.get();

	return 0;
}
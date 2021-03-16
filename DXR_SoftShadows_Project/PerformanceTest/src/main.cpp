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
    -i rt               // Raytracing Type (rt, ip, ic)
    -q false            // Quit after test
    -l 10               // Num Lights
    */

#pragma region TestSponza
    wchar_t Sponza1Light_RT[] = L"-q 1 -i rt -s Sponza -l 1";
    wchar_t Sponza2Light_RT[] = L"-q 1 -i rt -s Sponza -l 2";
    wchar_t Sponza4Light_RT[] = L"-q 1 -i rt -s Sponza -l 4";
    wchar_t Sponza8Light_RT[] = L"-q 1 -i rt -s Sponza -l 8";

    wchar_t Sponza1Light_IP[] = L"-q 1 -i ip -s Sponza -l 1";
    wchar_t Sponza2Light_IP[] = L"-q 1 -i ip -s Sponza -l 2";
    wchar_t Sponza4Light_IP[] = L"-q 1 -i ip -s Sponza -l 4";
    wchar_t Sponza8Light_IP[] = L"-q 1 -i ip -s Sponza -l 8";

    wchar_t Sponza1Light_IC[] = L"-q 1 -i ic -s Sponza -l 1";
    wchar_t Sponza2Light_IC[] = L"-q 1 -i ic -s Sponza -l 2";
    wchar_t Sponza4Light_IC[] = L"-q 1 -i ic -s Sponza -l 4";
    wchar_t Sponza8Light_IC[] = L"-q 1 -i ic -s Sponza -l 8";

    // Scene 1
    Printl("Sponza Scenes RT START...........................................");
    TestSandbox(Sponza1Light_RT);
    TestSandbox(Sponza2Light_RT);
    TestSandbox(Sponza4Light_RT);
    TestSandbox(Sponza8Light_RT);
    Printl("Sponza Scenes RT END..................................................\n");
    Printl("Sponza Scenes InlinePixel START...........................................");
    TestSandbox(Sponza1Light_IP);
    TestSandbox(Sponza2Light_IP);
    TestSandbox(Sponza4Light_IP);
    TestSandbox(Sponza8Light_IP);
    Printl("Sponza Scenes InlinePixel END..................................................\n");
    Printl("Sponza Scenes InlineCompute START...........................................");
    TestSandbox(Sponza1Light_IC);
    TestSandbox(Sponza2Light_IC);
    TestSandbox(Sponza4Light_IC);
    TestSandbox(Sponza8Light_IC);
    Printl("Sponza Scenes InlineCompute END..................................................\n");

#pragma endregion TestSponza


    Printl("..................................................");
    Printl("---------------------FINISHED---------------------");
    Printl("..................................................");

    std::cin.get();

	return 0;
}
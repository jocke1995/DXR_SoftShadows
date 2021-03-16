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
#pragma region TestDragon
    wchar_t Dragon1Light_RT[] = L"-q 1 -i rt -s Dragon -l 1";
    wchar_t Dragon2Light_RT[] = L"-q 1 -i rt -s Dragon -l 2";
    wchar_t Dragon4Light_RT[] = L"-q 1 -i rt -s Dragon -l 4";
    wchar_t Dragon8Light_RT[] = L"-q 1 -i rt -s Dragon -l 8";

    wchar_t Dragon1Light_IP[] = L"-q 1 -i ip -s Dragon -l 1";
    wchar_t Dragon2Light_IP[] = L"-q 1 -i ip -s Dragon -l 2";
    wchar_t Dragon4Light_IP[] = L"-q 1 -i ip -s Dragon -l 4";
    wchar_t Dragon8Light_IP[] = L"-q 1 -i ip -s Dragon -l 8";

    wchar_t Dragon1Light_IC[] = L"-q 1 -i ic -s Dragon -l 1";
    wchar_t Dragon2Light_IC[] = L"-q 1 -i ic -s Dragon -l 2";
    wchar_t Dragon4Light_IC[] = L"-q 1 -i ic -s Dragon -l 4";
    wchar_t Dragon8Light_IC[] = L"-q 1 -i ic -s Dragon -l 8";

    // Scene 1
    Printl("Dragon Scenes RT START...........................................");
    TestSandbox(Dragon1Light_RT);
    TestSandbox(Dragon2Light_RT);
    TestSandbox(Dragon4Light_RT);
    TestSandbox(Dragon8Light_RT);
    Printl("Dragon Scenes RT END..................................................\n");
    Printl("Dragon Scenes InlinePixel START...........................................");
    TestSandbox(Dragon1Light_IP);
    TestSandbox(Dragon2Light_IP);
    TestSandbox(Dragon4Light_IP);
    TestSandbox(Dragon8Light_IP);
    Printl("Dragon Scenes InlinePixel END..................................................\n");
    Printl("Dragon Scenes InlineCompute START...........................................");
    TestSandbox(Dragon1Light_IC);
    TestSandbox(Dragon2Light_IC);
    TestSandbox(Dragon4Light_IC);
    TestSandbox(Dragon8Light_IC);
    Printl("Dragon Scenes InlineCompute END..................................................\n");

#pragma endregion TestDragon
#pragma region TestSponzaDragons
    wchar_t SponzaDragons1Light_RT[] = L"-q 1 -i rt -s SponzaDragons -l 1";
    wchar_t SponzaDragons2Light_RT[] = L"-q 1 -i rt -s SponzaDragons -l 2";
    wchar_t SponzaDragons4Light_RT[] = L"-q 1 -i rt -s SponzaDragons -l 4";
    wchar_t SponzaDragons8Light_RT[] = L"-q 1 -i rt -s SponzaDragons -l 8";

    wchar_t SponzaDragons1Light_IP[] = L"-q 1 -i ip -s SponzaDragons -l 1";
    wchar_t SponzaDragons2Light_IP[] = L"-q 1 -i ip -s SponzaDragons -l 2";
    wchar_t SponzaDragons4Light_IP[] = L"-q 1 -i ip -s SponzaDragons -l 4";
    wchar_t SponzaDragons8Light_IP[] = L"-q 1 -i ip -s SponzaDragons -l 8";

    wchar_t SponzaDragons1Light_IC[] = L"-q 1 -i ic -s SponzaDragons -l 1";
    wchar_t SponzaDragons2Light_IC[] = L"-q 1 -i ic -s SponzaDragons -l 2";
    wchar_t SponzaDragons4Light_IC[] = L"-q 1 -i ic -s SponzaDragons -l 4";
    wchar_t SponzaDragons8Light_IC[] = L"-q 1 -i ic -s SponzaDragons -l 8";

    // Scene 1
    Printl("SponzaDragons Scenes RT START...........................................");
    TestSandbox(SponzaDragons1Light_RT);
    TestSandbox(SponzaDragons2Light_RT);
    TestSandbox(SponzaDragons4Light_RT);
    TestSandbox(SponzaDragons8Light_RT);
    Printl("SponzaDragons Scenes RT END..................................................\n");
    Printl("SponzaDragons Scenes InlinePixel START...........................................");
    TestSandbox(SponzaDragons1Light_IP);
    TestSandbox(SponzaDragons2Light_IP);
    TestSandbox(SponzaDragons4Light_IP);
    TestSandbox(SponzaDragons8Light_IP);
    Printl("SponzaDragons Scenes InlinePixel END..................................................\n");
    Printl("SponzaDragons Scenes InlineCompute START...........................................");
    TestSandbox(SponzaDragons1Light_IC);
    TestSandbox(SponzaDragons2Light_IC);
    TestSandbox(SponzaDragons4Light_IC);
    TestSandbox(SponzaDragons8Light_IC);
    Printl("SponzaDragons Scenes InlineCompute END..................................................\n");

#pragma endregion TestSponzaDragons

    Printl("..................................................");
    Printl("---------------------FINISHED---------------------");
    Printl("..................................................");

    std::cin.get();

	return 0;
}
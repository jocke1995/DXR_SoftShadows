#ifndef TESTPARAMETER_H
#define TESTPARAMETER_H

#include "Headers/Core.h"
#include <shellapi.h> // CommandLineToArgvW

struct TestParameters
{
    int numTests = 5;
};

inline bool ParseTestParameters(TestParameters* output)
{
    LPWSTR* szArglist;
    int nArgs;

    szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (NULL == szArglist)
    {
        Log::Print("CommandLineToArgvW failed\n");
        return false;
    }
    else
    {
        for (int i = 0; i < nArgs; i++)
        {
#ifdef _DEBUG
            Log::Print("%d: %ws\n", i, szArglist[i]);
#endif

            // Scene
            if (wcscmp(szArglist[i], L"-n") == 0)
            {
                output->numTests = std::stoi(szArglist[++i]);
            }

        }
    }

    // Free memory allocated by CommandLineToArgvW
    LocalFree(szArglist);

    return true;
}

#endif
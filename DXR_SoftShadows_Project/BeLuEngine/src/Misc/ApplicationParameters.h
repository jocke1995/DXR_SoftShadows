#ifndef APPLICATIONPARAMETER_H
#define APPLICATIONPARAMETER_H

#include "Core.h"
#include <shellapi.h> // CommandLineToArgvW

struct ApplicationParameters
{
    std::wstring scene = L"test";
    std::wstring outputFile = L"noname.csv";
    bool useInlineRaytracing = false;
    bool quitOnFinish = false;
};

inline bool ParseParameters(ApplicationParameters* output)
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
            if (wcscmp(szArglist[i], L"-s") == 0)
            {
                output->scene = szArglist[++i];
            }

            // Result file
            if (wcscmp(szArglist[i], L"-o") == 0)
            {
                output->outputFile = szArglist[++i];
            }

            // Inline Raytracing
            if (wcscmp(szArglist[i], L"-i") == 0)
            {
                ++i;

                if (wcscmp(szArglist[i], L"true") == 0 || wcscmp(szArglist[i], L"True") == 0 || wcscmp(szArglist[i], L"TRUE") == 0 ||
                    wcscmp(szArglist[i], L"yes") == 0 || wcscmp(szArglist[i], L"Yes") == 0 || wcscmp(szArglist[i], L"YES") == 0 ||
                    wcscmp(szArglist[i], L"1") == 0)
                {
                    output->useInlineRaytracing = true;
                }
                else
                {
                    output->useInlineRaytracing = false;
                }
            }

            // Inline Raytracing
            if (wcscmp(szArglist[i], L"-q") == 0)
            {
                ++i;

                if (wcscmp(szArglist[i], L"true") == 0 || wcscmp(szArglist[i], L"True") == 0 || wcscmp(szArglist[i], L"TRUE") == 0 ||
                    wcscmp(szArglist[i], L"yes") == 0 || wcscmp(szArglist[i], L"Yes") == 0 || wcscmp(szArglist[i], L"YES") == 0 ||
                    wcscmp(szArglist[i], L"1") == 0)
                {
                    output->quitOnFinish = true;
                }
                else
                {
                    output->quitOnFinish = false;
                }
            }
        }
    }

    // Free memory allocated by CommandLineToArgvW
    LocalFree(szArglist);

    return true;
}

#endif
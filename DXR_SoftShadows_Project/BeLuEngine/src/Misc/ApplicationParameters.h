#ifndef APPLICATIONPARAMETER_H
#define APPLICATIONPARAMETER_H

#include "../Headers/Core.h"
#include <shellapi.h> // CommandLineToArgvW

struct ApplicationParameters
{
    std::wstring scene = L"test";
    std::wstring RayTracingType = L"notype";
    bool quitOnFinish = false;
    int numLights = 1;
};

inline bool ParseParameters(ApplicationParameters* output)
{
    LPWSTR* szArglist;
    int nArgs;

    szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (NULL == szArglist)
    {
        BL_LOG_WARNING("CommandLineToArgvW failed\n");
        return false;
    }
    else
    {
        for (int i = 0; i < nArgs; i++)
        {
            BL_LOG("%d: %ws\n", i, szArglist[i]);

            // Scene
            if (wcscmp(szArglist[i], L"-s") == 0)
            {
                output->scene = szArglist[++i];
            }

            // Inline Raytracing
            if (wcscmp(szArglist[i], L"-i") == 0)
            {
                output->RayTracingType = szArglist[++i];
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

            // Result file
            if (wcscmp(szArglist[i], L"-l") == 0)
            {
                output->numLights = std::stoi(szArglist[++i]);
            }
        }
    }

    // Free memory allocated by CommandLineToArgvW
    LocalFree(szArglist);

    return true;
}

#endif
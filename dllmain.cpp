#include <windows.h>
#include "JMSPlugin.h"

// 全局 DLL 模块句柄（供 JMSConfig 等类获取 DLL 路径用）
HINSTANCE g_hPluginInst = NULL;

BOOL APIENTRY DllMain(HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hPluginInst = (HINSTANCE)hModule;
        CJmsPlugin::s_hInst = (HINSTANCE)hModule;
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

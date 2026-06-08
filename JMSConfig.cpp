#include "JMSConfig.h"
#include <windows.h>
#include <string>

// dllmain.cpp 中设置的全局模块句柄
extern HINSTANCE g_hPluginInst;

CJmsConfig::CJmsConfig()
{
}

// 获取 DLL 所在目录
static std::wstring GetDllDirectory()
{
    if (!g_hPluginInst)
        return L"";

    wchar_t dllPath[MAX_PATH] = {};
    if (GetModuleFileNameW(g_hPluginInst, dllPath, MAX_PATH))
    {
        std::wstring path(dllPath);
        size_t pos = path.find_last_of(L"\\/");
        if (pos != std::wstring::npos)
        {
            return path.substr(0, pos);
        }
    }
    return L"";
}

bool CJmsConfig::Load()
{
    std::wstring dllDir = GetDllDirectory();

    // 依次尝试以下路径
    std::wstring searchPaths[] = {
        // 1. 插件配置目录（由主程序通过 OnExtenedInfo 传入）
        m_configDir.empty() ? L"" : (m_configDir + L"\\jms_config.ini"),
        // 2. DLL 同目录
        dllDir.empty() ? L"" : (dllDir + L"\\jms_config.ini"),
        // 3. 当前工作目录
        L"jms_config.ini",
    };

    for (const auto& configPath : searchPaths)
    {
        if (configPath.empty())
            continue;

        // 检查文件是否存在
        if (GetFileAttributesW(configPath.c_str()) == INVALID_FILE_ATTRIBUTES)
            continue;

        wchar_t url[2048] = {};
        DWORD result = GetPrivateProfileStringW(L"jms", L"api_url", L"",
                                                 url, 2048, configPath.c_str());
        if (result > 0 && url[0] != L'\0')
        {
            m_apiUrl = url;
            m_configPath = configPath;
            // 去除首尾空白
            while (!m_apiUrl.empty() && m_apiUrl.back() == L' ')
                m_apiUrl.pop_back();
            while (!m_apiUrl.empty() && m_apiUrl.front() == L' ')
                m_apiUrl.erase(m_apiUrl.begin());
            return true;
        }
    }

    // 都没找到，用 DLL 目录作为默认保存位置
    if (!dllDir.empty())
    {
        m_configPath = dllDir + L"\\jms_config.ini";
    }
    else
    {
        m_configPath = L"jms_config.ini";
    }

    return false;
}

bool CJmsConfig::Save(const std::wstring& apiUrl)
{
    if (m_configPath.empty())
    {
        // 尝试找个地方保存
        std::wstring dllDir = GetDllDirectory();
        if (!dllDir.empty())
            m_configPath = dllDir + L"\\jms_config.ini";
        else
            m_configPath = L"jms_config.ini";
    }

    // 写入配置文件
    BOOL ret = WritePrivateProfileStringW(L"jms", L"api_url",
                                           apiUrl.c_str(),
                                           m_configPath.c_str());
    if (ret)
    {
        m_apiUrl = apiUrl;
        return true;
    }
    return false;
}

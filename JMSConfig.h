#pragma once
#include <string>

class CJmsConfig
{
public:
    CJmsConfig();
    ~CJmsConfig() = default;

    // 加载配置文件
    bool Load();

    // 保存 api_url 到配置文件
    bool Save(const std::wstring& apiUrl);

    // 获取 API URL
    const std::wstring& GetApiUrl() const { return m_apiUrl; }

    // 获取实际使用的配置文件路径
    const std::wstring& GetConfigPath() const { return m_configPath; }

    // 设置配置目录（由主程序通过 OnExtenedInfo 传入）
    void SetConfigDir(const std::wstring& dir) { m_configDir = dir; }

private:
    std::wstring m_configDir;
    std::wstring m_apiUrl;
    std::wstring m_configPath;   // 实际加载/保存的配置文件路径
};

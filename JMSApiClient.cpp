#include "JMSApiClient.h"
#include <windows.h>
#include <winhttp.h>
#include <sstream>
#include <cctype>

#pragma comment(lib, "winhttp.lib")

CJmsApiClient::CJmsApiClient()
{
}

CJmsApiClient::~CJmsApiClient()
{
}

bool CJmsApiClient::ParseUrl(const std::wstring& url, std::wstring& host,
                              std::wstring& path, bool& isHttps, int& port)
{
    // 查找协议分隔符
    isHttps = false;
    port = 0;
    host.clear();
    path = L"/";

    std::wstring remain = url;

    // 处理协议头
    size_t pos = remain.find(L"://");
    if (pos != std::wstring::npos)
    {
        std::wstring scheme = remain.substr(0, pos);
        for (auto& c : scheme) c = towlower(c);
        if (scheme == L"https")
            isHttps = true;
        else if (scheme == L"http")
            isHttps = false;
        else
            return false;
        remain = remain.substr(pos + 3);
    }
    else
    {
        // 默认 HTTPS
        isHttps = true;
    }

    // 查找主机部分结束位置（/ 或 ? 或 #）
    pos = remain.find_first_of(L"/?#");
    if (pos != std::wstring::npos)
    {
        host = remain.substr(0, pos);
        path = remain.substr(pos);
        if (path.empty()) path = L"/";
    }
    else
    {
        host = remain;
        path = L"/";
    }

    // 处理端口号
    pos = host.find(L':');
    if (pos != std::wstring::npos)
    {
        port = std::stoi(host.substr(pos + 1));
        host = host.substr(0, pos);
    }
    else
    {
        port = isHttps ? 443 : 80;
    }

    return !host.empty();
}

bool CJmsApiClient::FetchBandwidthData(const std::wstring& apiUrl, JmsBandwidthData& data)
{
    data.valid = false;

    std::wstring host, path;
    bool isHttps;
    int port;

    if (!ParseUrl(apiUrl, host, path, isHttps, port))
        return false;

    HINTERNET hSession = WinHttpOpen(L"TrafficMonitor-JMS-Plugin/1.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      NULL, NULL, 0);
    if (!hSession)
        return false;

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), (INTERNET_PORT)port, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD flags = 0;
    if (isHttps)
        flags |= WINHTTP_FLAG_SECURE;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
                                             NULL, NULL, NULL, flags);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // 设置超时
    DWORD timeout = 15000;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));

    // 跳过证书验证（JMS 的证书可能有问题）
    if (isHttps)
    {
        DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                         SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
                         SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
    }

    bool sent = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                    WINHTTP_NO_REQUEST_DATA, 0, 0, 0) != FALSE;

    bool result = false;
    if (sent && WinHttpReceiveResponse(hRequest, NULL))
    {
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            NULL, &statusCode, &statusSize, NULL);

        if (statusCode == 200)
        {
            // 读取响应数据
            std::string response;
            DWORD bytesAvailable = 0;
            DWORD bytesRead = 0;

            while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0)
            {
                std::vector<char> buffer(bytesAvailable);
                if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead))
                {
                    response.append(buffer.data(), bytesRead);
                }
                else
                {
                    break;
                }
            }

            if (!response.empty())
            {
                result = ParseJsonResponse(response, data);
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
}

// 简单的 JSON 解析器（仅针对 JMS API 返回格式优化）
bool CJmsApiClient::ParseJsonResponse(const std::string& json, JmsBandwidthData& data)
{
    // 查找 "monthly_bw_limit_b": 后面的数字
    auto findInt64 = [&](const std::string& key) -> long long {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos)
            return -1;
        pos = json.find(':', pos + search.length());
        if (pos == std::string::npos)
            return -1;
        // 跳过空白
        pos++;
        while (pos < json.length() && std::isspace((unsigned char)json[pos]))
            pos++;
        // 提取数字
        long long val = 0;
        bool negative = false;
        if (pos < json.length() && json[pos] == '-')
        {
            negative = true;
            pos++;
        }
        while (pos < json.length() && std::isdigit((unsigned char)json[pos]))
        {
            val = val * 10 + (json[pos] - '0');
            pos++;
        }
        return negative ? -val : val;
    };

    auto findInt = [&](const std::string& key) -> int {
        return (int)findInt64(key);
    };

    long long limit = findInt64("monthly_bw_limit_b");
    long long counter = findInt64("bw_counter_b");
    int resetDay = findInt("bw_reset_day_of_month");

    if (limit >= 0 && counter >= 0 && resetDay > 0)
    {
        data.monthly_bw_limit_b = limit;
        data.bw_counter_b = counter;
        data.bw_reset_day_of_month = resetDay;
        data.valid = true;
        return true;
    }

    return false;
}

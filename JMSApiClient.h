#pragma once
#include <string>
#include <vector>

// Just My Socks API 返回的数据
struct JmsBandwidthData
{
    long long monthly_bw_limit_b = 0;   // 月流量上限（字节）
    long long bw_counter_b = 0;         // 已用流量（字节）
    int bw_reset_day_of_month = 1;      // 每月重置日
    bool valid = false;                 // 数据是否有效
};

// JMS API HTTP 客户端
class CJmsApiClient
{
public:
    CJmsApiClient();
    ~CJmsApiClient();

    // 从 API URL 获取流量数据
    bool FetchBandwidthData(const std::wstring& apiUrl, JmsBandwidthData& data);

private:
    // 解析 URL 成主机名、路径等组件
    static bool ParseUrl(const std::wstring& url, std::wstring& host,
                         std::wstring& path, bool& isHttps, int& port);

    // 解析 JSON 响应
    bool ParseJsonResponse(const std::string& json, JmsBandwidthData& data);
};

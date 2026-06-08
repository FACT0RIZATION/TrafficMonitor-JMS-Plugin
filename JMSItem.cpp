#include "JMSItem.h"
#include <windows.h>
#include <shellapi.h>
#include <cstdio>
#include <string>

CJmsItem::CJmsItem()
    : m_labelText(L"JMS")
    , m_valueText(L"--")
    , m_detailText(L"")
    , m_dataValid(false)
{
}

const wchar_t* CJmsItem::GetItemName() const
{
    return L"JMS Traffic";
}

const wchar_t* CJmsItem::GetItemId() const
{
    return L"JMSPluginItem";
}

const wchar_t* CJmsItem::GetItemLableText() const
{
    return m_labelText.c_str();
}

const wchar_t* CJmsItem::GetItemValueText() const
{
    return m_valueText.c_str();
}

const wchar_t* CJmsItem::GetItemValueSampleText() const
{
    return L"100.0%";
}

int CJmsItem::OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag)
{
    if (type == MT_LCLICKED)
    {
        // 左键点击打开 JMS 后台
        ShellExecuteW((HWND)hWnd, L"open",
                      L"https://justmysocks6.net/members/clientarea.php",
                      NULL, NULL, SW_SHOW);
        return 1;
    }
    return 0;
}

void CJmsItem::UpdateData(const JmsBandwidthData& data)
{
    if (!data.valid)
    {
        m_valueText = L"ERR";
        m_detailText = L"JMS: failed to get data";
        m_dataValid = false;
        return;
    }

    m_dataValid = true;

    long long used = data.bw_counter_b;
    long long total = data.monthly_bw_limit_b;
    long long remaining = total - used;
    double pct = (total > 0) ? (static_cast<double>(used) / total * 100.0) : 0.0;

    // 显示使用率百分比（和 CPU 格式一致）
    wchar_t valBuf[16] = {};
    swprintf_s(valBuf, L"%.1f%%", pct);
    m_valueText = valBuf;

    // 超过 90% 加警告标记
    if (pct >= 90.0)
    {
        m_valueText += L" !";
    }

    // 详细信息（tooltip 用）
    std::wstring usedDetail = FormatBytes(used, 1);
    std::wstring totalDetail = FormatBytes(total, 1);
    std::wstring remainDetail = FormatBytes(remaining, 1);
    wchar_t detail[256] = {};
    swprintf_s(detail, L"Just My Socks\nUsed: %s / %s\nRemaining: %s (%.1f%%)\nReset day: %d",
               usedDetail.c_str(), totalDetail.c_str(),
               remainDetail.c_str(), 100.0 - pct,
               data.bw_reset_day_of_month);
    m_detailText = detail;
}

std::wstring CJmsItem::FormatBytes(long long bytes, int decimals)
{
    if (bytes < 0) return L"0 B";

    static const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    double val = static_cast<double>(bytes);
    int unitIndex = 0;

    while (val >= 1024.0 && unitIndex < 4)
    {
        val /= 1024.0;
        unitIndex++;
    }

    wchar_t buf[32] = {};
    if (decimals == 0)
    {
        // 整数模式
        swprintf_s(buf, L"%.0f %s", val, units[unitIndex]);
    }
    else if (unitIndex == 0)
    {
        swprintf_s(buf, L"%.0f %s", val, units[unitIndex]);
    }
    else
    {
        swprintf_s(buf, L"%.*f %s", decimals, val, units[unitIndex]);
    }
    return buf;
}

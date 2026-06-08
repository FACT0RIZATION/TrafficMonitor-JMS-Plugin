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
    return L"JMS 流量";
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
    return L"888.8 GB / 999.9 GB";
}

int CJmsItem::OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag)
{
    if (type == MT_LCLICKED)
    {
        // 左键点击打开 JMS 后台
        ShellExecuteW((HWND)hWnd, L"open",
                      L"https://justmysocks6.net/members/clientarea.php",
                      NULL, NULL, SW_SHOW);
        return 1; // 已处理
    }
    else if (type == MT_DBCLICKED)
    {
        // 双击打开官网
        ShellExecuteW((HWND)hWnd, L"open",
                      L"https://justmysocks.net",
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
        m_detailText = L"JMS: 数据获取失败";
        m_dataValid = false;
        return;
    }

    m_dataValid = true;

    long long used = data.bw_counter_b;
    long long total = data.monthly_bw_limit_b;
    long long remaining = total - used;
    double pct = (total > 0) ? (static_cast<double>(used) / total * 100.0) : 0.0;

    std::wstring usedStr = FormatBytes(used);
    std::wstring totalStr = FormatBytes(total);
    std::wstring remainStr = FormatBytes(remaining);

    // 显示文本：已用 / 总量
    m_valueText = usedStr + L" / " + totalStr;

    // 详细信息（tooltip 用）
    wchar_t detail[256] = {};
    swprintf_s(detail, L"JMS 流量\n已用: %s / %s\n剩余: %s (%.1f%%)\n重置日: 每月%d日",
               usedStr.c_str(), totalStr.c_str(),
               remainStr.c_str(), 100.0 - pct,
               data.bw_reset_day_of_month);
    m_detailText = detail;

    // 如果已用超过 90%，在数值后加警告标记
    if (pct >= 90.0)
    {
        m_valueText += L" !";
    }
}

std::wstring CJmsItem::FormatBytes(long long bytes)
{
    if (bytes < 0) return L"0 B";

    static const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    double val = static_cast<double>(bytes);
    int unitIndex = 0;

    // 使用 1024 进制
    while (val >= 1024.0 && unitIndex < 4)
    {
        val /= 1024.0;
        unitIndex++;
    }

    wchar_t buf[32] = {};
    if (unitIndex == 0)
    {
        swprintf_s(buf, L"%.0f %s", val, units[unitIndex]);
    }
    else
    {
        swprintf_s(buf, L"%.1f %s", val, units[unitIndex]);
    }
    return buf;
}

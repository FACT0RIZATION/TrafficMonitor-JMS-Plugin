#include "JMSPlugin.h"
#include <windows.h>
#include <string>
#include <vector>

// 全局单例
static CJmsPlugin g_plugin;

// 静态成员定义
HINSTANCE CJmsPlugin::s_hInst = NULL;
bool CJmsPlugin::s_dlgRegistered = false;

// 对话框窗口类名
static const wchar_t* DLG_CLASS = L"JMSPluginSettingsDlg";

CJmsPlugin& CJmsPlugin::Instance()
{
    return g_plugin;
}

CJmsPlugin::CJmsPlugin()
    : m_initialized(false)
{
    InitializeCriticalSection(&m_cs);
}

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CJmsPlugin::Instance();
}

IPluginItem* CJmsPlugin::GetItem(int index)
{
    if (index == 0)
        return &m_item;
    return nullptr;
}

void CJmsPlugin::DataRequired()
{
    if (!m_initialized)
    {
        m_initialized = true;
        if (!m_config.Load())
        {
            m_tooltipText = L"JMS: no config file. Right-click -> Plugin Settings to set API URL";
            return;
        }
        if (m_config.GetApiUrl().empty())
        {
            m_tooltipText = L"JMS: API URL is empty. Right-click -> Plugin Settings to configure";
            return;
        }
    }

    RefreshData();
}

void CJmsPlugin::RefreshData()
{
    EnterCriticalSection(&m_cs);

    std::wstring apiUrl = m_config.GetApiUrl();
    if (apiUrl.empty())
    {
        m_item.UpdateData(JmsBandwidthData{});
        LeaveCriticalSection(&m_cs);
        return;
    }

    JmsBandwidthData data;
    bool success = m_apiClient.FetchBandwidthData(apiUrl, data);

    if (success && data.valid)
    {
        m_item.UpdateData(data);

        long long remaining = data.monthly_bw_limit_b - data.bw_counter_b;
        double remainPct = (data.monthly_bw_limit_b > 0)
            ? (static_cast<double>(remaining) / data.monthly_bw_limit_b * 100.0)
            : 0.0;

        wchar_t tip[512] = {};
        swprintf_s(tip,
            L"Just My Socks\n"
            L"Used: %s / %s\n"
            L"Remaining: %s (%.1f%%)\n"
            L"Usage: %.1f%%\n"
            L"Reset day: %d",
            CJmsItem::FormatBytes(data.bw_counter_b).c_str(),
            CJmsItem::FormatBytes(data.monthly_bw_limit_b).c_str(),
            CJmsItem::FormatBytes(remaining).c_str(),
            remainPct,
            (data.monthly_bw_limit_b > 0)
                ? (static_cast<double>(data.bw_counter_b) / data.monthly_bw_limit_b * 100.0)
                : 0.0,
            data.bw_reset_day_of_month
        );
        m_tooltipText = tip;
    }
    else
    {
        m_item.UpdateData(JmsBandwidthData{});
        m_tooltipText = L"JMS: API request failed. Check your API URL";
    }

    LeaveCriticalSection(&m_cs);
}

const wchar_t* CJmsPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:          return L"Just My Socks Traffic";
    case TMI_DESCRIPTION:   return L"Display Just My Socks bandwidth usage in taskbar";
    case TMI_AUTHOR:        return L"FACT0RIZATION";
    case TMI_COPYRIGHT:     return L"Copyright (C) 2026";
    case TMI_VERSION:       return L"1.0.0";
    case TMI_URL:           return L"https://github.com/FACT0RIZATION/TrafficMonitor-JMS-Plugin";
    default:                return L"";
    }
}

// ============================================================
// 设置对话框 — 使用 CreateWindowEx + 模态消息循环
// ============================================================

LRESULT CALLBACK CJmsPlugin::SettingsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        // 标签
        CreateWindowExW(0, L"Static",
            L"Just My Socks API URL (from JMS admin -> Service -> API):",
            WS_CHILD | WS_VISIBLE,
            12, 10, 316, 16,
            hWnd, (HMENU)IDC_LABEL, s_hInst, NULL);

        // 编辑框
        HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            12, 30, 316, 22,
            hWnd, (HMENU)IDC_EDIT_URL, s_hInst, NULL);

        // 填入当前 URL
        const std::wstring& url = CJmsPlugin::Instance().m_config.GetApiUrl();
        SetWindowTextW(hEdit, url.c_str());

        // 确定按钮
        CreateWindowExW(0, L"Button",
            L"OK",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            170, 65, 70, 26,
            hWnd, (HMENU)IDC_BTN_OK, s_hInst, NULL);

        // 取消按钮
        CreateWindowExW(0, L"Button",
            L"Cancel",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            250, 65, 70, 26,
            hWnd, (HMENU)IDC_BTN_CANCEL, s_hInst, NULL);

        // 全选编辑框文字
        SendDlgItemMessageW(hWnd, IDC_EDIT_URL, EM_SETSEL, 0, -1);
        SetFocus(hEdit);
        return 0;
    }

    case WM_COMMAND:
    {
        WORD id = LOWORD(wParam);
        if (id == IDC_BTN_OK)
        {
            wchar_t url[2048] = {};
            GetDlgItemTextW(hWnd, IDC_EDIT_URL, url, 2048);

            std::wstring newUrl(url);
            // 去除首尾空白
            while (!newUrl.empty() && newUrl.back() == L' ')
                newUrl.pop_back();
            while (!newUrl.empty() && newUrl.front() == L' ')
                newUrl.erase(newUrl.begin());

            if (!newUrl.empty())
            {
                CJmsPlugin::Instance().m_config.Save(newUrl);
                DestroyWindow(hWnd);
            }
            else
            {
                MessageBoxW(hWnd,
                    L"API URL cannot be empty.",
                    L"Invalid input", MB_OK | MB_ICONWARNING);
            }
            return 0;
        }
        else if (id == IDC_BTN_CANCEL || id == IDCANCEL)
        {
            DestroyWindow(hWnd);
            return 0;
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

ITMPlugin::OptionReturn CJmsPlugin::ShowOptionsDialog(void* hParent)
{
    // 注册窗口类（仅首次）
    if (!s_dlgRegistered)
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = SettingsWndProc;
        wc.hInstance = s_hInst;
        wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = DLG_CLASS;
        RegisterClassExW(&wc);
        s_dlgRegistered = true;
    }

    // 创建窗口
    HWND hWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOOLWINDOW,
        DLG_CLASS, L"JMS Plugin Settings",
        WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        352, 140,
        (HWND)hParent, NULL, s_hInst, NULL);

    if (!hWnd)
        return OR_OPTION_NOT_PROVIDED;

    // 居中到父窗口
    RECT rc, rcParent;
    GetWindowRect(hWnd, &rc);
    if (hParent)
    {
        GetWindowRect((HWND)hParent, &rcParent);
        SetWindowPos(hWnd, NULL,
            rcParent.left + (rcParent.right - rcParent.left - (rc.right - rc.left)) / 2,
            rcParent.top + (rcParent.bottom - rcParent.top - (rc.bottom - rc.top)) / 2,
            0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    // 模态消息循环
    HWND hParentWnd = (HWND)hParent;
    if (hParentWnd) EnableWindow(hParentWnd, FALSE);

    MSG msg;
    BOOL ret;
    while ((ret = GetMessageW(&msg, NULL, 0, 0)) > 0)
    {
        if (!IsDialogMessageW(hWnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (hParentWnd)
    {
        EnableWindow(hParentWnd, TRUE);
        SetFocus(hParentWnd);
    }

    // 判断是否修改了配置
    return OR_OPTION_CHANGED;
}

const wchar_t* CJmsPlugin::GetTooltipInfo()
{
    if (m_tooltipText.empty())
        return L"Just My Socks\nWaiting for initialization...";
    return m_tooltipText.c_str();
}

void CJmsPlugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data)
{
    if (index == EI_CONFIG_DIR && data)
    {
        m_config.SetConfigDir(data);
    }
}

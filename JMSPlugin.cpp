#include "JMSPlugin.h"
#include <windows.h>
#include <string>
#include <vector>

// 全局单例
static CJmsPlugin g_plugin;

// DLL 模块句柄（由 DllMain 设置）
HINSTANCE CJmsPlugin::s_hInst = NULL;

// 设置对话框中编辑框的控件 ID
#define IDC_API_URL_EDIT 1001

CJmsPlugin& CJmsPlugin::Instance()
{
    return g_plugin;
}

CJmsPlugin::CJmsPlugin()
    : m_name(L"Just My Socks 流量监控")
    , m_description(L"在任务栏显示 Just My Socks 代理的已用流量和总流量")
    , m_author(L"FACT0RIZATION")
    , m_copyright(L"Copyright (C) 2026")
    , m_version(L"1.0.0")
    , m_url(L"https://justmysocks.net")
    , m_initialized(false)
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
            m_tooltipText = L"JMS: 未找到配置文件，请右击 → 插件设置 → 填入 API URL";
            return;
        }
        if (m_config.GetApiUrl().empty())
        {
            m_tooltipText = L"JMS: API URL 为空，请右击 → 插件设置 → 配置";
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
            L"Just My Socks 流量\n"
            L"已用: %s / %s\n"
            L"剩余: %s (%.1f%%)\n"
            L"使用率: %.1f%%\n"
            L"每月 %d 日重置",
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
        m_tooltipText = L"JMS: API 请求失败\n请检查 API URL 是否正确";
    }

    LeaveCriticalSection(&m_cs);
}

const wchar_t* CJmsPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:          return m_name.c_str();
    case TMI_DESCRIPTION:   return m_description.c_str();
    case TMI_AUTHOR:        return m_author.c_str();
    case TMI_COPYRIGHT:     return m_copyright.c_str();
    case TMI_VERSION:       return m_version.c_str();
    case TMI_URL:           return m_url.c_str();
    default:                return L"";
    }
}

// ============================================================
// 设置对话框 — 使用 DialogBoxIndirectParamW + 内存模板
// ============================================================

// 辅助：构建对话框模板的缓冲区
class CDlgTemplateBuilder
{
public:
    std::vector<BYTE> buf;

    void AlignTo(DWORD alignment)
    {
        while (buf.size() % alignment != 0)
            buf.push_back(0);
    }

    void WriteWord(WORD w)
    {
        AlignTo(2);
        buf.insert(buf.end(), (BYTE*)&w, (BYTE*)&w + sizeof(w));
    }

    void WriteDword(DWORD d)
    {
        AlignTo(4);
        buf.insert(buf.end(), (BYTE*)&d, (BYTE*)&d + sizeof(d));
    }

    void WriteString(const wchar_t* s)
    {
        size_t len = (wcslen(s) + 1) * sizeof(wchar_t);
        buf.insert(buf.end(), (BYTE*)s, (BYTE*)s + len);
    }

    void WriteByte(BYTE b)
    {
        buf.push_back(b);
    }

    // 添加一个预定义类的控件（Button/Edit/Static）
    void AddControl(DWORD style, WORD id,
                    short x, short y, short cx, short cy,
                    WORD classAtom, const wchar_t* text)
    {
        AlignTo(4);
        WriteDword(style);
        WriteDword(0);                          // dwExtendedStyle
        WriteWord(x); WriteWord(y);
        WriteWord(cx); WriteWord(cy);
        WriteWord(id);                          // 控件 ID（WORD）
        WriteWord(0xFFFF);                      // 预定义类标记
        WriteWord(classAtom);                   // 类原子
        WriteString(text);                      // 控件文本
        WriteWord(0);                           // 无附加创建数据
    }
};

INT_PTR CALLBACK CJmsPlugin::SettingsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        // 把当前 URL 设置到编辑框
        const std::wstring& url = CJmsPlugin::Instance().m_config.GetApiUrl();
        SetDlgItemTextW(hDlg, IDC_API_URL_EDIT, url.c_str());
        // 编辑框获得焦点
        SetFocus(GetDlgItem(hDlg, IDC_API_URL_EDIT));
        // 全选文字方便直接替换
        SendDlgItemMessageW(hDlg, IDC_API_URL_EDIT, EM_SETSEL, 0, -1);
        return FALSE;   // 因为我们手动设置了焦点
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            // 获取编辑框中的 URL
            wchar_t url[2048] = {};
            GetDlgItemTextW(hDlg, IDC_API_URL_EDIT, url, 2048);

            // 去除首尾空白
            std::wstring newUrl(url);
            while (!newUrl.empty() && newUrl.back() == L' ')
                newUrl.pop_back();
            while (!newUrl.empty() && newUrl.front() == L' ')
                newUrl.erase(newUrl.begin());

            if (!newUrl.empty())
            {
                // 保存到配置文件
                CJmsPlugin::Instance().m_config.Save(newUrl);
                EndDialog(hDlg, IDOK);
            }
            else
            {
                MessageBoxW(hDlg,
                    L"API URL 不能为空，请输入正确的 Just My Socks API 地址。",
                    L"输入无效", MB_OK | MB_ICONWARNING);
            }
            return TRUE;
        }

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }

    return FALSE;
}

// 构建设置对话框并显示
ITMPlugin::OptionReturn CJmsPlugin::ShowOptionsDialog(void* hParent)
{
    CDlgTemplateBuilder tb;

    // ======== 对话框头 ========
    DLGTEMPLATE dlgHeader = {};
    dlgHeader.style = DS_MODALFRAME | DS_SETFONT | DS_CENTER |
                      WS_POPUP | WS_CAPTION | WS_SYSMENU;
    dlgHeader.dwExtendedStyle = 0;
    dlgHeader.cdit = 4;             // 4 个控件
    dlgHeader.x = 0;
    dlgHeader.y = 0;
    dlgHeader.cx = 340;             // 宽度
    dlgHeader.dy = 130;             // 高度

    tb.AlignTo(4);
    tb.WriteDword(dlgHeader.style);
    tb.WriteDword(dlgHeader.dwExtendedStyle);
    tb.WriteWord(dlgHeader.cdit);
    tb.WriteWord(dlgHeader.x);
    tb.WriteWord(dlgHeader.y);
    tb.WriteWord(dlgHeader.cx);
    tb.WriteWord(dlgHeader.dy);

    // 菜单 (0 = 无)
    tb.WriteWord(0);
    // 类 (0 = 默认)
    tb.WriteWord(0);
    // 标题
    tb.WriteString(L"JMS 插件设置");
    // 字体 (DS_SETFONT)
    tb.WriteWord(9);                // 字号
    tb.WriteWord(0);                // 字重 (0=默认)
    tb.WriteByte(0);                // 斜体
    tb.WriteByte(0);                // 字符集
    tb.WriteString(L"Microsoft Sans Serif");

    // ======== 控件 1: 静态标签 ========
    tb.AddControl(
        SS_LEFT | WS_CHILD | WS_VISIBLE,
        -1,                         // IDC_STATIC
        12, 10, 316, 12,
        0x0082,                     // Static class
        L"Just My Socks API URL（登录 JMS 后台 → Service → API 获取）:"
    );

    // ======== 控件 2: 编辑框 ========
    tb.AddControl(
        WS_BORDER | WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_LEFT | ES_AUTOHSCROLL,
        IDC_API_URL_EDIT,
        12, 26, 316, 48,
        0x0081,                     // Edit class
        L""
    );

    // ======== 控件 3: "确定"按钮 ========
    tb.AddControl(
        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        IDOK,
        170, 88, 70, 25,
        0x0080,                     // Button class
        L"确定"
    );

    // ======== 控件 4: "取消"按钮 ========
    tb.AddControl(
        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        IDCANCEL,
        250, 88, 70, 25,
        0x0080,                     // Button class
        L"取消"
    );

    // 显示对话框
    INT_PTR result = DialogBoxIndirectParamW(
        s_hInst,
        (LPDLGTEMPLATE)tb.buf.data(),
        (HWND)hParent,
        SettingsDlgProc,
        (LPARAM)this
    );

    if (result == IDOK)
    {
        // 设置已更改，立即刷新数据
        RefreshData();
        return OR_OPTION_CHANGED;
    }

    return OR_OPTION_UNCHANGED;
}

const wchar_t* CJmsPlugin::GetTooltipInfo()
{
    if (m_tooltipText.empty())
        return L"Just My Socks 流量\n等待初始化...";
    return m_tooltipText.c_str();
}

void CJmsPlugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data)
{
    if (index == EI_CONFIG_DIR && data)
    {
        m_config.SetConfigDir(data);
    }
}

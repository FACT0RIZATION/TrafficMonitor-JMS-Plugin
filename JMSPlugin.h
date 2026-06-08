#pragma once
#include "PluginInterface.h"
#include "JMSApiClient.h"
#include "JMSConfig.h"
#include "JMSItem.h"
#include <string>
#include <windows.h>

class CJmsPlugin : public ITMPlugin
{
public:
    static CJmsPlugin& Instance();

    CJmsPlugin();
    virtual ~CJmsPlugin() = default;

    // ITMPlugin 接口
    virtual int GetAPIVersion() const override { return 6; }
    virtual IPluginItem* GetItem(int index) override;
    virtual void DataRequired() override;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override;
    virtual ITMPlugin::OptionReturn ShowOptionsDialog(void* hParent) override;
    virtual const wchar_t* GetTooltipInfo() override;
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) override;

    // DLL 模块实例句柄（由 DllMain 设置）
    static HINSTANCE s_hInst;

private:
    CJmsItem m_item;
    CJmsApiClient m_apiClient;
    CJmsConfig m_config;

    std::wstring m_tooltipText;
    CRITICAL_SECTION m_cs;
    bool m_initialized;

    void RefreshData();

    // 设置对话框窗口过程
    static LRESULT CALLBACK SettingsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static bool s_dlgRegistered;

    enum { IDC_EDIT_URL = 1001, IDC_BTN_OK = 1002, IDC_BTN_CANCEL = 1003, IDC_LABEL = 1004 };
};

// DLL 导出函数
extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance();

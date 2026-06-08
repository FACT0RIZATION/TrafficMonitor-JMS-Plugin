#pragma once
#include "PluginInterface.h"
#include "JMSApiClient.h"
#include <string>

class CJmsItem : public IPluginItem
{
public:
    CJmsItem();
    virtual ~CJmsItem() = default;

    // IPluginItem 接口
    virtual const wchar_t* GetItemName() const override;
    virtual const wchar_t* GetItemId() const override;
    virtual const wchar_t* GetItemLableText() const override;
    virtual const wchar_t* GetItemValueText() const override;
    virtual const wchar_t* GetItemValueSampleText() const override;
    virtual int OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag) override;

    // 更新数据
    void UpdateData(const JmsBandwidthData& data);

    // 已用/总量字符串
    const std::wstring& GetDetailText() const { return m_detailText; }

    // 格式化字节数（公有，供其他类使用）
    static std::wstring FormatBytes(long long bytes);

private:
    std::wstring m_labelText;
    std::wstring m_valueText;
    std::wstring m_detailText;   // 完整详细信息（tooltip 用）
    bool m_dataValid;
};

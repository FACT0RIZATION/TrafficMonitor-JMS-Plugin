# TrafficMonitor JMS 流量插件

在 TrafficMonitor 任务栏显示 Just My Socks 代理的已用流量和总流量。

## 效果

```
↑82.3 GB / 465.7 GB
```

鼠标悬停显示详细信息，左键打开 JMS 后台，流量超 90% 自动显示 `!` 警告。

## 下载

前往 [Releases](../../releases) 页面下载编译好的 `JMSPlugin.dll`。

## 安装

1. 将 `JMSPlugin.dll` 复制到 TrafficMonitor 目录下的 `plugins/` 文件夹
2. 重启 TrafficMonitor
3. 右击插件 → **插件设置** → 填入你的 Just My Socks API URL
4. 确定后自动显示流量

> API URL 获取方式：登录 JMS 后台 → Service Configuration → API → 复制链接

## 自行编译

### GitHub Actions（推荐）

Fork 本仓库 → Actions 页面自动触发编译 → 下载 Artifact。

或者直接从 [Actions](https://github.com/FACT0RIZATION/TrafficMonitor-JMS-Plugin/actions) 页面下载最新编译产物。

### 本地编译

需要 Visual Studio 2022（C++ 桌面开发工作负载）。

```bash
msbuild JMSPlugin.sln /p:Configuration=Release /p:Platform=x64
```

## 操作说明

| 操作 | 行为 |
|------|------|
| 鼠标悬停 | 显示已用/总量/剩余/百分比/重置日 |
| 左键单击 | 打开 JMS 客户中心 |
| 左键双击 | 打开 Just My Socks 官网 |
| 右键 → 插件设置 | 修改 API URL（改完后自动刷新） |

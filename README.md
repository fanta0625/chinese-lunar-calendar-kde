# 岁时

岁时是一个 KDE Plasma 6 系统日历扩展。它不创建第二个日历窗口，而是在右下角系统“数字时钟”的既有月历中，为每个阳历日期显示本地农历信息。

插件 ID：`cn.loongson.suishi`

## 功能

- 在阳历日期下方显示农历日，例如 `初六`、`廿三`。
- 节气和传统节日优先显示，例如 `立春`、`中秋节`。
- 对调休日期显示 `休·初六` 或 `班·初六`。
- 鼠标悬停日期时显示完整农历日期、节气、传统节日和调休说明。
- 农历换算覆盖 1900 至 2100 年。
- 所有数据均在本地读取，运行时不访问网络。

## 数据来源与范围

- 农历：程序内置的 1900 至 2100 年压缩历法数据。
- 节气：本地计算，不依赖在线服务。
- 传统节日：根据农历日期本地推导。
- 法定节假日和调休：按年份随软件包提供，当前内置 2026 年安排。

2026 年调休数据来自国务院办公厅《关于 2026 年部分节假日安排的通知》（国办发明电〔2025〕7号）。地区性假期和单位自行安排不在本插件范围内。

## 使用效果

系统月历中的日期格会按以下优先级显示副标题：

```text
  8          23          17
初六        处暑        中秋节

  4          17
班·初七     休·初一
```

阳历日期始终由 KDE 系统日历负责显示；岁时只提供下方的小字和 tooltip，因此不会引入第二个日历入口。

## 构建依赖

目标环境为 Linux + KDE Plasma 6，需要：

- CMake 3.16 或更高版本
- C++17 编译器
- ECM（Extra CMake Modules）
- Qt 6 Core 与 Test 开发包
- KDE Frameworks 6 的 `CalendarEvents` 与 `CoreAddons` 开发包

在 Debian/Ubuntu 等发行版上，常见的依赖包包括：

```bash
sudo apt install build-essential cmake extra-cmake-modules qt6-base-dev \
  qt6-base-dev-tools libkf6declarative-dev libkf6coreaddons-dev
```

不同发行版的包名可能不同。`CalendarEvents` 由 `kdeclarative` 的开发包提供。

## 构建与安装

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
ctest --test-dir build --output-on-failure
sudo cmake --install build
```

安装完成后重启 Plasma Shell：

```bash
kquitapp6 plasmashell
kstart6 plasmashell
```

随后右键系统右下角的“数字时钟”，依次打开“配置数字时钟”与“日历”，启用“岁时”。如果系统的“其他历法”插件已设为中国农历，应关闭它，避免两个插件同时向日期格提供农历副标题。

## 更新调休数据

岁时不包含联网更新功能。每年发布正式调休安排后，新增一个数据文件即可：

```text
data/workdays/2027.json
```

文件格式与 `data/workdays/2026.json` 相同。通过软件包更新发布新文件后，插件会自动读取对应年份的数据。

插件会按 XDG 数据目录查找数据，因此放在以下路径的同名文件会优先于系统包内数据：

```text
~/.local/share/lunarcalendar/workdays/2027.json
```

这可用于本地修订或在正式软件包更新前测试新年份数据，且不需要修改插件代码。

## 项目结构

```text
src/
  lunarcalendarplugin.*   Plasma 日历事件插件
  lunarconverter.*        本地农历转换与传统节日
  solarterms.*            二十四节气计算
  workdaydata.*           按年份读取本地调休数据
data/workdays/
  2026.json               2026 年法定假日与补班数据
tests/
  lunarconvertertest.cpp  农历、节气与调休加载测试
```

## 许可

GPL-2.0-or-later。

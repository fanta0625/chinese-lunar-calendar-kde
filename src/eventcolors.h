/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QHash>
#include <QString>

// 岁时日历事件的配色。默认值内置，可被
// lunarcalendar/eventcolors.json（XDG 数据目录，用户级覆盖优先）替换。
class EventColors
{
public:
    enum class Kind {
        Festival, // 传统节日
        SolarTerm, // 二十四节气
        Workday, // 调休上班（班）
        DayOff, // 假期（休）
    };

    EventColors();

    QString colorFor(Kind kind) const;

private:
    QHash<Kind, QString> m_colors;
};

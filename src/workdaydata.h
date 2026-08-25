/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDate>
#include <QHash>
#include <QSet>
#include <QString>

#include <optional>

struct WorkdayEntry {
    enum class Kind {
        Holiday,
        Workday,
    };

    Kind kind = Kind::Holiday;
    QString name;
};

class WorkdayData
{
public:
    std::optional<WorkdayEntry> entryForDate(const QDate &date);

private:
    void loadYear(int year);

    QHash<int, QHash<QDate, WorkdayEntry>> m_entries;
    QSet<int> m_loadedYears;
};

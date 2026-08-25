/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDate>
#include <QString>

#include <optional>

struct LunarDate {
    int year = 0;
    int month = 0;
    int day = 0;
    bool isLeapMonth = false;
};

class LunarConverter
{
public:
    static std::optional<LunarDate> fromGregorian(const QDate &date);
    static int monthDays(int lunarYear, int lunarMonth, bool isLeapMonth = false);

    static QString dayLabel(const LunarDate &date);
    static QString monthLabel(const LunarDate &date);
    static QString fullLabel(const LunarDate &date);
    static QString festivalLabel(const LunarDate &date);

private:
    static int yearDays(int lunarYear);
    static int leapMonth(int lunarYear);
    static int leapMonthDays(int lunarYear);
};

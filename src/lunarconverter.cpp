/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "lunarconverter.h"

#include <QHash>
#include <QtGlobal>

#include <array>

namespace
{
constexpr int FirstLunarYear = 1900;
constexpr int LastLunarYear = 2100;

// Packed lunar year records for 1900-2100. The low four bits encode the leap
// month, the remaining bits encode regular and leap month lengths.
constexpr std::array<quint32, LastLunarYear - FirstLunarYear + 1> LunarYearInfo = {
    0x04bd8, 0x04ae0, 0x0a570, 0x054d5, 0x0d260, 0x0d950, 0x16554, 0x056a0, 0x09ad0, 0x055d2,
    0x04ae0, 0x0a5b6, 0x0a4d0, 0x0d250, 0x1d255, 0x0b540, 0x0d6a0, 0x0ada2, 0x095b0, 0x14977,
    0x04970, 0x0a4b0, 0x0b4b5, 0x06a50, 0x06d40, 0x1ab54, 0x02b60, 0x09570, 0x052f2, 0x04970,
    0x06566, 0x0d4a0, 0x0ea50, 0x06e95, 0x05ad0, 0x02b60, 0x186e3, 0x092e0, 0x1c8d7, 0x0c950,
    0x0d4a0, 0x1d8a6, 0x0b550, 0x056a0, 0x1a5b4, 0x025d0, 0x092d0, 0x0d2b2, 0x0a950, 0x0b557,
    0x06ca0, 0x0b550, 0x15355, 0x04da0, 0x0a5b0, 0x14573, 0x052b0, 0x0a9a8, 0x0e950, 0x06aa0,
    0x0aea6, 0x0ab50, 0x04b60, 0x0aae4, 0x0a570, 0x05260, 0x0f263, 0x0d950, 0x05b57, 0x056a0,
    0x096d0, 0x04dd5, 0x04ad0, 0x0a4d0, 0x0d4d4, 0x0d250, 0x0d558, 0x0b540, 0x0b5a0, 0x195a6,
    0x095b0, 0x049b0, 0x0a974, 0x0a4b0, 0x0b27a, 0x06a50, 0x06d40, 0x0af46, 0x0ab60, 0x09570,
    0x04af5, 0x04970, 0x064b0, 0x074a3, 0x0ea50, 0x06b58, 0x05ac0, 0x0ab60, 0x096d5, 0x092e0,
    0x0c960, 0x0d954, 0x0d4a0, 0x0da50, 0x07552, 0x056a0, 0x0abb7, 0x025d0, 0x092d0, 0x0cab5,
    0x0a950, 0x0b4a0, 0x0baa4, 0x0ad50, 0x055d9, 0x04ba0, 0x0a5b0, 0x15176, 0x052b0, 0x0a930,
    0x07954, 0x06aa0, 0x0ad50, 0x05b52, 0x04b60, 0x0a6e6, 0x0a4e0, 0x0d260, 0x0ea65, 0x0d530,
    0x05aa0, 0x076a3, 0x096d0, 0x04bd7, 0x04ad0, 0x0a4d0, 0x1d0b6, 0x0d250, 0x0d520, 0x0dd45,
    0x0b5a0, 0x056d0, 0x055b2, 0x049b0, 0x0a577, 0x0a4b0, 0x0aa50, 0x1b255, 0x06d20, 0x0ada0,
    0x14b63, 0x09370, 0x049f8, 0x04970, 0x064b0, 0x168a6, 0x0ea50, 0x06b20, 0x1a6c4, 0x0aae0,
    0x0a2e0, 0x0d2e3, 0x0c960, 0x0d557, 0x0d4a0, 0x0da50, 0x05d55, 0x056a0, 0x0a6d0, 0x055d4,
    0x052d0, 0x0a9b8, 0x0a950, 0x0b4a0, 0x0b6a6, 0x0ad50, 0x055a0, 0x0aba4, 0x0a5b0, 0x052b0,
    0x0b273, 0x06930, 0x07337, 0x06aa0, 0x0ad50, 0x14b55, 0x04b60, 0x0a570, 0x054e4, 0x0d160,
    0x0e968, 0x0d520, 0x0daa0, 0x16aa6, 0x056d0, 0x04ae0, 0x0a9d4, 0x0a2d0, 0x0d150, 0x0f252,
    0x0d520,
};

const quint32 yearInfo(int lunarYear)
{
    if (lunarYear < FirstLunarYear || lunarYear > LastLunarYear) {
        return 0;
    }
    return LunarYearInfo.at(lunarYear - FirstLunarYear);
}
}

int LunarConverter::yearDays(int lunarYear)
{
    const quint32 info = yearInfo(lunarYear);
    if (info == 0) {
        return 0;
    }

    int days = 348;
    for (quint32 mask = 0x8000; mask > 0x8; mask >>= 1) {
        if ((info & mask) != 0) {
            ++days;
        }
    }
    return days + leapMonthDays(lunarYear);
}

int LunarConverter::leapMonth(int lunarYear)
{
    return yearInfo(lunarYear) & 0xf;
}

int LunarConverter::leapMonthDays(int lunarYear)
{
    const int leap = leapMonth(lunarYear);
    if (leap == 0) {
        return 0;
    }
    return (yearInfo(lunarYear) & 0x10000) != 0 ? 30 : 29;
}

int LunarConverter::monthDays(int lunarYear, int lunarMonth, bool isLeapMonth)
{
    if (lunarMonth < 1 || lunarMonth > 12 || yearInfo(lunarYear) == 0) {
        return 0;
    }
    if (isLeapMonth) {
        return leapMonth(lunarYear) == lunarMonth ? leapMonthDays(lunarYear) : 0;
    }
    return (yearInfo(lunarYear) & (0x10000 >> lunarMonth)) != 0 ? 30 : 29;
}

std::optional<LunarDate> LunarConverter::fromGregorian(const QDate &date)
{
    const QDate baseDate(1900, 1, 31); // Lunar 1900-01-01.
    int daysSinceBase = baseDate.daysTo(date);
    if (daysSinceBase < 0) {
        return std::nullopt;
    }

    int lunarYear = FirstLunarYear;
    while (lunarYear <= LastLunarYear) {
        const int days = yearDays(lunarYear);
        if (daysSinceBase < days) {
            break;
        }
        daysSinceBase -= days;
        ++lunarYear;
    }
    if (lunarYear > LastLunarYear) {
        return std::nullopt;
    }

    for (int lunarMonth = 1; lunarMonth <= 12; ++lunarMonth) {
        const int regularMonthDays = monthDays(lunarYear, lunarMonth);
        if (daysSinceBase < regularMonthDays) {
            return LunarDate{lunarYear, lunarMonth, daysSinceBase + 1, false};
        }
        daysSinceBase -= regularMonthDays;

        const int leapDays = monthDays(lunarYear, lunarMonth, true);
        if (leapDays > 0) {
            if (daysSinceBase < leapDays) {
                return LunarDate{lunarYear, lunarMonth, daysSinceBase + 1, true};
            }
            daysSinceBase -= leapDays;
        }
    }

    return std::nullopt;
}

QString LunarConverter::dayLabel(const LunarDate &date)
{
    static const std::array<QString, 30> names = {
        QStringLiteral("初一"), QStringLiteral("初二"), QStringLiteral("初三"), QStringLiteral("初四"), QStringLiteral("初五"),
        QStringLiteral("初六"), QStringLiteral("初七"), QStringLiteral("初八"), QStringLiteral("初九"), QStringLiteral("初十"),
        QStringLiteral("十一"), QStringLiteral("十二"), QStringLiteral("十三"), QStringLiteral("十四"), QStringLiteral("十五"),
        QStringLiteral("十六"), QStringLiteral("十七"), QStringLiteral("十八"), QStringLiteral("十九"), QStringLiteral("二十"),
        QStringLiteral("廿一"), QStringLiteral("廿二"), QStringLiteral("廿三"), QStringLiteral("廿四"), QStringLiteral("廿五"),
        QStringLiteral("廿六"), QStringLiteral("廿七"), QStringLiteral("廿八"), QStringLiteral("廿九"), QStringLiteral("三十"),
    };

    if (date.day < 1 || date.day > static_cast<int>(names.size())) {
        return {};
    }
    return names.at(date.day - 1);
}

QString LunarConverter::monthLabel(const LunarDate &date)
{
    static const std::array<QString, 12> names = {
        QStringLiteral("正月"), QStringLiteral("二月"), QStringLiteral("三月"), QStringLiteral("四月"),
        QStringLiteral("五月"), QStringLiteral("六月"), QStringLiteral("七月"), QStringLiteral("八月"),
        QStringLiteral("九月"), QStringLiteral("十月"), QStringLiteral("冬月"), QStringLiteral("腊月"),
    };

    if (date.month < 1 || date.month > static_cast<int>(names.size())) {
        return {};
    }
    return date.isLeapMonth ? QStringLiteral("闰") + names.at(date.month - 1) : names.at(date.month - 1);
}

QString LunarConverter::fullLabel(const LunarDate &date)
{
    return QStringLiteral("农历 %1%2").arg(monthLabel(date), dayLabel(date));
}

QString LunarConverter::festivalLabel(const LunarDate &date)
{
    if (date.isLeapMonth) {
        return {};
    }

    const QString key = QStringLiteral("%1-%2").arg(date.month).arg(date.day);
    static const QHash<QString, QString> festivals = {
        {QStringLiteral("1-1"), QStringLiteral("春节")},
        {QStringLiteral("1-15"), QStringLiteral("元宵节")},
        {QStringLiteral("2-2"), QStringLiteral("龙抬头")},
        {QStringLiteral("5-5"), QStringLiteral("端午节")},
        {QStringLiteral("7-7"), QStringLiteral("七夕")},
        {QStringLiteral("7-15"), QStringLiteral("中元节")},
        {QStringLiteral("8-15"), QStringLiteral("中秋节")},
        {QStringLiteral("9-9"), QStringLiteral("重阳节")},
        {QStringLiteral("12-8"), QStringLiteral("腊八节")},
        {QStringLiteral("12-23"), QStringLiteral("小年")},
    };

    const auto festival = festivals.constFind(key);
    if (festival != festivals.cend()) {
        return festival.value();
    }
    if (date.month == 12 && date.day == monthDays(date.year, 12)) {
        return QStringLiteral("除夕");
    }
    return {};
}

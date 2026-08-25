/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "solarterms.h"

#include <QDateTime>
#include <QTime>
#include <QtMath>

#include <array>

QString SolarTerms::labelForDate(const QDate &date)
{
    if (date.year() < 1900 || date.year() > 2100) {
        return {};
    }

    static const std::array<qint64, 24> offsetsInMinutes = {
        0, 21208, 42467, 63836, 85337, 107014, 128867, 150921,
        173149, 195551, 218072, 240693, 263343, 285989, 308563,
        331033, 353350, 375494, 397447, 419210, 440795, 462224,
        483532, 504758,
    };
    static const std::array<QString, 24> names = {
        QStringLiteral("小寒"), QStringLiteral("大寒"), QStringLiteral("立春"), QStringLiteral("雨水"),
        QStringLiteral("惊蛰"), QStringLiteral("春分"), QStringLiteral("清明"), QStringLiteral("谷雨"),
        QStringLiteral("立夏"), QStringLiteral("小满"), QStringLiteral("芒种"), QStringLiteral("夏至"),
        QStringLiteral("小暑"), QStringLiteral("大暑"), QStringLiteral("立秋"), QStringLiteral("处暑"),
        QStringLiteral("白露"), QStringLiteral("秋分"), QStringLiteral("寒露"), QStringLiteral("霜降"),
        QStringLiteral("立冬"), QStringLiteral("小雪"), QStringLiteral("大雪"), QStringLiteral("冬至"),
    };

    constexpr double tropicalYearMilliseconds = 31556925974.7;
    const QDateTime base(QDate(1900, 1, 6), QTime(2, 5), Qt::UTC);
    const double yearMilliseconds = tropicalYearMilliseconds * (date.year() - 1900);

    for (size_t index = 0; index < names.size(); ++index) {
        const qint64 milliseconds = qRound64(yearMilliseconds + offsetsInMinutes.at(index) * 60'000.0);
        if (base.addMSecs(milliseconds).date() == date) {
            return names.at(index);
        }
    }
    return {};
}

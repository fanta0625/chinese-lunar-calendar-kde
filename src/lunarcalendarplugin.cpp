/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "lunarcalendarplugin.h"

#include "lunarconverter.h"
#include "solarterms.h"

#include <QStringList>

using SubLabel = CalendarEvents::CalendarEventsPlugin::SubLabel;
using SubLabelPriority = CalendarEvents::CalendarEventsPlugin::SubLabelPriority;

LunarCalendarPlugin::LunarCalendarPlugin(QObject *parent)
    : CalendarEventsPlugin(parent)
{
}

void LunarCalendarPlugin::loadEventsForDateRange(const QDate &startDate, const QDate &endDate)
{
    QHash<QDate, SubLabel> labels;
    if (!startDate.isValid() || !endDate.isValid() || endDate < startDate) {
        Q_EMIT subLabelReady(labels);
        return;
    }

    for (QDate date = startDate; date <= endDate; date = date.addDays(1)) {
        const SubLabel label = subLabelForDate(date);
        if (!label.dayLabel.isEmpty()) {
            labels.insert(date, label);
        }
    }
    Q_EMIT subLabelReady(labels);
}

SubLabel LunarCalendarPlugin::subLabelForDate(const QDate &date)
{
    SubLabel result;
    const auto lunarDate = LunarConverter::fromGregorian(date);
    if (!lunarDate) {
        return result;
    }

    const QString lunarDay = LunarConverter::dayLabel(*lunarDate);
    const QString solarTerm = SolarTerms::labelForDate(date);
    const QString festival = LunarConverter::festivalLabel(*lunarDate);
    const auto workday = m_workdayData.entryForDate(date);

    QStringList tooltipParts{LunarConverter::fullLabel(*lunarDate)};
    if (!solarTerm.isEmpty()) {
        tooltipParts.append(solarTerm);
    }
    if (!festival.isEmpty()) {
        tooltipParts.append(festival);
    }

    result.dayLabel = lunarDay;
    result.priority = SubLabelPriority::Low;

    if (workday) {
        const QString marker = workday->kind == WorkdayEntry::Kind::Workday ? QStringLiteral("班") : QStringLiteral("休");
        result.dayLabel = QStringLiteral("%1·%2").arg(marker, lunarDay);
        result.priority = SubLabelPriority::High;
        tooltipParts.append(QStringLiteral("%1：%2").arg(marker, workday->name));
    } else if (!solarTerm.isEmpty()) {
        result.dayLabel = solarTerm;
        result.priority = SubLabelPriority::Default;
    } else if (!festival.isEmpty()) {
        result.dayLabel = festival;
        result.priority = SubLabelPriority::Default;
    }

    result.label = tooltipParts.join(QStringLiteral(" · "));
    return result;
}

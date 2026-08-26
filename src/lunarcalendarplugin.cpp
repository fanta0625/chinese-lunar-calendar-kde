/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "lunarcalendarplugin.h"

#include "lunarconverter.h"
#include "solarterms.h"

#include <QDateTime>
#include <QStringList>

using SubLabel = CalendarEvents::CalendarEventsPlugin::SubLabel;
using SubLabelPriority = CalendarEvents::CalendarEventsPlugin::SubLabelPriority;
using EventData = CalendarEvents::EventData;

LunarCalendarPlugin::LunarCalendarPlugin(QObject *parent)
    : CalendarEventsPlugin(parent)
{
}

void LunarCalendarPlugin::loadEventsForDateRange(const QDate &startDate, const QDate &endDate)
{
    QHash<QDate, SubLabel> labels;
    QMultiHash<QDate, EventData> events;
    if (!startDate.isValid() || !endDate.isValid() || endDate < startDate) {
        Q_EMIT subLabelReady(labels);
        Q_EMIT dataReady(events);
        return;
    }

    for (QDate date = startDate; date <= endDate; date = date.addDays(1)) {
        const DateInfo info = infoForDate(date);

        const SubLabel label = subLabelForDate(info);
        if (!label.dayLabel.isEmpty()) {
            labels.insert(date, label);
        }

        const auto dateEvents = eventsForDate(info, date);
        for (const EventData &event : dateEvents) {
            events.insert(date, event);
        }
    }
    Q_EMIT subLabelReady(labels);
    Q_EMIT dataReady(events);
}

LunarCalendarPlugin::DateInfo LunarCalendarPlugin::infoForDate(const QDate &date)
{
    DateInfo info;
    info.lunar = LunarConverter::fromGregorian(date);
    if (!info.lunar) {
        return info;
    }
    info.solarTerm = SolarTerms::labelForDate(date);
    info.festival = LunarConverter::festivalLabel(*info.lunar);
    info.workday = m_workdayData.entryForDate(date);
    return info;
}

SubLabel LunarCalendarPlugin::subLabelForDate(const DateInfo &info) const
{
    SubLabel result;
    if (!info.lunar) {
        return result;
    }

    const QString lunarDay = LunarConverter::dayLabel(*info.lunar);

    QStringList tooltipParts{LunarConverter::fullLabel(*info.lunar)};
    if (!info.solarTerm.isEmpty()) {
        tooltipParts.append(info.solarTerm);
    }
    if (!info.festival.isEmpty()) {
        tooltipParts.append(info.festival);
    }

    result.dayLabel = lunarDay;
    result.priority = SubLabelPriority::Low;

    if (info.workday) {
        const QString marker = info.workday->kind == WorkdayEntry::Kind::Workday ? QStringLiteral("班") : QStringLiteral("休");
        result.dayLabel = QStringLiteral("%1·%2").arg(marker, lunarDay);
        result.priority = SubLabelPriority::High;
        tooltipParts.append(QStringLiteral("%1：%2").arg(marker, info.workday->name));
    } else if (!info.solarTerm.isEmpty()) {
        result.dayLabel = info.solarTerm;
        result.priority = SubLabelPriority::Default;
    } else if (!info.festival.isEmpty()) {
        result.dayLabel = info.festival;
        result.priority = SubLabelPriority::Default;
    }

    result.label = tooltipParts.join(QStringLiteral(" · "));
    return result;
}

QList<EventData> LunarCalendarPlugin::eventsForDate(const DateInfo &info, const QDate &date) const
{
    QList<EventData> events;
    if (!info.lunar) {
        return events;
    }

    const QString dateKey = date.toString(Qt::ISODate);
    const QString lunarFull = LunarConverter::fullLabel(*info.lunar);
    const auto addEvent = [&](const QString &title, const QString &description, const QString &color, const QString &uid) {
        EventData event;
        event.setTitle(title);
        event.setDescription(description);
        event.setStartDateTime(date.startOfDay());
        event.setEndDateTime(date.startOfDay());
        event.setIsAllDay(true);
        event.setEventType(EventData::Holiday);
        event.setEventColor(color);
        event.setUid(uid);
        events.append(event);
    };

    if (!info.solarTerm.isEmpty()) {
        addEvent(info.solarTerm,
                 QStringLiteral("节气 · %1").arg(lunarFull),
                 m_colors.colorFor(EventColors::Kind::SolarTerm),
                 QStringLiteral("suishi-solarterm-%1").arg(dateKey));
    }
    if (!info.festival.isEmpty()) {
        addEvent(info.festival,
                 QStringLiteral("传统节日 · %1").arg(lunarFull),
                 m_colors.colorFor(EventColors::Kind::Festival),
                 QStringLiteral("suishi-festival-%1").arg(dateKey));
    }
    if (info.workday) {
        const bool isWorkday = info.workday->kind == WorkdayEntry::Kind::Workday;
        const QString marker = isWorkday ? QStringLiteral("班") : QStringLiteral("休");
        addEvent(QStringLiteral("%1：%2").arg(marker, info.workday->name),
                 isWorkday ? QStringLiteral("调休上班") : QStringLiteral("假期"),
                 m_colors.colorFor(isWorkday ? EventColors::Kind::Workday : EventColors::Kind::DayOff),
                 QStringLiteral("suishi-%1-%2").arg(isWorkday ? QStringLiteral("workday") : QStringLiteral("dayoff"), dateKey));
    }
    return events;
}

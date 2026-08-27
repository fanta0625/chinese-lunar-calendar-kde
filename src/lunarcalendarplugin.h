/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <CalendarEvents/CalendarEventsPlugin>

#include <QDate>
#include <QList>

#include <optional>

#include "eventcolors.h"
#include "customevents.h"
#include "lunarconverter.h"
#include "workdaydata.h"

class LunarCalendarPlugin final : public CalendarEvents::CalendarEventsPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.CalendarEventsPlugin" FILE "lunarcalendarplugin.json")
    Q_INTERFACES(CalendarEvents::CalendarEventsPlugin)

public:
    explicit LunarCalendarPlugin(QObject *parent = nullptr);

    void loadEventsForDateRange(const QDate &startDate, const QDate &endDate) override;

private:
    struct DateInfo {
        std::optional<LunarDate> lunar;
        QString solarTerm;
        QString festival;
        std::optional<WorkdayEntry> workday;
    };

    DateInfo infoForDate(const QDate &date);
    CalendarEvents::CalendarEventsPlugin::SubLabel subLabelForDate(const DateInfo &info, const QDate &date) const;
    QList<CalendarEvents::EventData> eventsForDate(const DateInfo &info, const QDate &date) const;

    WorkdayData m_workdayData;
    EventColors m_colors;
    CustomEvents m_customEvents;
    bool m_showEnglishDate = false;
};

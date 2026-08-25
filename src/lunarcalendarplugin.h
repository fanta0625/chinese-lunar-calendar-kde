/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <CalendarEvents/CalendarEventsPlugin>

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
    CalendarEvents::CalendarEventsPlugin::SubLabel subLabelForDate(const QDate &date);

    WorkdayData m_workdayData;
};

/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

class ChineseLunarCalendarSettings final
{
public:
    ChineseLunarCalendarSettings() = delete;

    static bool showEnglishDate();
    static void setShowEnglishDate(bool enabled);
};

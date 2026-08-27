/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

class SuishiSettings final
{
public:
    SuishiSettings() = delete;

    static bool showEnglishDate();
    static void setShowEnglishDate(bool enabled);
};

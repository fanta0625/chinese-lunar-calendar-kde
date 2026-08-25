/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDate>
#include <QString>

class SolarTerms
{
public:
    static QString labelForDate(const QDate &date);
};

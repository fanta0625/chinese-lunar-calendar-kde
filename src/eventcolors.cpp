/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "eventcolors.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>

namespace {

QString kindKey(EventColors::Kind kind)
{
    switch (kind) {
    case EventColors::Kind::Festival:
        return QStringLiteral("festival");
    case EventColors::Kind::SolarTerm:
        return QStringLiteral("solarTerm");
    case EventColors::Kind::Workday:
        return QStringLiteral("workday");
    case EventColors::Kind::DayOff:
        return QStringLiteral("dayOff");
    }
    return QString();
}

bool isValidColor(const QString &value)
{
    // #RRGGBB 或 #AARRGGBB，与 EventData::setEventColor 的约定一致。
    static const QRegularExpression pattern(QStringLiteral("^#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?$"));
    return pattern.match(value).hasMatch();
}

} // namespace

EventColors::EventColors()
{
    m_colors.insert(Kind::Festival, QStringLiteral("#c62828"));
    m_colors.insert(Kind::SolarTerm, QStringLiteral("#2e7d32"));
    m_colors.insert(Kind::Workday, QStringLiteral("#1565c0"));
    m_colors.insert(Kind::DayOff, QStringLiteral("#ef6c00"));

    const QString filePath = QStandardPaths::locate(
        QStandardPaths::GenericDataLocation, QStringLiteral("lunarcalendar/eventcolors.json"));
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return;
    }

    const QJsonObject root = document.object();
    for (auto it = m_colors.begin(); it != m_colors.end(); ++it) {
        const QString value = root.value(kindKey(it.key())).toString();
        if (isValidColor(value)) {
            it.value() = value;
        }
    }
}

QString EventColors::colorFor(Kind kind) const
{
    return m_colors.value(kind);
}

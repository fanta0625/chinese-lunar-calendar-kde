/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "workdaydata.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStandardPaths>

std::optional<WorkdayEntry> WorkdayData::entryForDate(const QDate &date)
{
    if (!date.isValid()) {
        return std::nullopt;
    }
    loadYear(date.year());

    const auto yearEntries = m_entries.constFind(date.year());
    if (yearEntries == m_entries.cend()) {
        return std::nullopt;
    }
    const auto entry = yearEntries->constFind(date);
    if (entry == yearEntries->cend()) {
        return std::nullopt;
    }
    return entry.value();
}

void WorkdayData::loadYear(int year)
{
    if (m_loadedYears.contains(year)) {
        return;
    }
    m_loadedYears.insert(year);

    const QString relativePath = QStringLiteral("lunarcalendar/workdays/%1.json").arg(year);
    const QString filePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, relativePath);
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
    if (root.value(QStringLiteral("schemaVersion")).toInt() != 1 || root.value(QStringLiteral("year")).toInt() != year) {
        return;
    }

    QHash<QDate, WorkdayEntry> entries;
    const QJsonArray days = root.value(QStringLiteral("days")).toArray();
    for (const QJsonValue &value : days) {
        const QJsonObject item = value.toObject();
        const QDate date = QDate::fromString(item.value(QStringLiteral("date")).toString(), Qt::ISODate);
        const QString kind = item.value(QStringLiteral("kind")).toString();
        if (!date.isValid() || date.year() != year || (kind != QStringLiteral("holiday") && kind != QStringLiteral("workday"))) {
            continue;
        }

        entries.insert(date, WorkdayEntry{
                                 kind == QStringLiteral("holiday") ? WorkdayEntry::Kind::Holiday : WorkdayEntry::Kind::Workday,
                                 item.value(QStringLiteral("name")).toString(),
                             });
    }
    m_entries.insert(year, entries);
}

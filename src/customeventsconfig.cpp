/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "customeventsconfig.h"

#include <algorithm>
#include <iterator>

#include <QUuid>

namespace {

bool eventFromInput(const QString &id,
                    const QString &name,
                    const QString &dateText,
                    const QString &color,
                    bool repeatYearly,
                    CustomEvent *event,
                    QString *error)
{
    const QString normalizedDate = dateText.trimmed();
    const QDate parsedDate = QDate::fromString(normalizedDate, Qt::ISODate);
    if (!parsedDate.isValid() || parsedDate.toString(Qt::ISODate) != normalizedDate) {
        if (error) {
            *error = QStringLiteral("事件日期无效，请使用 YYYY-MM-DD 格式");
        }
        return false;
    }

    CustomEvent result;
    result.id = id.trimmed();
    result.date = parsedDate;
    result.name = name.trimmed();
    result.color = color.trimmed();
    result.repeatYearly = repeatYearly;

    if (!CustomEvents::validateEvent(result, error)) {
        return false;
    }

    *event = result;
    return true;
}

} // namespace

CustomEventsModel::CustomEventsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    reload();
}

int CustomEventsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_events.size();
}

QVariant CustomEventsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_events.size()) {
        return QVariant();
    }

    const CustomEvent &event = m_events.at(index.row());
    switch (role) {
    case IdRole:
        return event.id;
    case DateRole:
        return event.date.toString(Qt::ISODate);
    case NameRole:
        return event.name;
    case ColorRole:
        return event.color;
    case RepeatYearlyRole:
        return event.repeatYearly;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> CustomEventsModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {DateRole, "date"},
        {NameRole, "name"},
        {ColorRole, "color"},
        {RepeatYearlyRole, "repeatYearly"},
    };
}

QString CustomEventsModel::errorString() const
{
    return m_errorString;
}

bool CustomEventsModel::usingSystemDefaults() const
{
    return m_store.source() == CustomEvents::Source::System;
}

bool CustomEventsModel::hasUserFile() const
{
    return m_store.source() == CustomEvents::Source::User;
}

bool CustomEventsModel::reload()
{
    QString error;
    const bool loaded = m_store.reloadIfChanged(&error);
    refreshModel();
    setError(loaded ? QString() : error);
    return loaded;
}

bool CustomEventsModel::addEvent(const QString &name, const QString &date, const QString &color, bool repeatYearly)
{
    CustomEvent event;
    QString error;
    if (!eventFromInput(QUuid::createUuid().toString(QUuid::WithoutBraces), name, date, color, repeatYearly, &event, &error)) {
        setError(error);
        return false;
    }

    QList<CustomEvent> nextEvents = m_events;
    nextEvents.append(event);
    return saveEvents(nextEvents);
}

bool CustomEventsModel::updateEvent(const QString &id,
                                    const QString &name,
                                    const QString &date,
                                    const QString &color,
                                    bool repeatYearly)
{
    const QString normalizedId = id.trimmed();
    const auto eventIt = std::find_if(m_events.cbegin(), m_events.cend(), [&normalizedId](const CustomEvent &event) {
        return event.id == normalizedId;
    });
    if (eventIt == m_events.cend()) {
        setError(QStringLiteral("找不到要修改的事件"));
        return false;
    }

    CustomEvent replacement;
    QString error;
    if (!eventFromInput(normalizedId, name, date, color, repeatYearly, &replacement, &error)) {
        setError(error);
        return false;
    }

    QList<CustomEvent> nextEvents = m_events;
    nextEvents[static_cast<int>(std::distance(m_events.cbegin(), eventIt))] = replacement;
    return saveEvents(nextEvents);
}

bool CustomEventsModel::removeEvent(const QString &id)
{
    const QString normalizedId = id.trimmed();
    const auto eventIt = std::find_if(m_events.cbegin(), m_events.cend(), [&normalizedId](const CustomEvent &event) {
        return event.id == normalizedId;
    });
    if (eventIt == m_events.cend()) {
        setError(QStringLiteral("找不到要删除的事件"));
        return false;
    }

    QList<CustomEvent> nextEvents = m_events;
    nextEvents.removeAt(static_cast<int>(std::distance(m_events.cbegin(), eventIt)));
    return saveEvents(nextEvents);
}

bool CustomEventsModel::resetToSystemDefaults()
{
    QString error;
    const bool reset = m_store.resetToSystemDefaults(&error);
    refreshModel();
    setError(reset ? QString() : error);
    return reset;
}

void CustomEventsModel::clearError()
{
    setError(QString());
}

bool CustomEventsModel::saveEvents(const QList<CustomEvent> &events)
{
    QString error;
    if (!m_store.saveUserEvents(events, &error)) {
        setError(error);
        return false;
    }

    refreshModel();
    setError(QString());
    return true;
}

void CustomEventsModel::refreshModel()
{
    beginResetModel();
    m_events = m_store.events();
    endResetModel();

    Q_EMIT sourceChanged();
}

void CustomEventsModel::setError(const QString &error)
{
    if (m_errorString == error) {
        return;
    }
    m_errorString = error;
    Q_EMIT errorStringChanged();
}

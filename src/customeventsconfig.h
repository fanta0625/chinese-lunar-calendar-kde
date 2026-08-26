/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QVariant>

#include "customevents.h"

class CustomEventsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(bool usingSystemDefaults READ usingSystemDefaults NOTIFY sourceChanged)
    Q_PROPERTY(bool hasUserFile READ hasUserFile NOTIFY sourceChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        DateRole,
        NameRole,
        ColorRole,
        RepeatTypeRole,
        RepeatIntervalRole,
        RepeatUnitRole,
    };
    Q_ENUM(Role)

    explicit CustomEventsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString errorString() const;
    bool usingSystemDefaults() const;
    bool hasUserFile() const;

    Q_INVOKABLE bool reload();
    Q_INVOKABLE bool addEvent(const QString &name,
                              const QString &date,
                              const QString &color,
                              const QString &repeatType,
                              int repeatInterval,
                              const QString &repeatUnit);
    Q_INVOKABLE bool updateEvent(const QString &id,
                                 const QString &name,
                                 const QString &date,
                                 const QString &color,
                                 const QString &repeatType,
                                 int repeatInterval,
                                 const QString &repeatUnit);
    Q_INVOKABLE bool removeEvent(const QString &id);
    Q_INVOKABLE bool resetToSystemDefaults();
    Q_INVOKABLE void clearError();

Q_SIGNALS:
    void errorStringChanged();
    void sourceChanged();

private:
    bool saveEvents(const QList<CustomEvent> &events);
    void refreshModel();
    void setError(const QString &error);

    CustomEvents m_store;
    QList<CustomEvent> m_events;
    QString m_errorString;
};

/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QByteArray>
#include <QDate>
#include <QList>
#include <QString>

struct CustomEvent {
    enum class RepeatType {
        None,
        Daily,
        Weekly,
        Monthly,
        Yearly,
        Custom,
    };

    enum class RepeatUnit {
        Day,
        Week,
        Month,
        Year,
    };

    QString id;
    QDate date;
    QString name;
    QString description;
    QString color;
    RepeatType repeatType = RepeatType::None;
    int repeatInterval = 1;
    RepeatUnit repeatUnit = RepeatUnit::Day;
};

class CustomEvents
{
public:
    enum class Source {
        None,
        User,
        System,
    };

    CustomEvents();

    // The path arguments are intended for deterministic tests. Production code
    // uses the XDG paths when both arguments are empty.
    CustomEvents(const QString &userFilePath, const QString &systemFilePath);

    bool reloadIfChanged(QString *error = nullptr);

    const QList<CustomEvent> &events() const;
    QList<CustomEvent> eventsForDate(const QDate &date) const;

    // Saving through this method adopts system events on the first edit.
    bool saveUserEvents(const QList<CustomEvent> &events, QString *error = nullptr);
    bool resetToSystemDefaults(QString *error = nullptr);

    Source source() const;
    QString sourcePath() const;
    QString errorString() const;

    static QString repeatTypeToString(CustomEvent::RepeatType type);
    static bool repeatTypeFromString(const QString &value, CustomEvent::RepeatType *type);
    static QString repeatUnitToString(CustomEvent::RepeatUnit unit);
    static bool repeatUnitFromString(const QString &value, CustomEvent::RepeatUnit *unit);
    static bool validateEvent(const CustomEvent &event, QString *error = nullptr);
    static bool validateEvents(const QList<CustomEvent> &events, QString *error = nullptr);

private:
    struct FileStamp {
        QString path;
        qint64 size = -1;
        qint64 modifiedMsecs = -1;
        bool exists = false;
    };

    QString userFilePath() const;
    QString effectiveFilePath() const;
    static FileStamp fileStamp(const QString &path);
    static bool sameStamp(const FileStamp &left, const FileStamp &right);
    static bool loadFile(const QString &path, QList<CustomEvent> *events, QString *error);
    static QByteArray serializeEvents(const QList<CustomEvent> &events);
    static bool writeFile(const QString &path, const QList<CustomEvent> &events, QString *error);
    static QString sourceError(const QString &path, const QString &details);

    QList<CustomEvent> m_events;
    QString m_sourcePath;
    QString m_lastError;
    Source m_source = Source::None;
    FileStamp m_stamp;
    bool m_hasStamp = false;

    QString m_userFilePathOverride;
    QString m_systemFilePathOverride;
};

/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "customevents.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

namespace {

bool writeFile(const QString &path, const QByteArray &content)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return file.write(content) == content.size();
}

CustomEvent makeEvent(const QString &id, const QDate &date, const QString &name, bool repeatYearly = false)
{
    return CustomEvent{id, date, name, QStringLiteral("#8e24aa"), repeatYearly};
}

} // namespace

class CustomEventsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void usesSystemFileUntilFirstEdit();
    void userFileTakesPrecedence();
    void emptyUserFileBlocksSystemDefaults();
    void expandsYearlyEvents();
    void reloadsWhenFileChanges();
    void rejectsInvalidData();
    void resetsToSystemDefaults();
};

void CustomEventsTest::usesSystemFileUntilFirstEdit()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userPath = directory.filePath(QStringLiteral("user/lunarcalendar/events.json"));
    const QString systemPath = directory.filePath(QStringLiteral("system/lunarcalendar/events.json"));
    QVERIFY(writeFile(systemPath, R"({
        "schemaVersion": 1,
        "events": [
            {"id": "system-event", "date": "2026-10-01", "name": "系统事件", "color": "#c62828", "repeatYearly": false}
        ]
    })"));

    CustomEvents events(userPath, systemPath);
    QString error;
    QVERIFY2(events.reloadIfChanged(&error), qPrintable(error));
    QVERIFY(events.source() == CustomEvents::Source::System);
    QCOMPARE(events.events().size(), 1);
    QCOMPARE(events.events().first().name, QStringLiteral("系统事件"));

    QList<CustomEvent> editedEvents = events.events();
    editedEvents.append(makeEvent(QStringLiteral("user-event"), QDate(2026, 12, 1), QStringLiteral("用户事件")));
    QVERIFY2(events.saveUserEvents(editedEvents, &error), qPrintable(error));
    QVERIFY(events.source() == CustomEvents::Source::User);
    QVERIFY(QFileInfo(userPath).isFile());
    QCOMPARE(events.events().size(), 2);

    CustomEvents reloaded(userPath, systemPath);
    QVERIFY2(reloaded.reloadIfChanged(&error), qPrintable(error));
    QVERIFY(reloaded.source() == CustomEvents::Source::User);
    QCOMPARE(reloaded.events().size(), 2);
}

void CustomEventsTest::userFileTakesPrecedence()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userPath = directory.filePath(QStringLiteral("user/lunarcalendar/events.json"));
    const QString systemPath = directory.filePath(QStringLiteral("system/lunarcalendar/events.json"));
    QVERIFY(writeFile(systemPath, R"({
        "schemaVersion": 1,
        "events": [
            {"id": "system-event", "date": "2026-10-01", "name": "系统事件", "color": "#c62828", "repeatYearly": false}
        ]
    })"));
    QVERIFY(writeFile(userPath, R"({
        "schemaVersion": 1,
        "events": [
            {"id": "user-event", "date": "2026-11-01", "name": "用户事件", "color": "#8e24aa", "repeatYearly": false}
        ]
    })"));

    CustomEvents events(userPath, systemPath);
    QString error;
    QVERIFY2(events.reloadIfChanged(&error), qPrintable(error));
    QVERIFY(events.source() == CustomEvents::Source::User);
    QCOMPARE(events.events().size(), 1);
    QCOMPARE(events.events().first().name, QStringLiteral("用户事件"));
}

void CustomEventsTest::emptyUserFileBlocksSystemDefaults()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userPath = directory.filePath(QStringLiteral("user/lunarcalendar/events.json"));
    const QString systemPath = directory.filePath(QStringLiteral("system/lunarcalendar/events.json"));
    QVERIFY(writeFile(systemPath, R"({
        "schemaVersion": 1,
        "events": [
            {"id": "system-event", "date": "2026-10-01", "name": "系统事件", "color": "#c62828", "repeatYearly": false}
        ]
    })"));

    CustomEvents events(userPath, systemPath);
    QString error;
    QVERIFY2(events.reloadIfChanged(&error), qPrintable(error));
    QVERIFY2(events.saveUserEvents(QList<CustomEvent>{}, &error), qPrintable(error));
    QVERIFY(events.source() == CustomEvents::Source::User);
    QVERIFY(events.events().isEmpty());
    QVERIFY(QFileInfo(userPath).isFile());
}

void CustomEventsTest::expandsYearlyEvents()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userPath = directory.filePath(QStringLiteral("user/lunarcalendar/events.json"));
    const QString systemPath = directory.filePath(QStringLiteral("system/lunarcalendar/events.json"));
    CustomEvents events(userPath, systemPath);
    QString error;
    QVERIFY2(events.saveUserEvents({makeEvent(QStringLiteral("birthday"), QDate(2026, 10, 1), QStringLiteral("生日"), true),
                                    makeEvent(QStringLiteral("leap-day"), QDate(2024, 2, 29), QStringLiteral("闰日"), true),
                                    makeEvent(QStringLiteral("one-time"), QDate(2026, 10, 2), QStringLiteral("一次性事件"))},
                                   &error),
             qPrintable(error));

    QCOMPARE(events.eventsForDate(QDate(2030, 10, 1)).size(), 1);
    QCOMPARE(events.eventsForDate(QDate(2030, 10, 1)).first().name, QStringLiteral("生日"));
    QCOMPARE(events.eventsForDate(QDate(2030, 10, 2)).size(), 1);
    QCOMPARE(events.eventsForDate(QDate(2030, 10, 2)).first().name, QStringLiteral("一次性事件"));
    QVERIFY(events.eventsForDate(QDate(2030, 2, 29)).isEmpty());
    QCOMPARE(events.eventsForDate(QDate(2032, 2, 29)).size(), 1);
}

void CustomEventsTest::reloadsWhenFileChanges()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userPath = directory.filePath(QStringLiteral("user/lunarcalendar/events.json"));
    const QString systemPath = directory.filePath(QStringLiteral("system/lunarcalendar/events.json"));
    QVERIFY(writeFile(systemPath, R"({
        "schemaVersion": 1,
        "events": [
            {"id": "system-event", "date": "2026-10-01", "name": "系统事件", "color": "#c62828", "repeatYearly": false}
        ]
    })"));

    CustomEvents events(userPath, systemPath);
    QString error;
    QVERIFY2(events.reloadIfChanged(&error), qPrintable(error));
    QCOMPARE(events.events().first().name, QStringLiteral("系统事件"));

    QVERIFY(writeFile(systemPath, R"({
        "schemaVersion": 1,
        "events": [
            {"id": "system-event", "date": "2026-10-01", "name": "更新后的系统事件", "color": "#c62828", "repeatYearly": false}
        ]
    })"));
    QVERIFY2(events.reloadIfChanged(&error), qPrintable(error));
    QCOMPARE(events.events().first().name, QStringLiteral("更新后的系统事件"));
}

void CustomEventsTest::rejectsInvalidData()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userPath = directory.filePath(QStringLiteral("user/lunarcalendar/events.json"));
    const QString systemPath = directory.filePath(QStringLiteral("system/lunarcalendar/events.json"));
    QVERIFY(writeFile(systemPath, R"({
        "schemaVersion": 1,
        "events": [
            {"id": "bad-event", "date": "2026-10-01", "name": "坏颜色", "color": "red", "repeatYearly": false}
        ]
    })"));

    CustomEvents events(userPath, systemPath);
    QString error;
    QVERIFY(!events.reloadIfChanged(&error));
    QVERIFY(!error.isEmpty());
    QVERIFY(events.events().isEmpty());
    QVERIFY(!events.saveUserEvents({makeEvent(QStringLiteral("new-event"), QDate(2026, 10, 1), QStringLiteral("新事件"))}, &error));
    QVERIFY(!QFileInfo(userPath).exists());
}

void CustomEventsTest::resetsToSystemDefaults()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userPath = directory.filePath(QStringLiteral("user/lunarcalendar/events.json"));
    const QString systemPath = directory.filePath(QStringLiteral("system/lunarcalendar/events.json"));
    QVERIFY(writeFile(systemPath, R"({
        "schemaVersion": 1,
        "events": [
            {"id": "system-event", "date": "2026-10-01", "name": "系统事件", "color": "#c62828", "repeatYearly": false}
        ]
    })"));

    CustomEvents events(userPath, systemPath);
    QString error;
    QVERIFY2(events.saveUserEvents({makeEvent(QStringLiteral("user-event"), QDate(2026, 12, 1), QStringLiteral("用户事件"))}, &error),
             qPrintable(error));
    QVERIFY(events.source() == CustomEvents::Source::User);

    QVERIFY2(events.resetToSystemDefaults(&error), qPrintable(error));
    QVERIFY(events.source() == CustomEvents::Source::System);
    QVERIFY(!QFileInfo(userPath).exists());
    QCOMPARE(events.events().size(), 1);
    QCOMPARE(events.events().first().name, QStringLiteral("系统事件"));
}

QTEST_MAIN(CustomEventsTest)

#include "customeventstest.moc"

/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "customevents.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
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

CustomEvent makeEvent(const QString &id,
                      const QDate &date,
                      const QString &name,
                      CustomEvent::RepeatType repeatType = CustomEvent::RepeatType::None,
                      int repeatInterval = 1,
                      CustomEvent::RepeatUnit repeatUnit = CustomEvent::RepeatUnit::Day,
                      const QString &description = QString())
{
    CustomEvent event;
    event.id = id;
    event.date = date;
    event.name = name;
    event.description = description;
    event.color = QStringLiteral("#8e24aa");
    event.repeatType = repeatType;
    event.repeatInterval = repeatInterval;
    event.repeatUnit = repeatUnit;
    return event;
}

bool hasEventNamed(const QList<CustomEvent> &events, const QString &name)
{
    for (const CustomEvent &event : events) {
        if (event.name == name) {
            return true;
        }
    }
    return false;
}

} // namespace

class CustomEventsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void usesSystemFileUntilFirstEdit();
    void userFileTakesPrecedence();
    void emptyUserFileBlocksSystemDefaults();
    void expandsRecurringEvents();
    void reloadsWhenFileChanges();
    void readsVersionTwoWithoutDescription();
    void readsVersionThreeDescriptions();
    void writesVersionThreeDescriptions();
    void rejectsInvalidData();
    void rejectsInvalidDescriptions();
    void rejectsInvalidRecurrence();
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
    QCOMPARE(events.events().first().description, QString());

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

void CustomEventsTest::expandsRecurringEvents()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userPath = directory.filePath(QStringLiteral("user/lunarcalendar/events.json"));
    const QString systemPath = directory.filePath(QStringLiteral("system/lunarcalendar/events.json"));
    CustomEvents events(userPath, systemPath);
    QString error;
    QVERIFY2(events.saveUserEvents({makeEvent(QStringLiteral("daily"),
                                              QDate(2026, 1, 10),
                                              QStringLiteral("每天"),
                                              CustomEvent::RepeatType::Daily),
                                    makeEvent(QStringLiteral("weekly"),
                                              QDate(2026, 1, 10),
                                              QStringLiteral("每周"),
                                              CustomEvent::RepeatType::Weekly,
                                              1,
                                              CustomEvent::RepeatUnit::Week),
                                    makeEvent(QStringLiteral("monthly"),
                                              QDate(2026, 1, 31),
                                              QStringLiteral("每月"),
                                              CustomEvent::RepeatType::Monthly,
                                              1,
                                              CustomEvent::RepeatUnit::Month),
                                    makeEvent(QStringLiteral("birthday"),
                                              QDate(2026, 10, 1),
                                              QStringLiteral("每年"),
                                              CustomEvent::RepeatType::Yearly,
                                              1,
                                              CustomEvent::RepeatUnit::Year),
                                    makeEvent(QStringLiteral("every-two-weeks"),
                                              QDate(2026, 1, 5),
                                              QStringLiteral("每两周"),
                                              CustomEvent::RepeatType::Custom,
                                              2,
                                              CustomEvent::RepeatUnit::Week),
                                    makeEvent(QStringLiteral("every-two-days"),
                                              QDate(2026, 1, 1),
                                              QStringLiteral("每两天"),
                                              CustomEvent::RepeatType::Custom,
                                              2,
                                              CustomEvent::RepeatUnit::Day),
                                    makeEvent(QStringLiteral("every-three-months"),
                                              QDate(2026, 1, 15),
                                              QStringLiteral("每三个月"),
                                              CustomEvent::RepeatType::Custom,
                                              3,
                                              CustomEvent::RepeatUnit::Month),
                                    makeEvent(QStringLiteral("every-two-years"),
                                              QDate(2026, 5, 20),
                                              QStringLiteral("每两年"),
                                              CustomEvent::RepeatType::Custom,
                                              2,
                                              CustomEvent::RepeatUnit::Year),
                                    makeEvent(QStringLiteral("leap-day"),
                                              QDate(2024, 2, 29),
                                              QStringLiteral("闰日"),
                                              CustomEvent::RepeatType::Yearly,
                                              1,
                                              CustomEvent::RepeatUnit::Year),
                                    makeEvent(QStringLiteral("one-time"), QDate(2026, 10, 2), QStringLiteral("一次性事件"))},
                                   &error),
             qPrintable(error));

    const auto januaryEleventh = events.eventsForDate(QDate(2026, 1, 11));
    QVERIFY(hasEventNamed(januaryEleventh, QStringLiteral("每天")));
    QVERIFY(hasEventNamed(januaryEleventh, QStringLiteral("每两天")));
    QVERIFY(hasEventNamed(events.eventsForDate(QDate(2026, 1, 3)), QStringLiteral("每两天")));
    QVERIFY(!hasEventNamed(events.eventsForDate(QDate(2026, 1, 2)), QStringLiteral("每两天")));
    const auto weeklyDate = events.eventsForDate(QDate(2026, 1, 17));
    QVERIFY(hasEventNamed(weeklyDate, QStringLiteral("每周")));
    QVERIFY(!hasEventNamed(events.eventsForDate(QDate(2026, 1, 12)), QStringLiteral("每周")));
    const auto monthlyDate = events.eventsForDate(QDate(2026, 3, 31));
    QVERIFY(hasEventNamed(monthlyDate, QStringLiteral("每月")));
    QVERIFY(!hasEventNamed(events.eventsForDate(QDate(2026, 2, 28)), QStringLiteral("每月")));
    const auto yearlyDate = events.eventsForDate(QDate(2030, 10, 1));
    QVERIFY(hasEventNamed(yearlyDate, QStringLiteral("每年")));
    QVERIFY(events.eventsForDate(QDate(2025, 10, 1)).isEmpty());
    const auto everyTwoWeeksDate = events.eventsForDate(QDate(2026, 1, 19));
    QVERIFY(hasEventNamed(everyTwoWeeksDate, QStringLiteral("每两周")));
    QVERIFY(!hasEventNamed(events.eventsForDate(QDate(2026, 1, 12)), QStringLiteral("每两周")));
    const auto everyThreeMonthsDate = events.eventsForDate(QDate(2026, 4, 15));
    QVERIFY(hasEventNamed(everyThreeMonthsDate, QStringLiteral("每三个月")));
    QVERIFY(hasEventNamed(events.eventsForDate(QDate(2026, 7, 15)), QStringLiteral("每三个月")));
    QVERIFY(!hasEventNamed(events.eventsForDate(QDate(2026, 3, 15)), QStringLiteral("每三个月")));
    const auto everyTwoYearsDate = events.eventsForDate(QDate(2028, 5, 20));
    QVERIFY(hasEventNamed(everyTwoYearsDate, QStringLiteral("每两年")));
    // 一次性事件只出现在其自身日期，不跨年。
    QVERIFY(!hasEventNamed(events.eventsForDate(QDate(2030, 10, 2)), QStringLiteral("一次性事件")));
    QVERIFY(hasEventNamed(events.eventsForDate(QDate(2026, 10, 2)), QStringLiteral("一次性事件")));
    QVERIFY(events.eventsForDate(QDate(2030, 2, 29)).isEmpty());
    QVERIFY(hasEventNamed(events.eventsForDate(QDate(2032, 2, 29)), QStringLiteral("闰日")));
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

void CustomEventsTest::readsVersionTwoWithoutDescription()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userPath = directory.filePath(QStringLiteral("user/lunarcalendar/events.json"));
    const QString systemPath = directory.filePath(QStringLiteral("system/lunarcalendar/events.json"));
    QVERIFY(writeFile(systemPath, R"({
        "schemaVersion": 2,
        "events": [
            {
                "id": "legacy-event",
                "date": "2026-10-01",
                "name": "旧版事件",
                "color": "#8e24aa",
                "recurrence": {"type": "none", "interval": 1, "unit": "day"}
            }
        ]
    })"));

    CustomEvents events(userPath, systemPath);
    QString error;
    QVERIFY2(events.reloadIfChanged(&error), qPrintable(error));
    QCOMPARE(events.events().size(), 1);
    QCOMPARE(events.events().first().description, QString());
}

void CustomEventsTest::readsVersionThreeDescriptions()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userPath = directory.filePath(QStringLiteral("user/lunarcalendar/events.json"));
    const QString systemPath = directory.filePath(QStringLiteral("system/lunarcalendar/events.json"));
    QVERIFY(writeFile(systemPath, R"({
        "schemaVersion": 3,
        "events": [
            {
                "id": "with-description",
                "date": "2026-10-01",
                "name": "有详情",
                "description": "准备礼物\n写卡片",
                "color": "#8e24aa",
                "recurrence": {"type": "none", "interval": 1, "unit": "day"}
            },
            {
                "id": "without-description",
                "date": "2026-10-02",
                "name": "无详情",
                "color": "#8e24aa",
                "recurrence": {"type": "none", "interval": 1, "unit": "day"}
            }
        ]
    })"));

    CustomEvents events(userPath, systemPath);
    QString error;
    QVERIFY2(events.reloadIfChanged(&error), qPrintable(error));
    QCOMPARE(events.events().size(), 2);
    QCOMPARE(events.events().at(0).description, QStringLiteral("准备礼物\n写卡片"));
    QCOMPARE(events.events().at(1).description, QString());
}

void CustomEventsTest::writesVersionThreeDescriptions()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userPath = directory.filePath(QStringLiteral("user/lunarcalendar/events.json"));
    const QString systemPath = directory.filePath(QStringLiteral("system/lunarcalendar/events.json"));
    CustomEvents events(userPath, systemPath);
    QString error;
    const CustomEvent event = makeEvent(QStringLiteral("with-description"),
                                         QDate(2026, 10, 1),
                                         QStringLiteral("有详情"),
                                         CustomEvent::RepeatType::None,
                                         1,
                                         CustomEvent::RepeatUnit::Day,
                                         QStringLiteral("准备礼物\n写卡片"));
    QVERIFY2(events.saveUserEvents({event}, &error), qPrintable(error));

    QFile file(userPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    QVERIFY2(parseError.error == QJsonParseError::NoError, qPrintable(parseError.errorString()));
    QCOMPARE(document.object().value(QStringLiteral("schemaVersion")).toInt(), 3);
    const QJsonArray serializedEvents = document.object().value(QStringLiteral("events")).toArray();
    QCOMPARE(serializedEvents.size(), 1);
    QCOMPARE(serializedEvents.first().toObject().value(QStringLiteral("description")).toString(),
             QStringLiteral("准备礼物\n写卡片"));

    CustomEvents reloaded(userPath, systemPath);
    QVERIFY2(reloaded.reloadIfChanged(&error), qPrintable(error));
    QCOMPARE(reloaded.events().first().description, QStringLiteral("准备礼物\n写卡片"));
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

void CustomEventsTest::rejectsInvalidDescriptions()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userPath = directory.filePath(QStringLiteral("user/lunarcalendar/events.json"));
    const QString systemPath = directory.filePath(QStringLiteral("system/lunarcalendar/events.json"));
    QVERIFY(writeFile(systemPath, R"({
        "schemaVersion": 3,
        "events": [
            {
                "id": "bad-description",
                "date": "2026-10-01",
                "name": "坏详情",
                "description": 42,
                "color": "#8e24aa",
                "recurrence": {"type": "none", "interval": 1, "unit": "day"}
            }
        ]
    })"));

    CustomEvents events(userPath, systemPath);
    QString error;
    QVERIFY(!events.reloadIfChanged(&error));
    QVERIFY(error.contains(QStringLiteral("详情")));
    QVERIFY(events.events().isEmpty());
}

void CustomEventsTest::rejectsInvalidRecurrence()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userPath = directory.filePath(QStringLiteral("user/lunarcalendar/events.json"));
    const QString systemPath = directory.filePath(QStringLiteral("system/lunarcalendar/events.json"));
    QVERIFY(writeFile(systemPath, R"({
        "schemaVersion": 2,
        "events": [
            {
                "id": "bad-recurrence",
                "date": "2026-10-01",
                "name": "坏重复",
                "color": "#8e24aa",
                "recurrence": {"type": "custom", "interval": 0, "unit": "week"}
            }
        ]
    })"));

    CustomEvents events(userPath, systemPath);
    QString error;
    QVERIFY(!events.reloadIfChanged(&error));
    QVERIFY(error.contains(QStringLiteral("recurrence")));
    QVERIFY(events.events().isEmpty());

    CustomEvent invalid = makeEvent(QStringLiteral("invalid"), QDate(2026, 10, 1), QStringLiteral("无效"), CustomEvent::RepeatType::Custom, 0, CustomEvent::RepeatUnit::Week);
    QVERIFY(!CustomEvents::validateEvent(invalid, &error));
    QVERIFY(error.contains(QStringLiteral("正整数")));
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

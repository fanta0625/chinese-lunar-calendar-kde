/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "eventcolors.h"
#include "lunarconverter.h"
#include "solarterms.h"
#include "workdaydata.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtTest>

class LunarConverterTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void convertsKnownNewYears();
    void identifiesFestivals();
    void identifiesSolarTerms();
    void loadsLocalWorkdayData();
    void loadsEventColorsWithOverride();
    void rejectsUnsupportedDates();
};

void LunarConverterTest::convertsKnownNewYears()
{
    const auto lunar2024 = LunarConverter::fromGregorian(QDate(2024, 2, 10));
    QVERIFY(lunar2024);
    QCOMPARE(lunar2024->year, 2024);
    QCOMPARE(lunar2024->month, 1);
    QCOMPARE(lunar2024->day, 1);

    const auto lunar2026 = LunarConverter::fromGregorian(QDate(2026, 2, 17));
    QVERIFY(lunar2026);
    QCOMPARE(lunar2026->year, 2026);
    QCOMPARE(lunar2026->month, 1);
    QCOMPARE(lunar2026->day, 1);
    QCOMPARE(LunarConverter::dayLabel(*lunar2026), QStringLiteral("初一"));
}

void LunarConverterTest::identifiesFestivals()
{
    const auto lunarDate = LunarConverter::fromGregorian(QDate(2026, 2, 17));
    QVERIFY(lunarDate);
    QCOMPARE(LunarConverter::festivalLabel(*lunarDate), QStringLiteral("春节"));
}

void LunarConverterTest::identifiesSolarTerms()
{
    QCOMPARE(SolarTerms::labelForDate(QDate(2026, 2, 4)), QStringLiteral("立春"));
    QCOMPARE(SolarTerms::labelForDate(QDate(2026, 8, 23)), QStringLiteral("处暑"));
}

void LunarConverterTest::loadsLocalWorkdayData()
{
    QStandardPaths::setTestModeEnabled(true);
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/lunarcalendar/workdays/2026.json");
    QVERIFY(QDir().mkpath(QFileInfo(path).dir().path()));

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(R"({
        "schemaVersion": 1,
        "year": 2026,
        "days": [
            {"date": "2026-02-17", "kind": "holiday", "name": "春节假期"},
            {"date": "2026-02-28", "kind": "workday", "name": "春节调休上班"}
        ]
    })");
    file.close();

    WorkdayData data;
    const auto holiday = data.entryForDate(QDate(2026, 2, 17));
    QVERIFY(holiday);
    QVERIFY(holiday->kind == WorkdayEntry::Kind::Holiday);
    QCOMPARE(holiday->name, QStringLiteral("春节假期"));

    const auto workday = data.entryForDate(QDate(2026, 2, 28));
    QVERIFY(workday);
    QVERIFY(workday->kind == WorkdayEntry::Kind::Workday);

    QVERIFY(!data.entryForDate(QDate(2026, 2, 27)));
}

void LunarConverterTest::loadsEventColorsWithOverride()
{
    QStandardPaths::setTestModeEnabled(true);
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/lunarcalendar/eventcolors.json");
    QVERIFY(QDir().mkpath(QFileInfo(path).dir().path()));

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(R"({"festival": "#ff0000", "solarTerm": "不是颜色"})");
    file.close();

    EventColors colors;
    QCOMPARE(colors.colorFor(EventColors::Kind::Festival), QStringLiteral("#ff0000"));
    QCOMPARE(colors.colorFor(EventColors::Kind::SolarTerm), QStringLiteral("#2e7d32"));
    QCOMPARE(colors.colorFor(EventColors::Kind::Workday), QStringLiteral("#1565c0"));
    QCOMPARE(colors.colorFor(EventColors::Kind::DayOff), QStringLiteral("#ef6c00"));
}

void LunarConverterTest::rejectsUnsupportedDates()
{
    QVERIFY(!LunarConverter::fromGregorian(QDate(1900, 1, 30)));
}

QTEST_MAIN(LunarConverterTest)

#include "lunarconvertertest.moc"

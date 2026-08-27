/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "suishisettings.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

namespace {

QString settingsFilePath()
{
    QString configLocation = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (configLocation.isEmpty()) {
        configLocation = QDir::homePath() + QStringLiteral("/.config");
    }
    return QDir(configLocation).filePath(QStringLiteral("suishi/settings.ini"));
}

void ensureSettingsDirectory(const QString &filePath)
{
    QDir().mkpath(QFileInfo(filePath).absolutePath());
}

} // namespace

bool SuishiSettings::showEnglishDate()
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Appearance"));
    const bool enabled = settings.value(QStringLiteral("showEnglishDate"), false).toBool();
    settings.endGroup();
    return enabled;
}

void SuishiSettings::setShowEnglishDate(bool enabled)
{
    const QString filePath = settingsFilePath();
    ensureSettingsDirectory(filePath);

    QSettings settings(filePath, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Appearance"));
    settings.setValue(QStringLiteral("showEnglishDate"), enabled);
    settings.endGroup();
    settings.sync();
}

/*
    SPDX-FileCopyrightText: 2026
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "customevents.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>

namespace {

constexpr int CurrentSchemaVersion = 3;
constexpr int LegacySchemaVersion = 1;
constexpr int RecurrenceSchemaVersion = 2;
constexpr int DescriptionSchemaVersion = 3;
const QString RelativePath = QStringLiteral("lunarcalendar/events.json");

bool isValidColor(const QString &value)
{
    static const QRegularExpression pattern(QStringLiteral("^#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?$"));
    return pattern.match(value).hasMatch();
}

void setError(QString *error, const QString &message)
{
    if (error) {
        *error = message;
    }
}

bool isIntervalMatch(qint64 value, qint64 interval)
{
    return value >= 0 && interval > 0 && value % interval == 0;
}

int monthsBetween(const QDate &start, const QDate &date)
{
    return (date.year() - start.year()) * 12 + date.month() - start.month();
}

bool isOccurrence(const CustomEvent &event, const QDate &date)
{
    if (!date.isValid() || !event.date.isValid() || date < event.date) {
        return false;
    }

    if (event.repeatType == CustomEvent::RepeatType::None) {
        return event.date == date;
    }
    if (event.repeatInterval < 1) {
        return false;
    }

    CustomEvent::RepeatUnit unit = CustomEvent::RepeatUnit::Day;
    switch (event.repeatType) {
    case CustomEvent::RepeatType::Daily:
        unit = CustomEvent::RepeatUnit::Day;
        break;
    case CustomEvent::RepeatType::Weekly:
        unit = CustomEvent::RepeatUnit::Week;
        break;
    case CustomEvent::RepeatType::Monthly:
        unit = CustomEvent::RepeatUnit::Month;
        break;
    case CustomEvent::RepeatType::Yearly:
        unit = CustomEvent::RepeatUnit::Year;
        break;
    case CustomEvent::RepeatType::Custom:
        unit = event.repeatUnit;
        break;
    case CustomEvent::RepeatType::None:
        return false;
    }

    switch (unit) {
    case CustomEvent::RepeatUnit::Day:
        return isIntervalMatch(event.date.daysTo(date), event.repeatInterval);
    case CustomEvent::RepeatUnit::Week:
        return isIntervalMatch(event.date.daysTo(date), 7 * static_cast<qint64>(event.repeatInterval));
    case CustomEvent::RepeatUnit::Month: {
        const int monthDelta = monthsBetween(event.date, date);
        return date.day() == event.date.day() && isIntervalMatch(monthDelta, event.repeatInterval);
    }
    case CustomEvent::RepeatUnit::Year: {
        const int yearDelta = date.year() - event.date.year();
        return date.month() == event.date.month() && date.day() == event.date.day()
            && isIntervalMatch(yearDelta, event.repeatInterval);
    }
    }

    return false;
}

} // namespace

CustomEvents::CustomEvents() = default;

CustomEvents::CustomEvents(const QString &userFilePath, const QString &systemFilePath)
    : m_userFilePathOverride(userFilePath)
    , m_systemFilePathOverride(systemFilePath)
{
}

const QList<CustomEvent> &CustomEvents::events() const
{
    return m_events;
}

QList<CustomEvent> CustomEvents::eventsForDate(const QDate &date) const
{
    QList<CustomEvent> result;
    if (!date.isValid()) {
        return result;
    }

    for (const CustomEvent &event : m_events) {
        // Invalid dates such as February 29 simply have no occurrence in
        // non-leap years because QDate cannot represent them.
        if (isOccurrence(event, date)) {
            result.append(event);
        }
    }
    return result;
}

bool CustomEvents::reloadIfChanged(QString *error)
{
    const QString path = effectiveFilePath();
    const FileStamp currentStamp = fileStamp(path);

    if (m_hasStamp && sameStamp(m_stamp, currentStamp)) {
        setError(error, m_lastError);
        return m_lastError.isEmpty();
    }

    m_hasStamp = true;
    m_stamp = currentStamp;
    m_sourcePath = path;
    if (path.isEmpty()) {
        m_source = Source::None;
    } else {
        const QString configuredUserPath = userFilePath();
        const QString userPath = configuredUserPath.isEmpty()
            ? QString()
            : QDir::cleanPath(QFileInfo(configuredUserPath).absoluteFilePath());
        const QString sourcePath = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
        m_source = sourcePath == userPath ? Source::User : Source::System;
    }

    if (!currentStamp.exists) {
        m_events.clear();
        m_lastError.clear();
        setError(error, QString());
        return true;
    }

    QList<CustomEvent> loadedEvents;
    QString loadError;
    if (!loadFile(path, &loadedEvents, &loadError)) {
        // Keep the last valid data available to the calendar while exposing the
        // parse error to the configuration page.
        m_lastError = sourceError(path, loadError);
        setError(error, m_lastError);
        return false;
    }

    m_events = loadedEvents;
    m_lastError.clear();
    setError(error, QString());
    return true;
}

bool CustomEvents::saveUserEvents(const QList<CustomEvent> &eventsToSave, QString *error)
{
    QString validationError;
    if (!validateEvents(eventsToSave, &validationError)) {
        setError(error, validationError);
        return false;
    }

    if (!m_lastError.isEmpty()) {
        setError(error, m_lastError);
        return false;
    }

    const QString path = userFilePath();
    if (path.isEmpty()) {
        const QString message = QStringLiteral("无法确定用户数据目录");
        setError(error, message);
        return false;
    }

    QString writeError;
    if (!writeFile(path, eventsToSave, &writeError)) {
        setError(error, writeError);
        return false;
    }

    m_hasStamp = false;
    return reloadIfChanged(error);
}

bool CustomEvents::resetToSystemDefaults(QString *error)
{
    const QString path = userFilePath();
    if (path.isEmpty()) {
        const QString message = QStringLiteral("无法确定用户数据目录");
        setError(error, message);
        return false;
    }

    if (QFileInfo::exists(path) && !QFile::remove(path)) {
        const QString message = QStringLiteral("无法删除用户事件文件：%1").arg(path);
        setError(error, message);
        return false;
    }

    m_hasStamp = false;
    return reloadIfChanged(error);
}

CustomEvents::Source CustomEvents::source() const
{
    return m_source;
}

QString CustomEvents::sourcePath() const
{
    return m_sourcePath;
}

QString CustomEvents::errorString() const
{
    return m_lastError;
}

QString CustomEvents::repeatTypeToString(CustomEvent::RepeatType type)
{
    switch (type) {
    case CustomEvent::RepeatType::None:
        return QStringLiteral("none");
    case CustomEvent::RepeatType::Daily:
        return QStringLiteral("daily");
    case CustomEvent::RepeatType::Weekly:
        return QStringLiteral("weekly");
    case CustomEvent::RepeatType::Monthly:
        return QStringLiteral("monthly");
    case CustomEvent::RepeatType::Yearly:
        return QStringLiteral("yearly");
    case CustomEvent::RepeatType::Custom:
        return QStringLiteral("custom");
    }

    return QString();
}

bool CustomEvents::repeatTypeFromString(const QString &value, CustomEvent::RepeatType *type)
{
    if (!type) {
        return false;
    }

    const QString normalized = value.trimmed();
    if (normalized == QStringLiteral("none")) {
        *type = CustomEvent::RepeatType::None;
    } else if (normalized == QStringLiteral("daily")) {
        *type = CustomEvent::RepeatType::Daily;
    } else if (normalized == QStringLiteral("weekly")) {
        *type = CustomEvent::RepeatType::Weekly;
    } else if (normalized == QStringLiteral("monthly")) {
        *type = CustomEvent::RepeatType::Monthly;
    } else if (normalized == QStringLiteral("yearly")) {
        *type = CustomEvent::RepeatType::Yearly;
    } else if (normalized == QStringLiteral("custom")) {
        *type = CustomEvent::RepeatType::Custom;
    } else {
        return false;
    }
    return true;
}

QString CustomEvents::repeatUnitToString(CustomEvent::RepeatUnit unit)
{
    switch (unit) {
    case CustomEvent::RepeatUnit::Day:
        return QStringLiteral("day");
    case CustomEvent::RepeatUnit::Week:
        return QStringLiteral("week");
    case CustomEvent::RepeatUnit::Month:
        return QStringLiteral("month");
    case CustomEvent::RepeatUnit::Year:
        return QStringLiteral("year");
    }

    return QString();
}

bool CustomEvents::repeatUnitFromString(const QString &value, CustomEvent::RepeatUnit *unit)
{
    if (!unit) {
        return false;
    }

    const QString normalized = value.trimmed();
    if (normalized == QStringLiteral("day")) {
        *unit = CustomEvent::RepeatUnit::Day;
    } else if (normalized == QStringLiteral("week")) {
        *unit = CustomEvent::RepeatUnit::Week;
    } else if (normalized == QStringLiteral("month")) {
        *unit = CustomEvent::RepeatUnit::Month;
    } else if (normalized == QStringLiteral("year")) {
        *unit = CustomEvent::RepeatUnit::Year;
    } else {
        return false;
    }
    return true;
}

bool CustomEvents::validateEvent(const CustomEvent &event, QString *error)
{
    if (event.id.trimmed().isEmpty()) {
        setError(error, QStringLiteral("事件 ID 不能为空"));
        return false;
    }
    if (event.name.trimmed().isEmpty()) {
        setError(error, QStringLiteral("事件名称不能为空"));
        return false;
    }
    if (!event.date.isValid()) {
        setError(error, QStringLiteral("事件日期无效"));
        return false;
    }
    if (!isValidColor(event.color)) {
        setError(error, QStringLiteral("事件颜色无效，应为 #RRGGBB 或 #AARRGGBB"));
        return false;
    }

    if (event.repeatInterval < 1) {
        setError(error, QStringLiteral("重复间隔必须是正整数"));
        return false;
    }

    switch (event.repeatType) {
    case CustomEvent::RepeatType::None:
        if (event.repeatInterval != 1 || event.repeatUnit != CustomEvent::RepeatUnit::Day) {
            setError(error, QStringLiteral("不重复事件的重复规则无效"));
            return false;
        }
        break;
    case CustomEvent::RepeatType::Daily:
        if (event.repeatInterval != 1 || event.repeatUnit != CustomEvent::RepeatUnit::Day) {
            setError(error, QStringLiteral("每天重复的单位无效"));
            return false;
        }
        break;
    case CustomEvent::RepeatType::Weekly:
        if (event.repeatInterval != 1 || event.repeatUnit != CustomEvent::RepeatUnit::Week) {
            setError(error, QStringLiteral("每周重复的单位无效"));
            return false;
        }
        break;
    case CustomEvent::RepeatType::Monthly:
        if (event.repeatInterval != 1 || event.repeatUnit != CustomEvent::RepeatUnit::Month) {
            setError(error, QStringLiteral("每月重复的单位无效"));
            return false;
        }
        break;
    case CustomEvent::RepeatType::Yearly:
        if (event.repeatInterval != 1 || event.repeatUnit != CustomEvent::RepeatUnit::Year) {
            setError(error, QStringLiteral("每年重复的单位无效"));
            return false;
        }
        break;
    case CustomEvent::RepeatType::Custom:
        switch (event.repeatUnit) {
        case CustomEvent::RepeatUnit::Day:
        case CustomEvent::RepeatUnit::Week:
        case CustomEvent::RepeatUnit::Month:
        case CustomEvent::RepeatUnit::Year:
            break;
        default:
            setError(error, QStringLiteral("自定义重复单位无效"));
            return false;
        }
        break;
    default:
        setError(error, QStringLiteral("重复类型无效"));
        return false;
    }

    return true;
}

bool CustomEvents::validateEvents(const QList<CustomEvent> &eventsToValidate, QString *error)
{
    QSet<QString> ids;
    for (const CustomEvent &event : eventsToValidate) {
        QString eventError;
        if (!validateEvent(event, &eventError)) {
            setError(error, eventError);
            return false;
        }
        if (ids.contains(event.id)) {
            setError(error, QStringLiteral("事件 ID 重复：%1").arg(event.id));
            return false;
        }
        ids.insert(event.id);
    }
    setError(error, QString());
    return true;
}

QString CustomEvents::userFilePath() const
{
    if (!m_userFilePathOverride.isEmpty()) {
        return QDir::cleanPath(m_userFilePathOverride);
    }

    const QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (dataLocation.isEmpty()) {
        return QString();
    }
    return QDir(dataLocation).filePath(RelativePath);
}

QString CustomEvents::effectiveFilePath() const
{
    if (!m_userFilePathOverride.isEmpty() || !m_systemFilePathOverride.isEmpty()) {
        const QString userPath = userFilePath();
        if (QFileInfo(userPath).isFile()) {
            return userPath;
        }
        if (QFileInfo(m_systemFilePathOverride).isFile()) {
            return QDir::cleanPath(m_systemFilePathOverride);
        }
        return QString();
    }

    return QStandardPaths::locate(QStandardPaths::GenericDataLocation, RelativePath);
}

CustomEvents::FileStamp CustomEvents::fileStamp(const QString &path)
{
    FileStamp stamp;
    if (path.isEmpty()) {
        return stamp;
    }

    const QFileInfo info(path);
    stamp.path = QDir::cleanPath(info.absoluteFilePath());
    stamp.exists = info.isFile();
    if (stamp.exists) {
        stamp.size = info.size();
        stamp.modifiedMsecs = info.lastModified().toMSecsSinceEpoch();
    }
    return stamp;
}

bool CustomEvents::sameStamp(const FileStamp &left, const FileStamp &right)
{
    return left.path == right.path && left.exists == right.exists && left.size == right.size && left.modifiedMsecs == right.modifiedMsecs;
}

bool CustomEvents::loadFile(const QString &path, QList<CustomEvent> *events, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(error, QStringLiteral("无法读取文件：%1").arg(file.errorString()));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(error, QStringLiteral("JSON 格式无效：%1").arg(parseError.errorString()));
        return false;
    }

    const QJsonObject root = document.object();
    const QJsonValue schemaVersionValue = root.value(QStringLiteral("schemaVersion"));
    const double schemaVersionNumber = schemaVersionValue.toDouble();
    if (!schemaVersionValue.isDouble() || !std::isfinite(schemaVersionNumber)
        || (schemaVersionNumber != LegacySchemaVersion
            && schemaVersionNumber != RecurrenceSchemaVersion
            && schemaVersionNumber != CurrentSchemaVersion)) {
        setError(error, QStringLiteral("不支持的事件文件版本"));
        return false;
    }
    const int schemaVersion = static_cast<int>(schemaVersionNumber);

    const QJsonValue eventsValue = root.value(QStringLiteral("events"));
    if (!eventsValue.isArray()) {
        setError(error, QStringLiteral("events 必须是数组"));
        return false;
    }

    QList<CustomEvent> loadedEvents;
    const QJsonArray array = eventsValue.toArray();
    for (qsizetype index = 0; index < array.size(); ++index) {
        const QJsonValue value = array.at(index);
        if (!value.isObject()) {
            setError(error, QStringLiteral("第 %1 个事件不是对象").arg(index + 1));
            return false;
        }

        const QJsonObject item = value.toObject();
        const QJsonValue idValue = item.value(QStringLiteral("id"));
        const QJsonValue dateValue = item.value(QStringLiteral("date"));
        const QJsonValue nameValue = item.value(QStringLiteral("name"));
        const QJsonValue colorValue = item.value(QStringLiteral("color"));
        if (!idValue.isString() || !dateValue.isString() || !nameValue.isString() || !colorValue.isString()) {
            setError(error, QStringLiteral("第 %1 个事件字段类型无效").arg(index + 1));
            return false;
        }

        const QString dateText = dateValue.toString();
        CustomEvent event;
        event.id = idValue.toString().trimmed();
        event.date = QDate::fromString(dateText, Qt::ISODate);
        event.name = nameValue.toString().trimmed();
        event.color = colorValue.toString().trimmed();

        if (schemaVersion >= DescriptionSchemaVersion) {
            const QJsonValue descriptionValue = item.value(QStringLiteral("description"));
            if (!descriptionValue.isUndefined() && !descriptionValue.isString()) {
                setError(error, QStringLiteral("第 %1 个事件详情字段类型无效").arg(index + 1));
                return false;
            }
            event.description = descriptionValue.isString() ? descriptionValue.toString().trimmed() : QString();
        }

        if (schemaVersion == LegacySchemaVersion) {
            const QJsonValue repeatValue = item.value(QStringLiteral("repeatYearly"));
            if (!repeatValue.isBool()) {
                setError(error, QStringLiteral("第 %1 个事件字段类型无效").arg(index + 1));
                return false;
            }
            event.repeatType = repeatValue.toBool() ? CustomEvent::RepeatType::Yearly : CustomEvent::RepeatType::None;
            event.repeatInterval = 1;
            event.repeatUnit = repeatValue.toBool() ? CustomEvent::RepeatUnit::Year : CustomEvent::RepeatUnit::Day;
        } else {
            const QJsonValue recurrenceValue = item.value(QStringLiteral("recurrence"));
            if (!recurrenceValue.isObject()) {
                setError(error, QStringLiteral("第 %1 个事件的 recurrence 必须是对象").arg(index + 1));
                return false;
            }

            const QJsonObject recurrence = recurrenceValue.toObject();
            const QJsonValue typeValue = recurrence.value(QStringLiteral("type"));
            const QJsonValue intervalValue = recurrence.value(QStringLiteral("interval"));
            const QJsonValue unitValue = recurrence.value(QStringLiteral("unit"));
            const double intervalNumber = intervalValue.toDouble();
            const bool validInterval = intervalValue.isDouble() && std::isfinite(intervalNumber)
                && intervalNumber >= 1.0
                && intervalNumber <= static_cast<double>(std::numeric_limits<int>::max())
                && std::floor(intervalNumber) == intervalNumber;
            if (!typeValue.isString() || !validInterval || !unitValue.isString()) {
                setError(error, QStringLiteral("第 %1 个事件的 recurrence 字段类型无效").arg(index + 1));
                return false;
            }

            if (!repeatTypeFromString(typeValue.toString(), &event.repeatType)
                || !repeatUnitFromString(unitValue.toString(), &event.repeatUnit)) {
                setError(error, QStringLiteral("第 %1 个事件的 recurrence 值无效").arg(index + 1));
                return false;
            }
            event.repeatInterval = static_cast<int>(intervalNumber);
        }

        if (!event.date.isValid() || event.date.toString(Qt::ISODate) != dateText) {
            setError(error, QStringLiteral("第 %1 个事件日期无效").arg(index + 1));
            return false;
        }

        QString eventError;
        if (!validateEvent(event, &eventError)) {
            setError(error, QStringLiteral("第 %1 个事件：%2").arg(index + 1).arg(eventError));
            return false;
        }
        if (std::any_of(loadedEvents.cbegin(), loadedEvents.cend(), [&event](const CustomEvent &loaded) {
                return loaded.id == event.id;
            })) {
            setError(error, QStringLiteral("事件 ID 重复：%1").arg(event.id));
            return false;
        }
        loadedEvents.append(event);
    }

    *events = loadedEvents;
    setError(error, QString());
    return true;
}

QByteArray CustomEvents::serializeEvents(const QList<CustomEvent> &eventsToSerialize)
{
    QJsonArray array;
    for (const CustomEvent &event : eventsToSerialize) {
        QJsonObject item;
        item.insert(QStringLiteral("id"), event.id);
        item.insert(QStringLiteral("date"), event.date.toString(Qt::ISODate));
        item.insert(QStringLiteral("name"), event.name);
        if (!event.description.isEmpty()) {
            item.insert(QStringLiteral("description"), event.description);
        }
        item.insert(QStringLiteral("color"), event.color);

        QJsonObject recurrence;
        recurrence.insert(QStringLiteral("type"), repeatTypeToString(event.repeatType));
        recurrence.insert(QStringLiteral("interval"), event.repeatInterval);
        recurrence.insert(QStringLiteral("unit"), repeatUnitToString(event.repeatUnit));
        item.insert(QStringLiteral("recurrence"), recurrence);
        array.append(item);
    }

    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), CurrentSchemaVersion);
    root.insert(QStringLiteral("events"), array);
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

bool CustomEvents::writeFile(const QString &path, const QList<CustomEvent> &eventsToWrite, QString *error)
{
    const QFileInfo fileInfo(path);
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        setError(error, QStringLiteral("无法创建用户数据目录：%1").arg(fileInfo.absolutePath()));
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(error, QStringLiteral("无法写入事件文件：%1").arg(file.errorString()));
        return false;
    }

    const QByteArray content = serializeEvents(eventsToWrite);
    if (file.write(content) != content.size()) {
        setError(error, QStringLiteral("写入事件文件失败：%1").arg(file.errorString()));
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        setError(error, QStringLiteral("提交事件文件失败：%1").arg(file.errorString()));
        return false;
    }

    setError(error, QString());
    return true;
}

QString CustomEvents::sourceError(const QString &path, const QString &details)
{
    return QStringLiteral("无法加载事件文件 %1：%2").arg(path, details);
}

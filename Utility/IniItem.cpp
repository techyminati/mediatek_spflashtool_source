#include "IniItem.h"

#include <QSettings>

#include "FileUtils.h"

IniItem::IniItem(const QString &file_path, const QString &section_name, const QString &item_name):
    file_path_(ABS_PATH_C(file_path.toLocal8Bit().constData())),
    section_name_(section_name),
    item_name_(item_name)
{
}

void IniItem::SaveIntValue(const int ivalue)
{
    Save(QVariant(ivalue));
}

void IniItem::SaveStringValue(const QString &svalue)
{
    Save(QVariant(svalue));
}

void IniItem::SaveBooleanValue(const bool bvalue)
{
    Save(QVariant(bvalue));
}

void IniItem::SaveStringListValue(const QStringList &values)
{
    Save(QVariant(values));
}

void IniItem::Save(QVariant value)
{
    QSettings settings(file_path_, QSettings::IniFormat);

    settings.beginGroup(section_name_);

    settings.setValue(item_name_, value);

    settings.endGroup();

    settings.sync();
}

int IniItem::GetIntValue()
{
    bool ok;
    int value;

    value = Get().toInt(&ok);

    if(ok)
        return value;

    return -1;
}

QString IniItem::GetStringValue()
{
    return Get().toString();
}

bool IniItem::GetBooleanValue()
{
    return Get().toBool();
}

QStringList IniItem::GetStringListValue()
{
    return Get().toStringList();
}

QVariant IniItem::Get()
{
    QSettings settings(file_path_, QSettings::IniFormat);

    settings.beginGroup(section_name_);

    QVariant value = settings.value(item_name_);

    settings.endGroup();

    return value;
}

bool IniItem::hasKey(const QString &sKey) const
{
    QSettings settings(file_path_, QSettings::IniFormat);
    settings.beginGroup(section_name_);
    QString item_name = sKey.trimmed().isEmpty() ? item_name_ : sKey.trimmed();
    bool b_has_key = settings.contains(item_name);
    settings.endGroup();
    return b_has_key;
}

bool IniItem::hasSection(const QString &sSectionName) const
{
    QSettings settings(file_path_, QSettings::IniFormat);
    QString section_name = sSectionName.trimmed().isEmpty() ? section_name_ : sSectionName.trimmed();
    QStringList allSections = settings.childGroups();
    return allSections.contains(section_name);
}


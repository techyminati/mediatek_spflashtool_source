#include "LogFilesClean.h"
#include <QDir>
#include <QStringList>
#include <QList>
#include "FileUtils.h"
#include "../Logger/Log.h"

LogCleanThread::LogCleanThread(const QString &logPath, qint64 nHours, QObject *parent):
    QThread(parent),
    m_logPath(logPath),
    m_hours(nHours)
{

}

LogCleanThread::~LogCleanThread()
{
    this->terminate();
    this->wait();
}

void LogCleanThread::run()
{
    QDir dir(m_logPath);
    QStringList nameFilters("SP_FT_Dump_*");
    dir.setNameFilters(nameFilters);
    dir.setSorting(QDir::Reversed | QDir::Name);
    QStringList list = dir.entryList();

    QList<QString>::iterator it = list.begin();
    QDateTime referenceDT = this->getReferenceDT();
    LOGD(referenceDT.toString("MM-dd-yyyy-HH-mm-ss").toStdString().c_str());

    for(; it != list.end(); ++it)
    {
        QString sdatetime = (*it).mid (11);//remove "SP_FT_Dump_"
        QDateTime datetime = QDateTime::fromString(sdatetime, "MM-dd-yyyy-HH-mm-ss");
        LOGD(datetime.toString("MM-dd-yyyy-HH-mm-ss").toStdString().c_str());
        if(referenceDT >= datetime)
        {
            QString dir_path = m_logPath;
            dir_path = dir_path.append("/").append(*it);
            dir_path = QDir::toNativeSeparators(dir_path);

            if(FileUtils::QDeleteDirectory(dir_path.toStdString()) == false)
            {
                LOGD("remove file %s failed.", (*it).toLocal8Bit().constData());
            }
        }
    }
}

QDateTime LogCleanThread::getReferenceDT() const
{
    qint64 nSeconds = m_hours * 60 * 60;
    return QDateTime::currentDateTime().addSecs(nSeconds);
}

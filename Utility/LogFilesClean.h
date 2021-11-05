#ifndef LOGFILESCLEAN_H
#define LOGFILESCLEAN_H
#include <QThread>
#include <QString>
#include <QDateTime>

class LogCleanThread: public QThread
{
public:
    LogCleanThread(const QString &logPath, qint64 nHours, QObject *parent = NULL);
    ~LogCleanThread();

protected:
    virtual void run();

private:
    QDateTime getReferenceDT() const;

private:
    QString m_logPath;
    qint64 m_hours;
};

#endif //LOGFILESCLEAN_H

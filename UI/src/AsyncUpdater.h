#ifndef ASYNCUPDATER_H
#define ASYNCUPDATER_H

#include <QObject>
#include <QString>
#include <QtCore/QThread>
#include <vector>
#include <QProcess>
#include <QFuture>
#include "JSONParser/json.h"

class AsyncUpdater : public QObject
{
    Q_OBJECT

public:
    typedef enum _UpdateFailReason
    {
        unknown_reason = 0,
        lack_of_update_files,
        start_update_exe_fail
    } UpdateFailReason;

public:
    explicit AsyncUpdater(QObject *parent);
    ~AsyncUpdater();

    static bool IsUpdatePackage(const std::string &file_name, const std::string &pattern);
    bool hasNewVersion() {return hasNewVersion_;}

    void CheckAsyncNewVersion();
    void CheckUpdate();
    void CheckFinish();

    void AsyncUpdateTool();
    void UpdateTool();

    const QString &GetReleaseNotes() const { return releaseNotes;}
    const std::string &GetVersionInfo() const { return newVersion;}

    void waitForFinished();

private:
    void GetAllVersions(std::vector<std::string> &versions);
    void processError(QProcess::ProcessError err);
    void ReadReleaseNotes(const std::string &path);
    void ConstructReleaseNotes(QtJson::JsonObject &jsonObject);
    bool UpdateToolsPreCheck() const;
    void DoUpdateTool();
    bool RunAsAdmin(const QString &appName, const QStringList &paramterList) const;

private:
    std::string ReleaseFolder;
    std::string UpdaterExe;
    std::string ReleaseNote;
    std::string CurrentVersion;

    void parserUpdateInfo();

    bool hasNewVersion_;
    std::string newVersion;
    std::string newVersionFolder;
    std::string newVersionPackage;
    QString releaseNotes;

    QFuture<void> future;

signals:
    void signal_check_finish(const bool &bUpdate);
    void signal_update_finish();
    void signal_update_failed(AsyncUpdater::UpdateFailReason);

public slots:

};

#endif // ASYNCUPDATER_H

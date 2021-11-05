#ifdef UNICODE
#undef UNICODE
// using ShellExecuteExA to startup and forward paramters for SP_Updater.exe program written by python.
#include <windows.h>
#define UNICODE
#endif

#include "AsyncUpdater.h"
#include <QRunnable>
#include <QThreadPool>
#include <QDir>
#include <QStringList>
#include <QApplication>
#include <QTextStream>
#include <QSettings>
#include <algorithm>
#include <QDebug>
#include <QFuture>
#include <QtConcurrentRun>

#include "../../BootRom/host.h"
#include "../../Utility/version.h"
#include "../../Utility/FileUtils.h"
#include "../../Utility/constString.h"
#include "../../Host/Inc/RuntimeMemory.h"
#include "../../Utility/Utils.h"
#include "../../Logger/Log.h"

#ifdef _WIN32
#define STR_RELEASE_FOLDER  "Windows"
#else
#define STR_RELEASE_FOLDER "Linux"
#endif

#define Tool_Version_key            "Version"
#define New_Feature_key             "New-Features/Enhancements"
#define Bug_Fixed_Key               "Bug-Fixed"
#define Limitation_Key              "Limitations"
#define Limitation_Bringup_SubKey   "Bringup"
#define Limitation_Update_SubKey    "Update"
#define Support_Platform_Key        "Support-Platforms"

const static std::string g_update_using_tools[] = {"ReplaceBin.exe", "update.ini"};

class CheckUpdateRunnable : public QRunnable
{
public:
    CheckUpdateRunnable(AsyncUpdater *updater):updater_(updater)
    {

    }

    ~CheckUpdateRunnable()
    {

    }

    virtual void run();

private:
    AsyncUpdater *updater_;
};

void CheckUpdateRunnable::run()
{
    updater_->CheckUpdate();
    updater_->CheckFinish();
}

class UpdateRunnable:public QRunnable
{
public:
    UpdateRunnable(AsyncUpdater *updater):updater_(updater)
    {

    }

    ~UpdateRunnable()
    {

    }

    virtual void run();

    AsyncUpdater *updater_;
};

void UpdateRunnable::run()
{
    updater_->UpdateTool();
}

void AsyncUpdater::parserUpdateInfo()
{
    QSettings settings(ABS_PATH_C("update.ini"), QSettings::IniFormat);

    settings.beginGroup("Exe Info");

    ReleaseFolder = settings.value("path").toString().toStdString();
    UpdaterExe = settings.value("name").toString().toStdString();
    ReleaseNote = settings.value("releaseNote").toString().toStdString();

    settings.endGroup();

    settings.beginGroup("BRING UP");
    CurrentVersion = settings.value("currentVersion", "").toString().toStdString();
    settings.endGroup();
}

AsyncUpdater::AsyncUpdater(QObject *parent) :
    QObject(parent)
{
    hasNewVersion_ = false;

    parserUpdateInfo();
}

AsyncUpdater::~AsyncUpdater()
{

}

void AsyncUpdater::AsyncUpdateTool()
{
    QRunnable *task = new UpdateRunnable(this);
    QThreadPool::globalInstance()->start(task);
}

void AsyncUpdater::UpdateTool()
{
    if (UpdateToolsPreCheck())
    {
        DoUpdateTool();
    }
    else {
        emit signal_update_failed(lack_of_update_files);
    }
}

void AsyncUpdater::CheckAsyncNewVersion()
{
    QRunnable *task = new CheckUpdateRunnable(this);

    //QThreadPool::globalInstance()->start(task);
    future = QtConcurrent::run(task, &QRunnable::run);
}

void AsyncUpdater::waitForFinished()
{
    future.waitForFinished();
}

void AsyncUpdater::CheckUpdate()
{
    std::vector<std::string> allVersions;

    GetAllVersions(allVersions);

    if(allVersions.empty())
        return;

    std::sort(allVersions.begin(), allVersions.end());

    std::string latestVer = allVersions.back();

    bool bNeedUpdate = CurrentVersion.empty() ? latestVer.compare(ToolInfo::VersionNum()) > 0 : latestVer.compare(CurrentVersion) > 0;
    if(bNeedUpdate)
    {
        std::list<std::string> files;

        FileUtils::FindFile(ReleaseFolder + C_SEP_STR + latestVer + C_SEP_STR + STR_RELEASE_FOLDER,
                            files, "", IsUpdatePackage);

        if(!files.empty())
        {
            hasNewVersion_ = true;
            newVersion = latestVer;
            newVersionFolder = ReleaseFolder + C_SEP_STR + latestVer + C_SEP_STR + STR_RELEASE_FOLDER;
            newVersionPackage = newVersionFolder + C_SEP_STR + files.front();

            ReadReleaseNotes(newVersionFolder + C_SEP_STR + ReleaseNote);
        }
    }
}

void AsyncUpdater::ConstructReleaseNotes(QtJson::JsonObject &jsonObject){
    QString tool_version = jsonObject[Tool_Version_key].toString();
    if(!tool_version.isEmpty()){
        releaseNotes = "Tool version: \n    "  + tool_version + "\n\n";
    }

    QtJson::JsonArray new_featureList = jsonObject[New_Feature_key].toList();

    if (new_featureList.count() > 0){
        releaseNotes += New_Feature_key;
        releaseNotes += "\n";

        foreach(QVariant feature, new_featureList){
            releaseNotes += "   " + feature.toString() + "\n\n";
        }
    }

    QtJson::JsonArray bug_fixedList = jsonObject[Bug_Fixed_Key].toList();

    if(bug_fixedList.count() > 0){
        releaseNotes += Bug_Fixed_Key;
        releaseNotes += "\n";

        foreach(QVariant bug, bug_fixedList){
            releaseNotes += "   " + bug.toString() + "\n\n";
        }
    }
}

bool AsyncUpdater::UpdateToolsPreCheck() const
{
    bool update_pre_check_state = true;
    for (unsigned int i = 0; i < sizeof(g_update_using_tools)/sizeof(g_update_using_tools[0]); ++i)
    {
        std::string tool_path = ABS_PATH(g_update_using_tools[i]);
        if (!FileUtils::IsFileExist(tool_path))
        {
            update_pre_check_state = false;
            break;
        }
    }
    // check Updater.exe/SP_Updater.exe/SP_Updater_backup.exe using update.ini file
    if (update_pre_check_state) {
        std::string updater_tool = ABS_PATH(UpdaterExe + ".exe");
        update_pre_check_state = FileUtils::IsFileExist(updater_tool);
    }
    return update_pre_check_state;
}

void AsyncUpdater::DoUpdateTool()
{
    std::string updater = ABS_PATH(UpdaterExe);

    QString exePath = QString(updater.c_str());

    QStringList parameter;

    parameter << newVersionPackage.c_str();

    bool bSuccess = false;
    // QSysInfo::WV_6_1 means WV_WINDOWS7, Operating system version 6.1, corresponds to Windows 7 and Windows Server 2008 R2.
    if (QSysInfo::windowsVersion() > QSysInfo::WV_6_1)
    {
        bSuccess = this->RunAsAdmin(exePath, parameter);
    }
    else
    {
        QProcess process;
        QObject::connect(&process, SIGNAL(error(QProcess::ProcessError)),
                         this, SLOT(processError(QProcess::ProcessError)));
        bSuccess = process.startDetached(exePath, parameter);
    }

    if (bSuccess)
    {
        QApplication::exit(0);
    }
    else
    {
        emit signal_update_failed(start_update_exe_fail);
    }
}

bool AsyncUpdater::RunAsAdmin(const QString &appName, const QStringList &paramterList) const
{
    QString paramters_arg;
    for (int i = 0; i < paramterList.size(); ++i)
    {
        paramters_arg += QString("\"%1\"").arg(paramterList[i]);
        if (i != paramterList.size() - 1) {
            paramters_arg += " ";
        }
    }

    paramters_arg = QDir::toNativeSeparators(paramters_arg);
    LOG("paramter argument: %s", paramters_arg.toStdString().c_str());

    SHELLEXECUTEINFO execinfo;
    memset(&execinfo, 0, sizeof(execinfo));
    execinfo.lpFile         = appName.toStdString().c_str();
    execinfo.cbSize         = sizeof(execinfo);
    execinfo.lpVerb         = TEXT("runas");
    execinfo.fMask          = SEE_MASK_NO_CONSOLE;
    execinfo.nShow          = SW_SHOWDEFAULT;
    execinfo.lpParameters   = paramters_arg.toStdString().c_str();

    BOOL bSuccess = ShellExecuteEx(&execinfo);
    if (!bSuccess) {
        LOG("ShellExecuteEx error code: %d", GetLastError());
    }
    return (bool)bSuccess;
}

void AsyncUpdater::ReadReleaseNotes(const std::string &path)
{
    QtJson::JsonObject jsonObject;

    Utils::parserJSONFile(path.c_str(), jsonObject);

    ConstructReleaseNotes(jsonObject);

    qDebug() <<"Release notes: \n" << releaseNotes;

    std::string curr_releaseFilePath = ABS_PATH_C("Release.json");
    QtJson::JsonObject currJsonObject;

    Utils::parserJSONFile(curr_releaseFilePath.c_str(),currJsonObject);

    QtJson::JsonObject limitations = currJsonObject[Limitation_Key].toMap();

    QString bringup_chip = limitations[Limitation_Bringup_SubKey].toString();

    QtJson::JsonArray platforms = jsonObject[Support_Platform_Key].toList();

    qDebug() << "Support platforms: \n";
    foreach(QVariant platform, platforms){
        qDebug() << " -" << platform.toString();
    }

    if(!bringup_chip.isEmpty() && !platforms.contains(bringup_chip))
        hasNewVersion_ = false;

    bool tempVersion_update = limitations[Limitation_Update_SubKey].toBool();
    if(!tempVersion_update)
        hasNewVersion_ = false;

 #if 0
    QFile file(QString(path.c_str()));

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);

    while (!in.atEnd())
    {
        QString line = in.readLine();
        releaseNotes += line + "\n";
    }
#endif
}

void AsyncUpdater::GetAllVersions(std::vector<std::string> &versions)
{
    std::string path(ReleaseFolder);

    QDir dir(QString(path.c_str()));
    if(!dir.exists())
        return;

    dir.setFilter(QDir::Dirs);

    QFileInfoList list = dir.entryInfoList();
    int i = 0;

    if(list.empty())
        return;

    do
    {
        QFileInfo info = list.at(i);

        versions.push_back(info.fileName().toStdString());

        i++;
    }while(i < list.size());
}

void AsyncUpdater::CheckFinish()
{
    emit signal_check_finish(hasNewVersion_);
}

bool AsyncUpdater::IsUpdatePackage(const std::string &file_name,
                                   const std::string &/*pattern*/)
{
    QSettings settings(ABS_PATH_C("update.ini"), QSettings::IniFormat);

    settings.beginGroup("Exe Info");

    QString fileName = settings.value("ToolName").toString();

    settings.endGroup();

    return file_name.find(fileName.toStdString()) != std::string::npos
            && file_name.find(".zip") != std::string::npos;
}

void AsyncUpdater::processError(QProcess::ProcessError err)
{
    switch(err)
    {
    case QProcess::FailedToStart:
        flashtool_message_box(0, 0, INFORMATION_MSGBOX, tr("Smart Phone Flash Tool"), tr("Failed to Start Updater.exe!"), tr("OK"));
        break;

    case QProcess::Crashed:
        flashtool_message_box(0, 0, INFORMATION_MSGBOX, tr("Smart Phone Flash Tool"), tr("Crashed!"), tr("OK"));
        break;

    case QProcess::Timedout:
        flashtool_message_box(0, 0, INFORMATION_MSGBOX, tr("Smart Phone Flash Tool"), tr("Time out!"), tr("OK"));
        break;

    default:
        flashtool_message_box(0, 0, INFORMATION_MSGBOX, tr("Smart Phone Flash Tool"), tr("Unknow Error!"), tr("OK"));
        break;
    }
}

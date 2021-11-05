#include "ConsoleModeEntry.h"

#include "CommandLineArguments.h"
#include "Config.h"
#include "SchemaValidator.h"
#include "CommandSettingValidator.h"
#include "../Setting/ConnSetting.h"
#include "../Setting/ChksumSetting.h"
#include "../Utility/FileUtils.h"
#include "../Utility/version.h"
#include "../UI/src/AsyncUpdater.h"
#include "../Utility/Utils.h"
#include "../Host/Inc/RuntimeMemory.h"

#include <QFile>
#include <QProcess>
#include <typeinfo>

namespace ConsoleMode
{
#ifdef _WIN32
class ConsoleSentry
{
public:
    ConsoleSentry(bool do_redirect)
    {
        attached_ = AttachConsole(ATTACH_PARENT_PROCESS);

        if (attached_ && (!do_redirect))
        {
            freopen("conin$", "r+t", stdin);
            freopen("conout$", "w+t", stdout);
            freopen("conout$", "w+t", stderr);
            /*
            BOOL WINAPI SetConsoleCtrlHandler(
              _In_opt_  PHANDLER_ROUTINE HandlerRoutine,
              _In_      BOOL Add
            );
            */
        }

    }

    ~ConsoleSentry()
    {
        if (attached_)
        {
            FreeConsole();
        }
    }

private:
    BOOL attached_;
};
#endif

ConsoleModeEntry::ConsoleModeEntry(): m_cleanThread(NULL)
{
}

ConsoleModeEntry::~ConsoleModeEntry()
{
    if (NULL != m_cleanThread)
    {
        delete m_cleanThread;
        m_cleanThread = NULL;
    }
}

void ConsoleModeEntry::CheckNewVersion()
{
    QStringList parameter;

    std::string version = ToolInfo::VersionNum();

    parameter << "-i"
              << "update.ini"
              << "-v"
              << version.c_str();

    std::string exePath = "\"" + FileUtils::GetAppDirectory()
            + "/FlashToolUpdater.exe" + "\"";

    LOGI("Update exe path: %s", exePath.c_str());

    QProcess::startDetached(exePath.c_str(), parameter);

    exit(0);
}

void ConsoleModeEntry::CleanLogFiles(const std::string &log_path, qint64 clean_hours)
{
    qint64 earlier_hours = -clean_hours; //the earlier time means the time before current time
    m_cleanThread = new LogCleanThread(QString::fromStdString(log_path), earlier_hours);
    m_cleanThread->start(QThread::LowPriority);
}

int ConsoleModeEntry::Run(int argc, char *argv[])
{
    try{
        CommandLineArguments cmdArg;
        bool result = cmdArg.Parse(argc, argv);

    #ifdef _WIN32
        ConsoleSentry consoleSentry(cmdArg.DoRedirect());
    #endif

        QSharedPointer<AppCore> app = QSharedPointer<AppCore>(new AppCore);
        if (cmdArg.GetInputFilename().empty())
        {
            app->EnableLog(AppCore::EnableLogging);
        }
        else
        {
            if (!cmdArg.ValidateInputFile())
            {
                return -1;
            }
            const QSharedPointer<APCore::LogInfoSetting> logInfoSetting = Config::getLogInfoSetting(cmdArg.GetInputFilename());
            app->EnableLog(logInfoSetting->getLogOn() ? AppCore::EnableLogging : AppCore::DisableLogging);
            CleanLogFiles(logInfoSetting->getLogPath(), logInfoSetting->getCleanHours());
        }

        if(!result)
        {
            LOGI("Invalid parameter.\n");
            LOGI("%s\n", cmdArg.GetHelpInfo().c_str());
            return -1;
        }
        else if(cmdArg.DisplayHelpInfo())
        {
            LOGI("%s\n", cmdArg.GetHelpInfo().c_str());
            return 0;
        }

        bool validate = cmdArg.Validate();
        if(validate == false)
        {
            return -1;
        }

        LOGI("Begin");
        LOGI("%s", ToolInfo::ToolName().c_str());
        LOGI("Build Time: " __DATE__ " " __TIME__);

        APKey session_id = app->NewKey();
        Config config(cmdArg);
#ifdef _WIN32
        //custom version: check if lib DA changelist match
        if(ToolInfo::IsCustomerVer())
        {
            if(!FlashTool_IfLibDAMatch(config.eGetDAFile().c_str()))
            {
                LOGE("Error: lib DA NOT match!");
                return -1;
            }
        }
#endif

#if defined(_LINUX)
        FileUtils::copy_99ttyacms_file();
#endif

        //check validation of command setting
        CommandSettingValidator cmdSettingValidator(config, cmdArg);
        if (!cmdSettingValidator.Validate())
        {
            return -1;
        }
        if (!cmdSettingValidator.needRSCSetting())
        {
            config.pclGetCommandSetting()->removeRSCSetting();
        }
        QSharedPointer<APCore::ChksumSetting> chksum_setting = config.pclGetGeneralSetting()->pclGetChksumSetting();
        if(NULL != chksum_setting)
        {
            app->EnableDAChksum(session_id, chksum_setting->chksum_level());
        }
        if(!config.pclGetGeneralSetting()->pclCreateCommand(app, session_id)->exec())
            return -3;
        LOGI("General command exec done!");

        HW_StorageType_E storage = config.eGetStorageType();
        if (HW_STORAGE_NONE == storage)
        {
            LOGE("Get Storage type fail, Please load scatter file first!");
            return -2;
        }

        int stop_flag = 0;
        QSharedPointer<APCore::ConnSetting> conn_setting = config.pclGetGeneralSetting()->pclGetConnSetting();
        conn_setting->set_stop_flag(&stop_flag);
        QSharedPointer<APCore::Connection> conn(
                    conn_setting->CreateConnection(
                        session_id, storage, false/*come from nowhere@@*/));
        LOGI("Connection create done!");

        bool is_combo_fmt_ = config.fgIsCommboFmt(app,session_id);
        std::string da = config.eGetDAFile();
        bool is_scatter_ver2 = config.fgIsScatterVer2(app,session_id);
        config.pclGetCommandSetting()->pclCreateCommand(session_id, storage, da, is_combo_fmt_, is_scatter_ver2)->exec(conn);
        LOGI("All command exec done!");

        return 0;
    }catch(const BaseException& e){
        LOGE("Exception: err_code[%d], err_msg[%s]", e.err_code(), e.err_msg().c_str());
        return 1;
    }
}


}

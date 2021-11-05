#include "CommandSettingValidator.h"
#include <QFileInfo>
#include <QDir>
#include "../Utility/version.h"
#include "../Utility/Utils.h"

#define INSTALLED_FILES_VENDOR_NAME "installed-files-vendor.txt"
#define RSC_FILE_NAME "rsc.xml"
#define RSC_FILE_PATTERN "^\\s+\\d+\\s+/vendor/etc/rsc/\\S+/ro.prop$"

ConsoleMode::CommandSettingValidator::CommandSettingValidator(const ConsoleMode::Config &config,
                                                              const CommandLineArguments &args):
    m_config(config), m_args(args)
{

}

ConsoleMode::CommandSettingValidator::~CommandSettingValidator()
{

}

bool ConsoleMode::CommandSettingValidator::Validate() const
{
    return this->ValidateRSCSetting();
}

bool ConsoleMode::CommandSettingValidator::needRSCSetting() const
{
    RSC_EXIST_STATUS_T rsc_exist_status = checkRSCFileStatus();
    return rsc_exist_status != res_rsc_not_need;
}

bool ConsoleMode::CommandSettingValidator::ValidateRSCSetting() const
{
    if (ToolInfo::IsCustomerVer())
    {
        //not support rsc mechanism in customer version
        //so quit if there has rsc cmd in config.xml(download/format/readback) or cmd arguments, no matter rsc.xml has or not.
        if (m_config.pclGetCommandSetting()->hasRSCCmdSetting())
        {
            LOGE("NOT support rsc mechanism in customer tool!");
            return false;
        }
        else
        {
            return true;
        }
    }
    if (!m_config.pclGetCommandSetting()->hasDownloadCmd()) {
        if (m_config.pclGetCommandSetting()->hasRSCCmdSetting()) {
            LOGI("rsc commnad exist, but ignore it for it's not a download command!");
        }
        return true;
    }
    //RSC only for internal flashtool    
    RSC_EXIST_STATUS_T rsc_exist_status = checkRSCFileStatus();
    switch (rsc_exist_status) {
    case res_rsc_exist:
        if (m_config.pclGetCommandSetting()->hasRSCCmdSetting())
        {
            LOGI("rsc file and rsc commnad both exist!");
            if (this->validRSCIndex())
            {
                return true;
            }
            else
            {
                LOGI("invalid rsc_index in rsc commnad!");
                return false;
            }
        }
        else
        {
            LOGE("rsc file exist, but has no rsc command!");
            return false;
        }
    case res_rsc_not_exist:
        if (m_config.pclGetCommandSetting()->hasRSCCmdSetting())
        {
            LOGE("rsc file need but not exist, and has rsc command!!");
        }
        else
        {
            LOGE("rsc file need but not exist, and has no rsc command!!");
        }
        return false;
    case res_rsc_not_need:
        if (m_config.pclGetCommandSetting()->hasRSCCmdSetting())
        {
            LOGW("rsc file no need, and has rsc command, but ignore it!");
        }
        else
        {
            LOGI("rsc file no need, and has no rsc command!!");
        }
        return true;
    default:
        assert(false);
        return true;
    }
}

ConsoleMode::RSC_EXIST_STATUS_T ConsoleMode::CommandSettingValidator::checkRSCFileStatus() const
{
    QString rsc_file = getRSCFileName();

    //check rsc.xml exist or not
    if (QFileInfo(rsc_file).exists())
    {
        LOG("rsc.xml exists!");
        return res_rsc_exist;
    }
    else
    {
        LOG("rsc.xml NOT exist");
        QDir install_files_dir = QFileInfo(rsc_file).dir();
        QString install_files_name = install_files_dir.absoluteFilePath(INSTALLED_FILES_VENDOR_NAME);

        //installed-files-vendor.txt contains vendor/etc/rsc/xxx/ro.prop, xxx is the project name in rsc.xml
        if(FilePatternContain(install_files_name, RSC_FILE_PATTERN))
        {
            LOG("rsc.xml is in the installed-files-vendor.txt but NOT found");
            return res_rsc_not_exist;
        }
        else
        {
            LOG("installed-files-vendor says NO need rsx.xml; or installed-files-vendor NOT exist");
            return res_rsc_not_need;
        }
    }
}

QString ConsoleMode::CommandSettingValidator::getRSCFileName() const
{
    QString sScatterFile = QString::fromStdString(m_config.pclGetGeneralSetting()->pclGetGeneralArg()->scatter_file);
    QString rsc_dir = QFileInfo(sScatterFile).absolutePath();
    QString rsc_file = rsc_dir + QDir::separator().toLatin1() + RSC_FILE_NAME;
    LOG("rsc file: %s", rsc_file.toStdString().c_str());
    return rsc_file;
}

bool ConsoleMode::CommandSettingValidator::validRSCIndex() const
{
    unsigned int rsc_count = 0;
    QString rsc_file = getRSCFileName();
    unsigned int ret = FlashTool_GetRSCCnt(rsc_file.toStdString().c_str(), &rsc_count);
    if(STATUS_OK != ret)
    {
        char err_msg[512] = {0};
        FlashTool_GetLastErrorMessage(NULL, err_msg);
        LOGE("FlashTool_GetRSCCnt() failed! ret: %s(0x%x), error_msg: %s", StatusToString(ret), ret, err_msg);
        return false;
    }
    QSharedPointer<APCore::RSCSetting> rsc_setting = m_config.pclGetCommandSetting()->getRSCSetting();
    if (rsc_setting.isNull())
    {
        return false;
    }
    unsigned int rsc_index = rsc_setting->getRSCIndex();
    return 0 <= rsc_index && rsc_index < rsc_count;
}

#include "Config.h"
#include "../XMLParser/XMLDocument.h"
#include "../XMLParser/XMLNode.h"
#include "../Setting/FormatSetting.h"
#include "../Setting/ComboFormatSetting.h"
#include "../Setting/DADownloadAllSetting.h"
#include "../Setting/DLOnlySetting.h"
#include "../Setting/DLPlugFmtAllSetting.h"
#include "../Setting/CertDLSetting.h"
#include "../Setting/ReadbackSetting.h"
#include "../Setting/FirmwareUpgradeSetting.h"
#include "../Setting/WatchDogSetting.h"
#include "../Setting/EfuseSetting.h"
#include "../Err/Exception.h"
#include "../Err/BromException.h"
#include "../Host/Inc/RuntimeMemory.h"
#include <QtGlobal>

namespace ConsoleMode
{

Config::Config():
      m_pclGeneralSetting(QSharedPointer<GeneralSetting>(new GeneralSetting())),
    m_pclCommandSetting(QSharedPointer<CommandSetting>(new CommandSetting))
{
}

//command_setting should be create after general_setting exec();
Config::Config(const CommandLineArguments& args)
{
    std::string config_file = args.GetInputFilename();
    if (!config_file.empty())
    {
        LOGI("Init config from config file");
        LoadFile(config_file, args.OnlyOutput(), args.reboot());
    }
    else
    {
        LOGI("Init config from input arguments");
        APCore::USBPower power = APCore::AutoDetect;
        if(stricmp(args.GetBatteryOption().c_str(), "with") == 0)
        {
            power = APCore::WithBattery;
        }
        else if(stricmp(args.GetBatteryOption().c_str(), "without") == 0)
        {
            power = APCore::WithoutBattery;
        }
        else if(stricmp(args.GetBatteryOption().c_str(), "auto") == 0)
        {
            power = APCore::AutoDetect;
        }
        m_pclGeneralSetting = QSharedPointer<GeneralSetting>(new GeneralSetting(power, args.StorageLifeCycleCheck()));
        m_pclCommandSetting = QSharedPointer<CommandSetting>(new CommandSetting(
            args.GetScatterFilename(), args.GetRSCProjName()));
    }
    m_pclGeneralSetting->vSetArgs(args);
    m_pclCommandSetting->vSetCommand(args.GetCommand());
}

Config::~Config()
{
}

void Config::LoadFile(const std::string &file_name, bool efuse_read_only, bool reboot)
{
    XML::Document document(file_name);
    const XML::Node root_node = document.GetRootNode();
    Q_ASSERT(root_node.GetName() == "flashtool-config");

    Q_ASSERT(root_node.GetAttribute("version") == "2.0");

    XML::Node general_node = root_node.GetFirstChildNode();
    m_pclGeneralSetting = QSharedPointer<GeneralSetting>(new GeneralSetting(general_node));

    XML::Node cmds_node = general_node.GetNextSibling();
    m_pclCommandSetting = QSharedPointer<CommandSetting>(new CommandSetting(cmds_node, efuse_read_only, reboot));
}

void Config::SaveFile(const std::string &file_name, bool full_general_save) const
{
    XML::Document document("1.0","UTF-8","flashtool-config");
    XML::Node root_node = document.GetRootNode();
    root_node.SetAttribute("version","2.0");

    XML::Node general_node = root_node.AppendChildNode("general");
    if (full_general_save)
    {
        m_pclGeneralSetting->SaveXML(general_node);
    }
    else
    {
        m_pclGeneralSetting->SaveXML_ReadBack_General(general_node);
    }

    XML::Node cmds_node = root_node.AppendChildNode("commands");
    m_pclCommandSetting->SaveXML(cmds_node);

    document.Beautify();
    document.Save(file_name);
}

bool Config::fgIsCommboFmt(QSharedPointer<AppCore> &app, const APKey &key) const
{
    QString sScatterVer = this->getScatterVersion(app, key);
    HW_StorageType_E storage = eGetStorageType();
    if((sScatterVer.indexOf("V2", 0, Qt::CaseInsensitive) != -1 || (sScatterVer.indexOf("V1", 0, Qt::CaseInsensitive) != -1 && stricmp(sScatterVer.toStdString().c_str(), "v1.1.1") > 0))
            && (storage == HW_STORAGE_EMMC || storage == HW_STORAGE_UFS))
        return true;
    return false;
}

bool Config::fgIsScatterVer2(QSharedPointer<AppCore> &app, const APKey &key) const
{
    QString sScatterVer = this->getScatterVersion(app, key);
    return sScatterVer.indexOf("V2", 0, Qt::CaseInsensitive) != -1;
}

HW_StorageType_E Config::eGetStorageType() const
{
    HW_StorageType_E storage_type_ = m_pclGeneralSetting->pclGetGeneralArg()->storage_type;
    return storage_type_;
}

std::string Config::eGetDAFile() const {
    return m_pclGeneralSetting->pclGetGeneralArg()->da_file;
}

const QSharedPointer<APCore::LogInfoSetting> Config::getLogInfoSetting(const std::string &sInputFileName)
{
    XML::Document document(sInputFileName);
    const XML::Node root_node = document.GetRootNode();

    Q_ASSERT(root_node.GetName() == "flashtool-config");
    Q_ASSERT(root_node.GetAttribute("version") == "2.0");

    return GeneralSetting::getLogInfoSetting(root_node.GetFirstChildNode());
}

QString Config::getScatterVersion(QSharedPointer<AppCore> &app, const APKey &key) const
{
    try{
        if(this->m_pclGeneralSetting->pclGetGeneralArg()->scatter_file.length() <= 0)
            return QString();

        char version[64];
        app->GetScatterVersion(key, version);
        return QString(version);
    }catch(const BromException& e){
        THROW_BROM_EXCEPTION_EX(e);
    }
}

}

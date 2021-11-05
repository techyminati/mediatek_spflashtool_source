#ifndef COMMANDSETTING_H
#define COMMANDSETTING_H

#include "CommandLineArguments.h"
#include "../Public/AppCore.h"
#include "../Setting/ISetting.h"
#include "../Setting/FormatSetting.h"
#include "../Setting/ComboFormatSetting.h"
#include "../Setting/DADownloadAllSetting.h"
#include "../Setting/DLOnlySetting.h"
#include "../Setting/DLPlugFmtAllSetting.h"
#include "../Setting/DLWithoutScatterSetting.h"
#include "../Setting/ReadbackWithoutScatterSetting.h"
#include "../Setting/CertDLSetting.h"
#include "../Setting/ReadbackSetting.h"
#include "../Setting/FirmwareUpgradeSetting.h"
#include "../Setting/WatchDogSetting.h"
#include "../Setting/WriteMemorySetting.h"
#include "../Setting/EfuseSetting.h"
#include "../Setting/RSCSetting.h"
#include "../Setting/DRAMRepairSetting.h"
#include "../XMLParser/XMLNode.h"
#include "../Cmd/MacroCommand.h"
#include <list>

namespace ConsoleMode
{
class CommandSetting : public XML::Serializable
{
public:
    CommandSetting(const std::string &scatter_file = "", const std::string &rsc_index_str = "");
    ~CommandSetting();

    CommandSetting(const XML::Node& node, bool efuse_read_only = false, bool reboot = false);

    void vSetCommand(const std::string &command);

    void vAddSetting(const QSharedPointer<ISetting>& setting);
    void vClearSetting();
    void LoadXML(const XML::Node &node);
    void SaveXML(XML::Node &node) const;

    std::list<QSharedPointer<ISetting> > aclGetCmdSettingList() const;

    QSharedPointer<APCore::ICommand> pclCreateCommand(const APKey & key , const HW_StorageType_E& storage_type,
                                                      const std::string &da, const bool& is_combo_fmt, bool is_scatter_ver2) const;
    bool hasRSCCmdSetting() const;
    bool hasDownloadCmd() const;
    void removeRSCSetting();
    QSharedPointer<APCore::RSCSetting> getRSCSetting();

private:
    void vCreateSetting(const std::string& command, const XML::Node &node);

    QSharedPointer<APCore::DADownloadAllSetting> CreateDownloadSetting();
    QSharedPointer<APCore::FormatSetting> CreateFormatSetting();
    QSharedPointer<APCore::ComboFormatSetting> CreateComboFormatSetting();
    QSharedPointer<APCore::DLOnlySetting> CreateDLOnlySetting();
    QSharedPointer<APCore::DLPlugFmtAllSetting> CreateDLPlugFmtAllSetting();
    QSharedPointer<APCore::ComboFormatSetting> CreateDLPlugFmtAllFormatSetting();
    QSharedPointer<APCore::FormatSetting> CreateFirmwareUpgradeFormatSetting();
    QSharedPointer<APCore::FirmwareUpgradeSetting> CreateFirmwareUpgradeSetting();
    QSharedPointer<APCore::DLWithoutScatterSetting> CreateDLWithoutScatterSetting();
    QSharedPointer<APCore::WatchDogSetting> CreateWatchDogSetting();
    QSharedPointer<APCore::ReadbackSetting> CreateReadbackSetting();
    QSharedPointer<APCore::ReadbackWithoutScatterSetting> CreateReadbackWithoutScatterSetting();
    QSharedPointer<APCore::WriteMemorySetting> CreateWriteMemorySetting();
    QSharedPointer<APCore::CertDLSetting> CreateCertDLSetting();
    QSharedPointer<APCore::EfuseSetting> CreateEfuseSetting();
    QSharedPointer<APCore::RSCSetting> CreateRSCSetting() const;
    QSharedPointer<APCore::DRAMRepairSetting> CreateDRAMRepairSetting() const;
    bool isDownloadCmd(const std::string &cmd_name) const;
    bool getRSCInfoByProjName(unsigned int *index, std::string *proj_operator) const;
    QString getRSCFileName() const;
    void addRSCSetting();

    CommandSetting& operator=(const CommandSetting&);

    std::list<QSharedPointer<ISetting> > m_aclCommandSettings;
private:
    bool efuse_read_only_;
    bool isNeedEnableWatchDog_;
    bool m_has_rsc_cmd;
    bool m_has_download_cmd;
    std::string m_scatter_file;
    std::string m_rsc_proj_name;
};

}
#endif // COMMANDSETTING_H

#ifndef COMMANDLINEARGUMENTS_H
#define COMMANDLINEARGUMENTS_H

#include <string>
#include <QString>

namespace ConsoleMode
{

class CommandLineArguments
{
public:
    CommandLineArguments();

    const std::string &GetInputFilename() const;

    const std::string &GetScatterFilename() const;

    const std::string &GetDownloadAgentFilename() const;

    const std::string &GetCommand() const;

    const std::string &GetAuthFilename() const;

    const std::string &GetCertFilename() const;

    const std::string &GetComPort() const;

    bool DoRedirect() const;

    bool DisplayHelpInfo() const;

    const std::string &GetHelpInfo() const;

    std::string GetRSCProjName() const;

    bool Parse(int argc, char **argv);

    bool Validate();
    bool ValidateInputFile() const;

    bool OnlyOutput()const{
        return m_onlyOutput;
    }

    bool reboot() const{
        return m_reboot;
    }

    std::string GetBatteryOption() const{
        return m_battery_option;
    }

    bool StorageLifeCycleCheck() const
    {
        return m_storage_life_cycle_check;
    }

private:
    bool valid_com_port(const QString &usb_com_port) const;
    char swith_long_arg_to_short(const char *arg) const;

private:
    std::string m_szInputFilename;
    std::string m_szDownloadAgentFilename;
    std::string m_szScatterFilename;
    std::string m_szAuthFilename;
    std::string m_szCertFilename;
    std::string m_szCommand;
    std::string m_szPort;
    bool m_fgRedirect;
    bool m_fgDisplayHepInfo;
    bool m_onlyOutput;
    bool m_reboot;
    bool m_storage_life_cycle_check;
    std::string m_rsc_proj_name;
    std::string m_battery_option;
};

}

#endif // COMMANDLINEARGUMENTS_H

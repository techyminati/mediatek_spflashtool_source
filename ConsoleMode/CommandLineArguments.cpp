#include "CommandLineArguments.h"

#ifndef _WIN32
#include <getopt.h>
#endif

#include <iostream>
#include <stdlib.h>
#include <QString>
#include <QStringList>
#include <algorithm>
#include <cctype>
#include "../Utility/FileUtils.h"
#include "../Utility/Utils.h"
#include "../Logger/Log.h"
#include "SchemaValidator.h"
#include "../XMLParser/XMLDocument.h"
#include "../XMLParser/XMLNode.h"
#include "../Utility/version.h"

using std::string;

namespace ConsoleMode
{

static const string CONSOLE_MODE_USAGE(
        "\nUsage: flash_tool OPTION...\n"
        "Run FlashTool in console mode.\n"
        "\nArgument instructions:\n"
        "  -i, --config    console mode configuration file, mandatory with config.xml.\n"
        "          flash_tool -i config.xml\n"
        "  -s, --scatter    scatter file path, mandatory without config.xml.\n"
        "          flash_tool -c download -s MT6575_Anroid_scatter.txt\n"
        "  -c, --operation    flash tool features, mandatory without config.xml, just support:\n"
        "            format\n"
        "            download\n"
        "            format-download\n"
        "            firmware-upgrade\n"
		"            cert-download\n"
        "            efuse\n"
        "            dram-repair\n"
        "          flash_tool -c download -s MT6575_Anroid_scatter.txt\n"
        "  -d, --da_agent    download agent file path.\n"
        "          flash_tool -d MTK_AllInOne_DA.bin -s MT6575_Anroid_scatter.txt -c download\n"
        "  -e, --cert_file    certification file path.\n"
        "          flash_tool -e cert_file.cert -s MT6575_Anroid_scatter.txt -c cert-download\n"
        "  -p, --com_port    com port, format is as COM4+COM6 (BROM download), COM5 (preloader download) on Windows "
        "and format is as 5-2+1-6(BROM download), 5-1(preloader download) on Linux.\n"
        "          flash_tool -p COM4+COM6 -i config.xml\n"
        "          flash_tool -p COM4+COM6 -s MT6575_Anroid_scatter.txt -c download\n"
        "  -r, --redirect    redirect debug log to parent process.\n"
        "          flash_tool -r -i config.xml\n"
        "  -t, --battery_mode    Specify battery mode. with, without and auto are available. if no specify the -t argument, the auto mode used."
        "        Only valid without config.xml.\n"
        "          flash_tool -d MTK_AllInOne_DA.bin  -s MT6575_Anroid_scatter.txt  -c download -t without\n"
        "  -b, --reboot    reboot the device.\n"
        "          flash_tool -b -i config.xml\n"
        "  -o, --efuse_read_only    efuse read only, ONLY on windows.\n"
        "          flash_tool -o -i config.xml\n"
        "  --disable_storage_life_cycle_check  disable storage life cycle check feature.\n"
        "          flash_tool -s MT6575_Anroid_scatter.txt -c download --disable_storage_life_cycle_check\n"
        "  -h, --help    display help infomation and exit.\n"
        "          flash_tool -h\n"
        );

static const string CONSOLE_MODE_CUSTOMER_USAGE = CONSOLE_MODE_USAGE;

static const string CONSOLE_MODE_INTERNAL_USAGE = CONSOLE_MODE_USAGE + (
            "  --rsc  Specify the project name in rsc.xml, ONLY support download/format-download/firmware-upgrade for internal tool.\n"
            "          flash_tool -s MT6575_Anroid_scatter.txt -c download --rsc k75v1_64_rsc_cn[op02]\n"
            );

const QString SUPPORTED_CMDS("combo-format,download-only,format-download,firmware-upgrade,cert-download,efuse,dram-repair");

#define RSC_ARG_TYPE 1
#define STOR_LIFE_CYCLE_CHECK_ARG_TYPE 2

const static std::map<std::string, char>::value_type LONG_SHORT_ARG_INIT_VALUES[] = {
    std::map<std::string, char>::value_type("config", 'i'),
    std::map<std::string, char>::value_type("scatter", 's'),
    std::map<std::string, char>::value_type("operation", 'c'),
    std::map<std::string, char>::value_type("da_agent", 'd'),
    std::map<std::string, char>::value_type("cert_file", 'e'),
    std::map<std::string, char>::value_type("com_port", 'p'),
    std::map<std::string, char>::value_type("redirect", 'r'),
    std::map<std::string, char>::value_type("battery_mode", 't'),
    std::map<std::string, char>::value_type("rsc", '\0'),
    std::map<std::string, char>::value_type("reboot", 'b'),
    std::map<std::string, char>::value_type("disable_storage_life_cycle_check", '\0'),
    std::map<std::string, char>::value_type("help", 'h'),
	std::map<std::string, char>::value_type("efuse_read_only", 'o')
};
const static std::map<string, char> LONG_SHORT_ARG_MAP(LONG_SHORT_ARG_INIT_VALUES, LONG_SHORT_ARG_INIT_VALUES+13);

CommandLineArguments::CommandLineArguments()
    :m_szInputFilename(),
     m_szDownloadAgentFilename(),
     m_szScatterFilename(),
     m_szAuthFilename(),
     m_szCertFilename(),
     m_szCommand(),
     m_szPort(),
     m_fgRedirect(false),
     m_fgDisplayHepInfo(false),
     m_onlyOutput(false),
     m_reboot(false),
     m_battery_option("auto"),
     m_storage_life_cycle_check(true)
{
}

const string &CommandLineArguments::GetInputFilename() const
{
    return m_szInputFilename;
}

const string &CommandLineArguments::GetScatterFilename() const
{
    return m_szScatterFilename;
}

const string &CommandLineArguments::GetDownloadAgentFilename() const
{
    return m_szDownloadAgentFilename;
}

const string &CommandLineArguments::GetCommand() const
{
    return m_szCommand;
}

const string &CommandLineArguments::GetAuthFilename() const
{
    return m_szAuthFilename;
}

const string &CommandLineArguments::GetCertFilename() const
{
    return m_szCertFilename;
}

const std::string &CommandLineArguments::GetComPort() const
{
    return m_szPort;
}

const std::string &CommandLineArguments::GetHelpInfo() const
{
    if (ToolInfo::IsCustomerVer()){
        return CONSOLE_MODE_CUSTOMER_USAGE;
    }
    else {
        return CONSOLE_MODE_INTERNAL_USAGE;
    }
}

std::string CommandLineArguments::GetRSCProjName() const
{
    return m_rsc_proj_name;
}

bool CommandLineArguments::DoRedirect() const
{
    return m_fgRedirect;
}

bool CommandLineArguments::DisplayHelpInfo() const
{
    return m_fgDisplayHepInfo;
}

bool CommandLineArguments::Parse(int argc, char **argv)
{
    bool result = false;
#ifndef _WIN32
    struct option longopts[] = {
      {"config", required_argument, NULL, 'i'},
      {"scatter", required_argument, NULL, 's'},
      {"operation", required_argument, NULL, 'c'},
      {"da_agent", required_argument, NULL, 'd'},
      {"cert_file", required_argument, NULL, 'e'},
      {"com_port", required_argument, NULL, 'p'},
      {"redirect", no_argument, NULL, 'r'},
      {"battery_mode", required_argument, NULL, 't'},
      {"reboot", no_argument, NULL, 'b'},
      {"rsc", required_argument, NULL, RSC_ARG_TYPE},
      {"disable_storage_life_cycle_check", no_argument, NULL, STOR_LIFE_CYCLE_CHECK_ARG_TYPE},
      {"help", no_argument, NULL, 'h'},
      {0, 0, 0, 0}
    };

    while (1)
    {
        int c = getopt_long(argc, argv, "i:d:c:s:p:rhbt:e:", longopts, NULL);

        if (c == -1)
        {
            break;
        }

        // deal with arguments ONLY for internal tool
        if (!ToolInfo::IsCustomerVer())
        {
            if (c == RSC_ARG_TYPE)
            {
                if (!m_szInputFilename.empty()) return false;
                m_rsc_proj_name = optarg;
                continue;
            }
        }

        // arguments for both customer and internal tool.
        switch (c)
        {
            case STOR_LIFE_CYCLE_CHECK_ARG_TYPE:
                m_storage_life_cycle_check = false;
                break;
            case 'i':
                m_szInputFilename = optarg;
                if (!ToolInfo::IsCustomerVer() && !m_rsc_proj_name.empty())
                {
                    return false;
                }
                result = true;
                break;

            case 'r':
                m_fgRedirect = true;
                break;

            case 'd':
                m_szDownloadAgentFilename = optarg;
                break;

            case 's':
                m_szScatterFilename = optarg;
                if(!m_szCommand.empty())
                {
                    if (m_szCommand == "cert-download")
                    {
                        if (!m_szCertFilename.empty())
                        {
                            result = true;
                        }
                    }
                    else
                    {
                        result = true;
                    }
				}
                break;
            case 'c':
                m_szCommand = optarg;
                if(!m_szScatterFilename.empty())
                {
                    if (m_szCommand == "cert-download")
                    {
                        if (!m_szCertFilename.empty())
                        {
                            result = true;
                        }
                    }
                    else
                    {
                        result = true;
                    }
                }
                break;

            case 'p':
                m_szPort = optarg;
                break;

            case 'h':
                m_fgDisplayHepInfo = true;
                result = true;
                break;

            case 'b':
                m_reboot = true;
                break;

            case 't':
                if((stricmp(optarg, "with") != 0)
                        && (stricmp(optarg, "without") != 0)
                        && (stricmp(optarg, "auto") != 0))
                {
                    return false;
                }

                m_battery_option = optarg;
                break;
            case 'e':
                m_szCertFilename = optarg;
                if (!m_szScatterFilename.empty() && !m_szCommand.empty())
                {
                    result = true;
                }
                break;

            default:
               return false;
        }
    }

    return  result;

#else
    for(int i = 1; i < argc; i++)
    {
        if(argv[i][0] == '-')
        {
            char ch = argv[i][1];
            if (strncmp(argv[i], "--", 2) == 0)
			{
                ch = this->swith_long_arg_to_short(argv[i]);
            }

            // deal with arguments ONLY for internal tool
            if (!ToolInfo::IsCustomerVer())
            {
                std::string arg = argv[i];
                if (arg == "--rsc")
                {
                    if (++i >= argc) return false;
                    if (!m_szInputFilename.empty()) return false;
                    m_rsc_proj_name = argv[i];
                    continue;
                }
            }

            // arguments for both customer and internal tool.
            switch(tolower(ch))
            {
            case 'i':
                if (++i >= argc)
                {
                    return false;
                }
                m_szInputFilename = argv[i];
                if (!ToolInfo::IsCustomerVer() && !m_rsc_proj_name.empty())
                {
                    return false;
                }
                result = true;
                break;

            case 'r':
                m_fgRedirect = true;
                break;
            case 'd':
                if (++i >= argc)
                {
                    return false;
                }
                m_szDownloadAgentFilename = argv[i];
                break;

            case 's':
                if (++i >= argc)
                {
                    return false;
                }
                m_szScatterFilename = argv[i];
                if(!m_szCommand.empty())
                {
                    if (m_szCommand == "cert-download")
                    {
                        if (!m_szCertFilename.empty())
                        {
                            result = true;
                        }
                    }
                    else
                    {
                        result = true;
                    }
                }
                break;
            case 'c':
                if (++i >= argc)
                {
                    return false;
                }
                m_szCommand = argv[i];
                if(!m_szScatterFilename.empty())
                {
                    if (m_szCommand == "cert-download")
                    {
                        if (!m_szCertFilename.empty())
                        {
                            result = true;
                        }
                    }
                    else
                    {
                        result = true;
                    }
                }
                break;
            case 'p':
                if (++i >= argc)
                {
                    return false;
                }
                m_szPort = argv[i];
                break;

            case 'h':
                m_fgDisplayHepInfo = true;
                result = true;
                break;

            case 'o':
                m_onlyOutput = true;
                break;

            case 'b':
                m_reboot = true;
                break;

            case 't':
                if (++i >= argc)
                {
                    return false;
                }
                if((stricmp(argv[i], "with") != 0)
                        && (stricmp(argv[i], "without") != 0)
                        && (stricmp(argv[i], "auto") != 0))
                {
                    return false;
                }

                m_battery_option = argv[i];
                break;
            case 'e':
                if (++i >= argc)
                {
                    return false;
                }
                m_szCertFilename = argv[i];
                if (!m_szScatterFilename.empty() && !m_szCommand.empty())
                {
                    result = true;
                }
                break;
             default: // deal with case of only long argument or invalid argument.
             {
                std::string arg = argv[i];
                if (arg == "--disable_storage_life_cycle_check")
                {
                    m_storage_life_cycle_check = false;
                }
                else {
                    return false; // invalid argument
                }
             }
            }
        }
    }

    return result;

#endif
}

char CommandLineArguments::swith_long_arg_to_short(const char *arg) const
{
    assert(NULL != arg && strncmp(arg, "--", 2) == 0);
    std::string long_arg = std::string(arg + 2);
    //std::transform(long_arg.begin(), long_arg.end(), long_arg.begin(), tolower);
    char ch = '\0';
    std::map<string, char>::const_iterator it = LONG_SHORT_ARG_MAP.find(long_arg);
    if(it != LONG_SHORT_ARG_MAP.end())
    {
        ch = it->second;
    }
    return ch;
}

bool CommandLineArguments::Validate()
{
    if (!ValidateInputFile())
    {
        return false;
    }

    if(!this->m_szScatterFilename.empty() && !FileUtils::IsFileExist(m_szScatterFilename))
    {
        LOGE("\"%s\": file not found", m_szScatterFilename.c_str());
        return false;
    }

    if(!this->m_szDownloadAgentFilename.empty() && !FileUtils::IsFileExist(m_szDownloadAgentFilename))
    {
        LOGE("\"%s\": file not found", m_szDownloadAgentFilename.c_str());
        return false;
    }

    if(!this->m_szCommand.empty())
    {
        transform(m_szCommand.begin(),m_szCommand.end(),m_szCommand.begin(),tolower);
        if("download" ==m_szCommand)
        {
            m_szCommand = "download-only";
        }
        else if ("format" == m_szCommand)
        {
            m_szCommand = "combo-format";
        }
        QString cmd = QString::fromStdString(m_szCommand);

        QStringList list = SUPPORTED_CMDS.split(",");
        if(!list.contains(cmd))
        {
            LOGE("Command \"%s\":is not support!", m_szCommand.c_str());
            return false;
        }
    }

    if(!this->m_szPort.empty())
    {
        std::string::size_type index = m_szPort.find("+");
        if(index != std::string::npos)
        {
            std::string full_com_port = m_szPort.substr(0, index);
            std::string high_com_port = m_szPort.substr(index + 1);
            if (!valid_com_port(full_com_port.c_str()) || !valid_com_port(high_com_port.c_str()))
            {
                return false;
            }
        }
        else
        {
            if (!valid_com_port(m_szPort.c_str()))
            {
                return false;
            }
        }
    }

    if(!this->m_szAuthFilename.empty() && !FileUtils::IsFileExist(m_szAuthFilename))
    {
        LOGE("\"%s\": file not found", m_szAuthFilename.c_str());
        return false;
    }

    if(!this->m_szCertFilename.empty() && !FileUtils::IsFileExist(m_szCertFilename))
    {
        LOGE("\"%s\": file not found", m_szCertFilename.c_str());
        return false;
    }

    return true;
}

bool CommandLineArguments::valid_com_port(const QString &usb_com_port) const
{
#ifdef _WIN32
    int port_num = Utils::GetPortNumFromStr(usb_com_port);
    return port_num > 0;
#else
    QRegExp regex("^\\d+-\\d+(?:\\.\\d+)*$");
    int pos = regex.indexIn(usb_com_port);
    return pos != -1;
#endif
}

bool CommandLineArguments::ValidateInputFile() const
{
    // validate configuration file
    if(!m_szInputFilename.empty())
    {
        if (!FileUtils::IsFileExist(m_szInputFilename))
        {
            LOGI("\"%s\": file not found", m_szInputFilename.c_str());
            return false;
        }

        SchemaValidator validator(Utils::GetTextCodec()->toUnicode(ABS_PATH_C("console_mode.xsd")));

        try
        {
            validator.Validate(QString::fromStdString(m_szInputFilename));
        }
        catch(...)
        {
            LOGI("Invalid xml file, please check the input file and try again.");
            return false;
        }
    }
    return true;
}

}

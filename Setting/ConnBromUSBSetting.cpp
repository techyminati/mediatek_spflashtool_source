#include "ConnBromUSBSetting.h"
#include "../XMLParser/XMLNode.h"
#include "../Host/Inc/DeviceConn.h"

#include <QtGlobal>

namespace APCore
{

static const std::map<DA_LOG_CHANNEL_E, QString>::value_type DA_LOG_CHANNEL_INIT[] ={
    std::map<DA_LOG_CHANNEL_E, QString>::value_type(DA_LOG_CHANNEL_NONE, "NONE"),
    std::map<DA_LOG_CHANNEL_E, QString>::value_type(DA_LOG_CHANNEL_UART, "UART"),
    std::map<DA_LOG_CHANNEL_E, QString>::value_type(DA_LOG_CHANNEL_USB, "USB"),
    std::map<DA_LOG_CHANNEL_E, QString>::value_type(DA_LOG_CHANNEL_UART_USB, "UART_USB")
};

static const std::map<DA_LOG_CHANNEL_E, QString> DA_LOG_CHANNEL_MAP(DA_LOG_CHANNEL_INIT, DA_LOG_CHANNEL_INIT + 4);

DA_LOG_CHANNEL_E getDALogChannelType(const QString &da_log_channel_name)
{
    DA_LOG_CHANNEL_E da_log_channel_type = DA_LOG_CHANNEL_UART;
    typedef std::map<DA_LOG_CHANNEL_E, QString>::const_iterator c_iterator_t;
    for (c_iterator_t citer = DA_LOG_CHANNEL_MAP.begin(); citer != DA_LOG_CHANNEL_MAP.end(); ++citer)
    {
        if (citer->second == da_log_channel_name)
        {
            da_log_channel_type = citer->first;
            break;
        }
    }
    return da_log_channel_type;
}

QString getDALogChannelName(DA_LOG_CHANNEL_E da_log_channel_type)
{
    QString da_log_channel_name = "UART";
    typedef std::map<DA_LOG_CHANNEL_E, QString>::const_iterator c_iterator_t;
    c_iterator_t citer = DA_LOG_CHANNEL_MAP.find(da_log_channel_type);
    if (citer != DA_LOG_CHANNEL_MAP.end())
    {
        da_log_channel_name = citer->second;
    }
    return da_log_channel_name;
}

ConnBromUSBSetting::ConnBromUSBSetting(USBSpeed speed, USBPower power)
    : speed_(speed), power_(power),
      m_da_log_level(DA_LOG_LEVEL_INFO),
      m_da_log_channel(DA_LOG_CHANNEL_UART),
      cb_download_da_init_(NULL),
      cb_download_da_(NULL),
      cb_usb_stat_init_(NULL)
{
}

ConnBromUSBSetting::ConnBromUSBSetting(const XML::Node &node)
    : cb_download_da_init_(NULL),
      cb_download_da_(NULL),
      cb_usb_stat_init_(NULL)
{
    LoadXML(node);
}

Connection *ConnBromUSBSetting::CreateConnection(
        APKey key, HW_StorageType_E stor, bool pwr_key_reset)
{
    BromBootArg args;

    FlashTool_Connect_Arg &ft_conn_arg =
            (FlashTool_Connect_Arg&)(args.GetBromBootArg());

    if(cb_download_da_init_ != NULL)
    {
        args.set_cb_download_da_init(cb_download_da_init_);
    }
    if(cb_download_da_ != NULL)
    {
        args.set_cb_download_da(cb_download_da_);
    }

    int force_chg = 0;

    if (power_== AutoDetect)
        force_chg = FORCE_CHARGE_AUTO;
    else if (power_== WithoutBattery)
        force_chg = FORCE_CHARGE_ON;
    else
        force_chg = FORCE_CHARGE_OFF;

    ft_conn_arg.m_force_charge = force_chg;

    ft_conn_arg.m_reset_key =
        pwr_key_reset ? RESET_BY_PWR_KEY_ALONE : RESET_BY_PWR_HOME_KEY;

    ft_conn_arg.m_da_log_level = get_da_log_level();

    ft_conn_arg.m_da_log_channel = get_da_log_channel();

    _SHARED_PTR<ConnMedia> med = _SHARED_PTR<ConnMedia>(new ConnMediaUSB(USBHighSpeed==speed_));

    if(!m_com_port.empty())
    {
        std::string::size_type index = m_com_port.find("+");
        if(index != std::string::npos)
        {
            std::string n1 = m_com_port.substr(0,index);
            std::string n2 = m_com_port.substr(index+1);
            LOGI("Specify Normal com port = %s , HighSpeed com port = %s." , n1.c_str(), n2.c_str());
            med->setFriendlyName(n1);
            med->setHighSpeedFriendlyName(n2);
        }
        else
        {
            med->setFriendlyName(m_com_port);
        }
    }

    _SHARED_PTR<ConnArgBox> box = _SHARED_PTR<ConnArgBox>(new ConnArgBox(
                key, get_stop_flag(), get_timeout(),stor, ft_conn_arg));

    return new Connection(med, box, get_storage_life_cycle_check());
}

void ConnBromUSBSetting::LoadXML(const XML::Node &node)
{
    Q_ASSERT(node.GetAttribute("type") == "BromUSB");

    std::string speed_str = node.GetAttribute("high-speed");
    speed_ = speed_str=="true"?USBHighSpeed:USBFullSpeed;

    std::string power_str = node.GetAttribute("power");
    power_ = AutoDetect; //default value
    if(power_str == "WithBattery")
    {
        power_ = WithBattery;
    }
    else if(power_str == "WithoutBattery")
    {
        power_ = WithoutBattery;
    }
    else if(power_str == "AutoDetect")
    {
        power_ = AutoDetect;
    }

    std::string str_log_level = node.GetAttribute("da_log_level");
    m_da_log_level = DA_LOG_LEVEL_INFO;
    if(str_log_level == "Trace")
    {
        m_da_log_level = DA_LOG_LEVEL_TRACE;
    }
    else if(str_log_level == "Debug")
    {
        m_da_log_level = DA_LOG_LEVEL_DEBUG;
    }
    else if(str_log_level == "Info")
    {
        m_da_log_level = DA_LOG_LEVEL_INFO;
    }
    else if(str_log_level == "Warning")
    {
        m_da_log_level = DA_LOG_LEVEL_WARNING;
    }
    else if(str_log_level == "Error")
    {
        m_da_log_level = DA_LOG_LEVEL_ERROR;
    }
    else if(str_log_level == "Fatal")
    {
        m_da_log_level = DA_LOG_LEVEL_FATAL;
    }

    std::string str_da_log_channel = node.GetAttribute("da_log_channel");
    m_da_log_channel = getDALogChannelType(QString::fromStdString(str_da_log_channel));

    std::string timeout_str = node.GetAttribute("timeout-count");
    if(timeout_str.size()==0)
        m_timeout = 120000;
    else
        m_timeout = atoi(timeout_str.c_str());

    m_com_port = node.GetAttribute("com-port");

    std::string storage_life_cycle_check = node.GetAttribute("storage_life_cycle_check");
    this->set_storage_life_cycle_check(storage_life_cycle_check == "true");
}

void ConnBromUSBSetting::SaveXML(XML::Node &node) const
{
    XML::Node child = node.AppendChildNode("connection");
    child.SetAttribute("type", "BromUSB");
    child.SetAttribute("high-speed",speed_==USBHighSpeed?"true":"false");

    //set power option;
    if(power_ == WithBattery)
        child.SetAttribute("power","WithBattery");
    else if(power_ == WithoutBattery)
        child.SetAttribute("power","WithoutBattery");
    else if(power_ == AutoDetect)
        child.SetAttribute("power","AutoDetect");

    //set da_log_level
    std::string str_log_level = "Info";
    switch(m_da_log_level)
    {
    case DA_LOG_LEVEL_TRACE:
        str_log_level = "Trace";
        break;
    case DA_LOG_LEVEL_DEBUG:
        str_log_level = "Debug";
        break;
    case DA_LOG_LEVEL_INFO:
        str_log_level = "Info";
        break;
    case DA_LOG_LEVEL_WARNING:
        str_log_level = "Warning";
        break;
    case DA_LOG_LEVEL_ERROR:
        str_log_level = "Error";
        break;
    case DA_LOG_LEVEL_FATAL:
        str_log_level = "Fatal";
        break;
    default:
        str_log_level = "Info";
        break;
    };
    child.SetAttribute("da_log_level", str_log_level);

    QString str_da_log_channel_name = getDALogChannelName(m_da_log_channel);
    child.SetAttribute("da_log_channel", str_da_log_channel_name.toStdString());

    child.SetAttribute("timeout-count",QString::number(m_timeout).toStdString());
    child.SetAttribute("com-port",m_com_port);

    child.SetAttribute("storage_life_cycle_check", get_storage_life_cycle_check()? "true": "false");
}

}

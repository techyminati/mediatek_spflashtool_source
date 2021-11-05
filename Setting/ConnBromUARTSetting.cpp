#include "ConnBromUARTSetting.h"
#include "../XMLParser/XMLNode.h"
#include "../Conn/Connection.h"
#include "../Host/Inc/RuntimeMemory.h"

#include <QtGlobal>

namespace APCore
{

ConnBromUARTSetting::ConnBromUARTSetting(const std::string& port)
    : com_port_(port),
      m_da_log_level(DA_LOG_LEVEL_INFO),
      m_da_log_channel(DA_LOG_CHANNEL_UART),
      cb_download_da_init_(NULL),
      cb_download_da_(NULL),
      m_baudrate(0)
{
}

ConnBromUARTSetting::ConnBromUARTSetting(const XML::Node &node)
    : m_da_log_level(DA_LOG_LEVEL_INFO),
      m_da_log_channel(DA_LOG_CHANNEL_UART),
      cb_download_da_init_(NULL),
      cb_download_da_(NULL),
      m_baudrate(0)
{
    LoadXML(node);
}

Connection *ConnBromUARTSetting::CreateConnection(
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

    ft_conn_arg.m_force_charge = FORCE_CHARGE_AUTO;

    ft_conn_arg.m_reset_key =
        pwr_key_reset ? RESET_BY_PWR_KEY_ALONE : RESET_BY_PWR_HOME_KEY;

    ft_conn_arg.m_boot_arg.m_baudrate = get_baudrate();

    ft_conn_arg.m_da_log_level = get_da_log_level();

    ft_conn_arg.m_da_log_channel = get_da_log_channel();

    _SHARED_PTR<ConnArgBox> box = _SHARED_PTR<ConnArgBox>(new ConnArgBox(
                key, get_stop_flag(),get_timeout(), stor, ft_conn_arg));

    _SHARED_PTR<ConnMedia> med = _SHARED_PTR<ConnMedia>(new ConnMediaUART(com_port_));

    return new Connection(med, box, get_storage_life_cycle_check());
}

void ConnBromUARTSetting::LoadXML(const XML::Node &node)
{
    Q_ASSERT(node.GetAttribute("type") == "BromUART");
    com_port_ = node.GetAttribute("port");
}

void ConnBromUARTSetting::SaveXML(XML::Node &node) const
{
    XML::Node child = node.AppendChildNode("connection");
    child.SetAttribute("type", "BromUART");
    child.SetAttribute("port", com_port_);
}

}

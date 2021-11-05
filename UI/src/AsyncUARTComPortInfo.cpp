#include "AsyncUARTComPortInfo.h"
#include "../../Utility/USBCOMFinderWrapper.h"
#include "../../Logger/Log.h"


AsyncUARTComPortInfo::AsyncUARTComPortInfo(): QThread()
{

}

AsyncUARTComPortInfo::~AsyncUARTComPortInfo()
{

}

void AsyncUARTComPortInfo::run()
{
    std::list<std::string> com_port_list;
    int ret = USBCOMFinderWrapper::getComPortInfoStrs(com_port_list);

    if(COM_ENUM_OK != ret)
    {
        if(COM_ENUM_NO_COM_PORT_FOUND== ret)
            LOGI("No com port detected.");
        else
            LOGI("Find com port failed.");

        return;
    }
    emit signal_send_uart_infos(com_port_list);
}

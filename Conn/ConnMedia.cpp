/*
 * Connection.cpp
 *
 *  Created on: Aug 27, 2011
 *      Author: MTK81019
 */

#include "ConnMedia.h"
#include "ConnArgBox.h"
#include "USBSetting.h"
#include "../Logger/Log.h"
#include "../Err/Exception.h"
#include "../Utility/Utils.h"


namespace APCore
{
// class ConnMediaUSB
bool ConnMediaUSB::Search(ConnArgBox *pArgBox)
{
    LOGI("Search usb, timeout set as %d ms",pArgBox->timeout() );
    if (USBSetting::instance()->Search(pArgBox->StopFlag(),pArgBox->timeout()))
    {
        dev_path_ = USBSetting::instance()->symbolic();
        friendly_name_ = USBSetting::instance()->friendly();
    }
    else
    {
        friendly_name_.clear();
        dev_path_.clear();
    }

    return !dev_path_.empty() || !friendly_name_.empty();
}

int ConnMediaUSB::GetUsbStatus(const FLASHTOOL_API_HANDLE_T p_ft_handle)
{
    FlashTool_USB_Status_Result result = {USB_STATUS_UNKNOWN};

    int ret = FlashTool_CheckUSBStatus(
                p_ft_handle, NULL, &result);

    if(S_DONE == ret)
    {
        usb_status_ = result.usb_speed_status;
    }
    else
        usb_status_ = USB_STATUS_UNKNOWN;

    return usb_status_;
}

int ConnMediaUSB::PreConnect(ConnArgBox *pArgBox)
{
    LOGI("Scanning USB port...");

    USBSetting::instance()->AddUSBPool(NORMAL_SPEED);
    if(!friendly_name_.empty())
    {
        LOGI("Preconnect com port: %s\n", friendly_name_.c_str());
        int com_port = Utils::GetPortNumFromStr(friendly_name_.c_str());
        std::string com_port_str = com_port > 0 ? QString::number(com_port).toStdString() : friendly_name_;
        USBSetting::instance()->SetPreferComPort(com_port_str);
    }

    bool result = Search(pArgBox);


    if (result)
    {
        LOGI("USB port detected: %s", dev_path_.c_str());

        return S_DONE;
    }
    else
    {
        LOGI("Failed to find USB port");
        return S_TIMEOUT;
    }
}

int ConnMediaUSB::PostConnect(ConnArgBox *pArgBox)
{
    FlashTool_USB_Status_Result result = {USB_STATUS_UNKNOWN};

    int ret = FlashTool_CheckUSBStatus(
                pArgBox->FTHandle(), NULL, &result);

    if (USB_FULL_SPEED == result.usb_speed_status && to_hi_speed_)
    {
        FLASHTOOL_API_HANDLE_T h = pArgBox->FTHandle();

        LOGI("Switching to USB high speed...");

        // why do you want a pointer???
        ret = FlashTool_SetupUSBDL(&h, 1);

        if (S_DONE == ret)
        {
            if(!friendly_name_h_.empty())
            {
                int port = Utils::GetPortNumFromStr(friendly_name_h_.c_str());
                COM_PORT_HANDLE com;
                if(port >0)
                {
                    LOGI("User specify high speed com port..., %s", friendly_name_h_.c_str());
                    INIT_COM_PORT_HANDLE_BY_NUM(com, port);
                }
                else {
                    USBSetting::instance()->AddUSBPool(DA_HIGH_SPEED);
                    USBSetting::instance()->SetPreferComPort(friendly_name_h_);
                    if (Search(pArgBox))
                    {
                        LOGI("User specify high speed com port..., %s", dev_path_.c_str());
                        Q_ASSERT(!dev_path_.empty() && dev_path_.compare("Unsupported") != 0);
                        INIT_COM_PORT_HANDLE_BY_NAME(com, dev_path_.c_str());
                    }
                }
                // why do you want a pointer???
                ret = FlashTool_ChangeCOM_Ex(&h, &com);
                if(S_DONE != ret)
                {
                    LOGI("FlashTool_ChangeCOM_Ex failed, ret: %s(0x%x)", StatusToString(ret), ret);
                    return ret;
                }
            }
            else
            {
                LOGI("Scaning DA high speed USB port...");
                USBSetting::instance()->AddUSBPool(DA_HIGH_SPEED);
                USBSetting::instance()->SetForibStop(true);
                if (Search(pArgBox))
                {
                    COM_PORT_HANDLE com;
                    LOGI("device path: %s\n", dev_path_.c_str());
                    if(!dev_path_.empty() && dev_path_.compare("Unsupported") != 0)
                    {
                        INIT_COM_PORT_HANDLE_BY_NAME(com, dev_path_.c_str());
                    }
                    else
                    {

                        INIT_COM_PORT_HANDLE_BY_NUM(com,
                                                    Utils::GetPortNumFromStr(friendly_name_.c_str()));
                    }

                    LOGI("DA high speed USB port detected: %s", dev_path_.c_str());
                    // why do you want a pointer???
                    ret = FlashTool_ChangeCOM_Ex(&h, &com);                    

                    if(S_DONE != ret)
                    {
                        LOGI("FlashTool_ChangeCOM_Ex failed, ret: %s(0x%x)", StatusToString(ret), ret);
                        return ret;
                    }
                }
                else
                {
                    ret = S_TIMEOUT;
                    LOGI("Failed to find DA high speed USB port, error code: %s(%d)", StatusToString(ret), ret);
                    return ret;
                }
            }
        }
		else
		{
            LOGI("FlashTool_SetupUSBDL failed, ret: %s(0x%x)", StatusToString(ret), ret);
			return ret;
		}
    }

    GetUsbStatus(pArgBox->FTHandle());

    return S_DONE;
}

} /* namespace APCore */

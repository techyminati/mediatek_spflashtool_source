#include "ICommand.h"
#include <algorithm>
#include "../BootRom/flashtoolex_api.h"
#include "../Logger/Log.h"


namespace APCore
{

status_t ICommand::connect(HSESSION *hs)
{
    status_t status = STATUS_OK;
    uint32 cnt = 0;
    char buffer[32] = {0};
    std::string tempstr;

    const char* filter[3] =
    {
        (char*)"USB\\VID_0E8D&PID_0003",
        (char*)"USB\\VID_0E8D&PID_2000",
        (char*)"USB\\VID_0E8D&PID_2001"
    };

    char port_name[256] = {0};
    LOGI("Wait for device. Plug in USB.");
    status = flashtool_waitfor_com(filter, 3, port_name);
    if(status != STATUS_OK)
    {
        goto exit;
    }

    *hs = flashtool_create_session();
    if(*hs == INVALID_HSESSION_VALUE)
    {
        goto exit;
    }

   LOGI("Connect port: ");
    status = flashtool_connect_brom(*hs, port_name, NULL, NULL, NULL);
    if(status != STATUS_OK)
    {
        goto exit;
    }

    connect_params_struct_t conn_da_params;
    conn_da_params.checksum_level = 0;
    conn_da_params.battery_setting = 2;
    conn_da_params.connect_da_end_stage = CONN_SECOND_DA;
    file_info da_info;
    memset(&da_info, 0, sizeof(da_info));
    da_info.input_type = OP_SOURCE_FILE;
    if (strlen(da_.c_str()) >= MAX_FILE_NAME_LEN)
    {
        status = STATUS_DA_FILE_INVALID;
        goto exit;
    }
    strcpy(da_info.file_path, da_.c_str());

    status = flashtool_connect_da(*hs, &da_info, NULL, NULL, &conn_da_params, NULL);
    if(status != STATUS_OK)
    {
        goto exit;
    }

    status = flashtool_device_control(*hs, DEV_DA_GET_USB_SPEED, 0, 0, buffer, 32, &cnt);
    if(status != STATUS_OK)
    {
        goto exit;
    }

    tempstr = buffer;
    std::transform(tempstr.begin(), tempstr.end(), tempstr.begin(), toupper);
    if(strcmp(tempstr.data(), "USB_FULL_SPEED") == 0)
    {
        status = flashtool_switch_com(*hs, filter+2, 1); //switch to DA com port

        if(status != STATUS_OK)
        {
            goto exit;
        }
    }

exit:
    return status;
}

void ICommand::reboot_device(HSESSION hs)
{
    struct reboot_option option = {0};
    option.is_dev_reboot = 1;
    option.timeout_ms = 3000;

    flashtool_shutdown_device(hs, option);
}

}

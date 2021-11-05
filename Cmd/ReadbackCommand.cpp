#include "ReadbackCommand.h"
#include "../Logger/Log.h"
#include "../Err/Exception.h"
#include "../Utility/FileUtils.h"
#include "../Utility/Utils.h"

namespace APCore
{

ReadbackCommand::ReadbackCommand(APKey key)
    :ICommand(key), readback_arg_(), m_is_scatter_ver2(false), m_is_sram_rb(false)
{
}

ReadbackCommand::~ReadbackCommand()
{

}

void ReadbackCommand::exec(const QSharedPointer<Connection> &conn)
{
    LOGI("m_is_sram_rb: %d", m_is_sram_rb);
    if(m_is_sram_rb)
    {
        conn->ConnectDA(FIRST_DA, _FALSE);
    }
    else
    {
        conn->ConnectDA();
    }

    if (m_is_scatter_ver2)
    {
        HW_StorageType_E storage_type = getDeviceStorageType(&(conn->da_report()));
        if (storage_type == HW_STORAGE_NONE || storage_type == HW_STORAGE_TYPE_END) {
            LOGE("error: unknown storage type!");
            THROW_BROM_EXCEPTION(STATUS_UNKNOWN_STORAGE_TYPE, 0);
        }
        readback_arg_.set_storage_type(storage_type);
    }
    LOGI("executing Readback command...");

    FlashTool_Readback_Result rb_result;
    memset(&rb_result, 0, sizeof(FlashTool_Readback_Result));

    int ret = FlashTool_Readback(
                conn->FTHandle(),
                readback_arg_.raw_arg(),
                &rb_result);
    LOGD("Readback result: %s(0x%x)", StatusToString(ret), ret);
    if(ret != S_DONE)
    {
        char out_buffer[128] = {0};
        unsigned int bytes_returned = 0;
        int status= FlashTool_Device_Control(conn->FTHandle(),
                                             DEV_DA_GET_ERROR_DETAIL,
                                             0,
                                             0,
                                             out_buffer,
                                             128,
                                             &bytes_returned);
        LOGI("DEV_DA_GET_ERROR_DETAIL status: 0x%x", status);
        if(status != S_DONE)
        {
            //for not support DEV_DA_GET_ERROR_DETAIL, just return ret is ok
            THROW_BROM_EXCEPTION(ret,0);
        }
        else
        {
            std::string err_msg = out_buffer;
            unsigned long lba = *(unsigned long*)(out_buffer+EMMC_ERR_TYPE_STR_MAXSIZE+1);
            LOGI("lba: %lu", lba);
            err_msg += "\nlba: "+ NumberToString(lba);
            LOGI("err_msg: %s", err_msg.c_str());
            THROW_APP_EXCEPTION(ret, err_msg);
        }
    }

    LOGI("Readback Succeeded.");
}

}

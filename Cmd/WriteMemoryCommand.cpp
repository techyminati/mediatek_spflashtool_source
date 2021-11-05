#include "WriteMemoryCommand.h"

#include "../Err/Exception.h"
#include "../Logger/Log.h"
#include "../Utility/FileUtils.h"
#include "../UI/src/ICallback.h"
#include "../Utility/Utils.h"

namespace APCore
{
WriteMemoryCommand::WriteMemoryCommand(APKey key):
   ICommand(key), wm_arg_(),
   cb_write_memory_init(NULL),
   is_by_sram_(false),
   m_is_scatter_ver2(false)
{}

WriteMemoryCommand::~WriteMemoryCommand()
{}

int __stdcall WriteMemoryCommand::cb_write_memory_init_console()
{
    LOGI("Write Memory Initial...");
    return 0;
}

void WriteMemoryCommand::exec(
        const QSharedPointer<Connection> &conn)
{
    LOG("is_by_sram(): %d", is_by_sram());
    if(is_by_sram())
    {
        conn->ConnectDA(FIRST_DA, _FALSE);//no need enable DRAM
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
        wm_arg_.set_flash_type(storage_type);
    }

    LOG("executing Write Memory Command...");
    int ret = 0;

    if(cb_write_memory_init != NULL)
    {
          cb_write_memory_init();
    }
    else
    {
        this->cb_write_memory_init_console();
    }

    ret = FlashTool_WriteFlashMemory(conn->FTHandle(), wm_arg_.raw_arg(), is_by_sram()?_TRUE:_FALSE);
    if( S_DONE != ret )
    {
         LOG("ERROR: FlashTool_WriteFlashMemory() failed, error code: %s(%d)!", StatusToString(ret), ret);
         THROW_BROM_EXCEPTION(ret, 0);
    }
}

}



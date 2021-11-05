#include "DRAMRepairCommand.h"
#include "../Conn/Connection.h"
#include "../Err/Exception.h"

namespace APCore
{

DRAMRepairCommand::DRAMRepairCommand(APKey key):
    ICommand(key),
    m_repair_cb(NULL)
{

}

DRAMRepairCommand::~DRAMRepairCommand()
{

}

void DRAMRepairCommand::exec(const QSharedPointer<Connection> &conn)
{
    int status_dram_repair = STATUS_OK;
    conn->ConnectDA(FIRST_DA, _TRUE); //to get FLASHTOOL_API_HANDLE_T instance
    status_dram_repair = FlashTool_Device_Control(conn->FTHandle(), DEV_DA_DRAM_REPAIR, 0, 0, 0, 0, 0);
    if (m_repair_cb != NULL)
    {
        m_repair_cb(status_dram_repair);
    }
    // for DRAMRepairCommand, only STATUS_DRAM_REPAIR_COMPLETE means success,
    // other status codes all mean failed, specially STATUS_OK.
    if (status_dram_repair != STATUS_DRAM_REPAIR_COMPLETE)
    {
        if (status_dram_repair == STATUS_OK)
        {
            LOGI("DRAM repair: DRAM reapair address is NOT written!");
        }
        else
        {
            LOGE("DRAM repair failed: %s(0x%x)", StatusToString(status_dram_repair), status_dram_repair);
        }
        THROW_BROM_EXCEPTION(status_dram_repair, 0);
    }
    LOGI("DRAM repair successed: %s(0x%x)", StatusToString(status_dram_repair), status_dram_repair);
}

void DRAMRepairCommand::set_cb_repair(MEMORY_TEST_REPAIR_CALLBACK cb)
{
    m_repair_cb = cb;
}

}

#include "RSCCommand.h"
#include "../Conn/Connection.h"
#include "../BootRom/flashtoolex_api.h"
#include "../Logger/Log.h"
#include "../Err/Exception.h"

namespace APCore
{
RSCCommand::RSCCommand(APKey key):
    ICommand(key),
    m_rsc_index(0),
    m_rsc_proj_name("")
{
}

RSCCommand::~RSCCommand()
{
}

void RSCCommand::exec(const QSharedPointer<APCore::Connection> &conn)
{
    conn->ConnectDA();

    LOGI("call FlashTool_SetRSCInfo with rsc_index: %d, rsc_proj_name: %s, rsc_operator_name: %s",
         m_rsc_index, m_rsc_proj_name.c_str(), m_rsc_operator_name.c_str());
    proj_item rsc_proj;
    rsc_proj.dtbo_index = m_rsc_index;
    if (m_rsc_proj_name.length() >= FULL_PRJ_NAME_LEN || m_rsc_operator_name.length() >= OP_NAME_LEN)
    {
		LOGE("error: rsc_full_proj_name or rsc_operator_name exceed range!");
        THROW_APP_EXCEPTION(S_INVALID_ARGUMENTS, "");
    }
    strcpy((char*)rsc_proj.full_project_name, (char*)m_rsc_proj_name.c_str());
    strcpy((char*)rsc_proj.op_name, (char*)m_rsc_operator_name.c_str());
    int ret = FlashTool_SetRSCInfo(conn->FTHandle(), &rsc_proj);
    if(ret != STATUS_OK)
    {
        LOGE("FlashTool_SetRSCInfo() failed! ret: %s(0x%x)", StatusToString(ret), ret);
        THROW_BROM_EXCEPTION(ret, 0);
    }
    LOGI("FlashTool_SetRSCInfo() Succeeded.");
}
}


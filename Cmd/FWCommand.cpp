#include "FWCommand.h"

#include "../Err/Exception.h"
#include "../Logger/Log.h"
#include "../Utility/FileUtils.h"
#include "../UI/src/ICallback.h"
#include "../Utility/Utils.h"

namespace APCore
{

FWCommand::FWCommand(APKey key):
    ICommand(key),
    m_fw_file("")
{}

FWCommand::~FWCommand()
{}

void FWCommand::exec(const QSharedPointer<Connection> &conn)
{
    conn->ConnectDA();

    LOG("executing Firmware Upgrade Command...");
    LOG("fw_file: %s", m_fw_file.c_str());
    int ret = 0;
    ret = FlashTool_FirmwareUpdate(conn->FTHandle(), m_fw_file.c_str(), NULL, 0);
    //LOG("test, check fw_file para ok or not!!!");
    if( S_DONE != ret )
    {
         LOG("ERROR: FlashTool_FirmwareUpdate() failed, error code: %s(%d)!", StatusToString(ret), ret);
         THROW_BROM_EXCEPTION(ret, 0);
    }
}

}

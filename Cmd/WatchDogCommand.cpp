#include "WatchDogCommand.h"

#include "../Conn/Connection.h"

#include "../Logger/Log.h"
#include "../Err/Exception.h"

namespace APCore
{

WatchDogCommand::WatchDogCommand(APKey key)
    :ICommand(key)
{
}

WatchDogCommand::~WatchDogCommand()
{
}

void WatchDogCommand::exec(
        const QSharedPointer<APCore::Connection> &conn)
{
    conn->ConnectDA();

    FlashTool_EnableWDT_Arg wdt_arg;

    ArgFlashToolWatchDlg(&wdt_arg);

    int ret = FlashTool_EnableWatchDogTimeout(
                conn->FTHandle(), &wdt_arg);

    if(ret != S_DONE)
    {
        LOGE("FlashTool_EnableWatchDogTimeout(%d) failed!, error code: %s(%d)", wdt_arg.m_timeout_ms, StatusToString(ret), ret);
        THROW_BROM_EXCEPTION(ret, 0);
    }

    LOGI("FlashTool_EnableWatchDogTimeout Succeeded.");
}

void WatchDogCommand::ArgFlashToolWatchDlg(FlashTool_EnableWDT_Arg *wdt_arg)
{
     memset(wdt_arg, 0, sizeof(FlashTool_EnableWDT_Arg));

     wdt_arg->m_timeout_ms = 3000;
     wdt_arg->m_async = _FALSE;
     wdt_arg->m_reboot = _FALSE;
}

}

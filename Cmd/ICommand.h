#ifndef ICOMMAND_H
#define ICOMMAND_H

#include <QSharedPointer>

#include "../Public/Host.h"
#include "../BootRom/flashtoolex_api.h"
#include "../BootRom/type_define.h"
#include "../BootRom/mtk_status.h"

namespace APCore
{

class Connection;
class ICommand
{
public:
    ICommand(APKey key):key_(key){}
    virtual ~ICommand(){}

    virtual void set_da_file(const std::string da){
        da_ = da;
    }

    virtual status_t connect(HSESSION *hs);
    void reboot_device(HSESSION hs);

    virtual void exec(const QSharedPointer<APCore::Connection> &conn) = 0;

protected:
    APKey key_;
    std::string da_;
};

}

#endif // ICOMMAND_H

#ifndef __FW_COMMAND_H__
#define __FW_COMMAND_H__

#include "ICommand.h"
#include "../Conn/Connection.h"

namespace APCore
{

class FWCommand:public ICommand
{
public:
    FWCommand(APKey key);
    ~FWCommand();

    virtual void exec(const QSharedPointer<Connection> &conn);
    void set_fw_file(std::string fw_file)
    {
        m_fw_file = fw_file;
    }

private:
    std::string m_fw_file;
};

}

#endif

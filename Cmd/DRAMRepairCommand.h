#ifndef DRAMREPAIRCOMMAND
#define DRAMREPAIRCOMMAND

#include "ICommand.h"
#include "../Setting/DRAMRepairSetting.h"

namespace APCore
{

class DRAMRepairCommand: public ICommand
{
public:
    DRAMRepairCommand(APKey key);
    virtual ~DRAMRepairCommand();

    // ICommand interface
public:
    virtual void exec(const QSharedPointer<Connection> &conn);

    void set_cb_repair(MEMORY_TEST_REPAIR_CALLBACK cb);

private:
    MEMORY_TEST_REPAIR_CALLBACK m_repair_cb;
};

}

#endif // DRAMREPAIRCOMMAND


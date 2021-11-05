#ifndef DRAMREPAIRSETTING
#define DRAMREPAIRSETTING

#include "ISetting.h"

namespace APCore
{

typedef void (*MEMORY_TEST_REPAIR_CALLBACK)(int);

class DRAMRepairSetting: public ISetting
{
public:
    DRAMRepairSetting();
    virtual ~DRAMRepairSetting();

public:
    // Serializable interface
    virtual void LoadXML(const XML::Node &node);
    virtual void SaveXML(XML::Node &node) const;

    // ISetting interface
    virtual QSharedPointer<ICommand> CreateCommand(APKey key);

    void set_cb_repair(MEMORY_TEST_REPAIR_CALLBACK cb);

private:
    MEMORY_TEST_REPAIR_CALLBACK m_repair_cb;
};

}

#endif // DRAMREPAIRSETTING


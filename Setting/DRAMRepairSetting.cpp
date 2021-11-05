#include "DRAMRepairSetting.h"
#include "../Cmd/DRAMRepairCommand.h"
#include "../XMLParser/XMLNode.h"

namespace APCore
{

DRAMRepairSetting::DRAMRepairSetting():
    ISetting(),
    m_repair_cb(NULL)
{

}

DRAMRepairSetting::~DRAMRepairSetting()
{

}

void DRAMRepairSetting::LoadXML(const XML::Node &node)
{
    LOG("The node name is %s.", node.GetName().c_str());
}

void DRAMRepairSetting::SaveXML(XML::Node &node) const
{
    LOG("The node name is %s.", node.GetName().c_str());
}

QSharedPointer<ICommand> DRAMRepairSetting::CreateCommand(APKey key)
{
    QSharedPointer<APCore::DRAMRepairCommand> cmd(new DRAMRepairCommand(key));
    cmd->set_cb_repair(m_repair_cb);
    return cmd;
}

void DRAMRepairSetting::set_cb_repair(MEMORY_TEST_REPAIR_CALLBACK cb)
{
    m_repair_cb = cb;
}

}

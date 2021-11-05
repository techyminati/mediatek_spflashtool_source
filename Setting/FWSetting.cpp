#include "FWSetting.h"
#include "../Cmd/FWCommand.h"
#include "../XMLParser/XMLNode.h"

namespace APCore
{
FWSetting::FWSetting():
    fw_file_("")
{

}

void FWSetting::LoadXML(const XML::Node &node)
{
    Q_UNUSED(node);
}
void FWSetting::SaveXML(XML::Node &node) const
{
    Q_UNUSED(node);
}

QSharedPointer<APCore::ICommand> FWSetting::CreateCommand(APKey key)
{
    QSharedPointer<APCore::FWCommand> cmd(new APCore::FWCommand(key));
    cmd->set_fw_file(fw_file_);

    return cmd;
}

}

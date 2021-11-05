#include "RSCSetting.h"
#include "../Cmd/RSCCommand.h"
#include "../XMLParser/XMLNode.h"
#include "../Host/Inc/RuntimeMemory.h"

namespace APCore
{

RSCSetting::RSCSetting():
    m_rsc_index(0),
    m_rsc_proj_name(""),
    m_rsc_operator_name("")
{

}

QSharedPointer<APCore::ICommand> RSCSetting::CreateCommand(APKey key)
{
    QSharedPointer<APCore::RSCCommand> cmd(new RSCCommand(key));
    cmd->setRSCIndex(m_rsc_index);
    cmd->setRSCProjectName(m_rsc_proj_name);
    cmd->setRSCOperatorName(m_rsc_operator_name);
    return cmd;
}

void RSCSetting::LoadXML(const XML::Node &node)
{
    LOG("The node name is %s.", node.GetName().c_str());
    XML::Node child = node.GetFirstChildNode();
    while (!child.IsEmpty())
    {
        if (child.GetName() == "rsc-index")
        {
            m_rsc_index = QString::fromStdString(child.GetText()).toUInt();
        }
        else if (child.GetName() == "rsc-proj-name")
        {
            m_rsc_proj_name = child.GetText();
        }
        else if (child.GetName() == "rsc-operator-name")
        {
            m_rsc_operator_name = child.GetText();
        }
        child = child.GetNextSibling();
    }
}

void RSCSetting::SaveXML(XML::Node &node) const
{
    LOG("The node name is %s.", node.GetName().c_str());
    XML::Node parent_node = node.AppendChildNode("rsc-command");
    parent_node.AppendChildNode("rsc-index", QString::number(m_rsc_index).toStdString());
    parent_node.AppendChildNode("rsc-proj-name", m_rsc_proj_name);
    parent_node.AppendChildNode("rsc-operator-name", m_rsc_operator_name);
}

}

#ifndef RSCSETTING_H
#define RSCSETTING_H

#include "ISetting.h"

namespace APCore
{

class RSCSetting : public ISetting
{
public:
    RSCSetting();
    virtual QSharedPointer<APCore::ICommand> CreateCommand(APKey key);

    virtual void LoadXML(const XML::Node &node);
    virtual void SaveXML(XML::Node &node) const;

    unsigned int getRSCIndex() const
    {
        return m_rsc_index;
    }

    void setRSCIndex(unsigned int index)
    {
        m_rsc_index = index;
    }

    void setRSCProjectName(std::string proj_name)
    {
        m_rsc_proj_name = proj_name;
    }

    void setRSCOperatorName(std::string op_name)
    {
        m_rsc_operator_name = op_name;
    }
private:
    unsigned int m_rsc_index;
    std::string m_rsc_proj_name;
    std::string m_rsc_operator_name;
};

}

#endif // RSCSETTING_H

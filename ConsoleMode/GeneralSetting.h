#ifndef GENERALSETTING_H
#define GENERALSETTING_H

#include <QSharedPointer>

#include <iostream>
#include <list>
#include "../Arg/GeneralArg.h"
#include "../BootRom/flashtool.h"
#include "../Cmd/GeneralCommand.h"
#include "../XMLParser/XMLNode.h"
#include "../XMLParser/XMLSerializable.h"
#include "../Setting/ConnSetting.h"
#include "../Setting/ChksumSetting.h"
#include "../Setting/LogInfoSetting.h"
#include "CommandLineArguments.h"
#include "../Setting/ConnBromUSBSetting.h"

namespace ConsoleMode
{

class GeneralSetting : public XML::Serializable
{
public:
    GeneralSetting(APCore::USBPower power = APCore::AutoDetect, bool storage_life_cycle_check = true);
    ~GeneralSetting();

    GeneralSetting(const GeneralSetting& setting);

    GeneralSetting(const XML::Node &node);

    void vSetArgs(const CommandLineArguments& args);

    virtual void LoadXML(const XML::Node &node);
    virtual void SaveXML(XML::Node &node) const;
    void SaveXML_ReadBack_General(XML::Node &root_node) const;

    GeneralSetting *pclClone() const
    {
        return new GeneralSetting(*this);
    }

    QSharedPointer<APCore::ConnSetting> pclGetConnSetting() const
    {
        return m_pclConnSetting;
    }

    void vSetConnSetting(const QSharedPointer<APCore::ConnSetting> setting)
    {
        m_pclConnSetting = setting;
    }

    QSharedPointer<APCore::ChksumSetting> pclGetChksumSetting() const
    {
        return m_pclChksumSetting;
    }

    void SetChksumSetting(const QSharedPointer<APCore::ChksumSetting> setting)
    {
        m_pclChksumSetting = setting;
    }

    QSharedPointer<GeneralArg> pclGetGeneralArg() const
    {
        return m_pclArg;
    }

    void vSetGeneralArg(const QSharedPointer<GeneralArg> arg)
    {
        m_pclArg = arg;
    }

    void SetLogInfoSetting(const QSharedPointer<APCore::LogInfoSetting> setting)
    {
        m_pclLogInfoSetting = setting;
    }

    QSharedPointer<APCore::LogInfoSetting> pclGetLogInfoSetting() const
    {
        return m_pclLogInfoSetting;
    }

    QSharedPointer<GeneralCommand> pclCreateCommand(const QSharedPointer<AppCore>& app, const APKey& key);
    static const QSharedPointer<APCore::LogInfoSetting> getLogInfoSetting(const XML::Node &general_node);
private:
    GeneralSetting& operator=(const GeneralSetting&);
    QSharedPointer<GeneralArg> m_pclArg;
    QSharedPointer<APCore::ConnSetting> m_pclConnSetting;
    QSharedPointer<APCore::ChksumSetting> m_pclChksumSetting;
    QSharedPointer<APCore::LogInfoSetting> m_pclLogInfoSetting;
};

}

#endif // GENERALSETTING_H

#ifndef READBACKWITHOUTSCATTERSETTING_H
#define READBACKWITHOUTSCATTERSETTING_H

#include "ISetting.h"
#include "./BootRom/flashtoolex_struct.h"

namespace APCore
{

class ReadbackWithoutScatterSetting : public ISetting
{
public:
    ReadbackWithoutScatterSetting();
    virtual QSharedPointer<APCore::ICommand> CreateCommand(APKey key);

    virtual void LoadXML(const XML::Node &node);
    virtual void SaveXML(XML::Node &node) const;

    void setReboot(bool reboot);

private:
    std::vector<op_part_list_t> items;
    bool m_reboot;
};

}
#endif // READBACKWITHOUTSCATTERSETTING_H

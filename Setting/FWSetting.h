#ifndef __FWSETTING_H__
#define __FWSETTING_H__

#include "ISetting.h"

namespace APCore
{

class FWSetting: public ISetting
{
public:
    typedef void (__stdcall * void_callback)();
    FWSetting();

    virtual void LoadXML(const XML::Node &node);
    virtual void SaveXML(XML::Node &node) const;

    virtual QSharedPointer<APCore::ICommand> CreateCommand(APKey key);

    void set_fw_file(const std::string &fw_file)
    {
        fw_file_ = fw_file;
    }

private:
    std::string fw_file_;
};

}
#endif

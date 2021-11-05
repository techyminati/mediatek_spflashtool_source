#ifndef CERTDLSETTING_H
#define CERTDLSETTING_H

#include "ISetting.h"

namespace APCore
{

class CertDLSetting : public ISetting
{
public:
    CertDLSetting();

    bool DownloadToStorage(){return m_dl_to_storage_;}
    void SetDLToStorage(bool dltoStorage)
    {
        m_dl_to_storage_ = dltoStorage;
    }

    virtual QSharedPointer<APCore::ICommand> CreateCommand(APKey key);

    virtual void LoadXML(const XML::Node &node);
    virtual void SaveXML(XML::Node &node) const;

private:
    bool m_dl_to_storage_;
};

}

#endif // CERTDLSETTING_H

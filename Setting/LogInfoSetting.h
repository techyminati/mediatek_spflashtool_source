#ifndef LOGINFOSETTING_H
#define LOGINFOSETTING_H
#include <string>
#include <QtGlobal>
#include "../XMLParser/XMLSerializable.h"

namespace APCore
{
class LogInfoSetting: public XML::Serializable
{
public:
    LogInfoSetting();
    LogInfoSetting(const XML::Node &node);
    virtual ~LogInfoSetting();

    virtual void LoadXML(const XML::Node &node);
    virtual void SaveXML(XML::Node &node) const;

    void setLogOn(bool bLogOn);
    void setLogPath(const std::string &sLogPath);
    void setCleanHours(qint64 nCleanHours);

    bool getLogOn() const;
    std::string getLogPath() const;
    qint64 getCleanHours() const;

    static LogInfoSetting *FromXML(const XML::Node &node);
private:
    std::string m_logPath;
    bool m_logOn;
    qint64 m_cleanHours;
};
}

#endif // LOGINFOSETTING_H


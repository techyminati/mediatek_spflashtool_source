#include "LogInfoSetting.h"
#include "../XMLParser/XMLNode.h"
#include "../Utility/Utils.h"

const std::string LOG_INFO_COMMENT("log_on: log switch, enable log if true, otherwise false;\n"
                                   "\t\t\tlog_path: the directory in which the log files has been stored;\n"
                                   "\t\t\tclean_hours: the time setting to delete log files regularly, the unit is hours.");

APCore::LogInfoSetting::LogInfoSetting(): XML::Serializable(),
    m_logOn(true), m_cleanHours(1), m_logPath(Utils::GetSpecifalFolderPath())
{

}

APCore::LogInfoSetting::LogInfoSetting(const XML::Node &node): XML::Serializable(),
  m_logOn(true), m_cleanHours(1), m_logPath(Utils::GetSpecifalFolderPath())
{
    LoadXML(node);
}

APCore::LogInfoSetting::~LogInfoSetting()
{

}

void APCore::LogInfoSetting::LoadXML(const XML::Node &node)
{
    Q_ASSERT(node.GetName() == "log-info");
    std::string sLogOn = node.GetAttribute("log_on");
    m_logOn = QString::fromStdString(sLogOn).toLower().trimmed() == "true" ? true : false;
    m_logPath = node.GetAttribute("log_path");
    m_cleanHours = atol(node.GetAttribute("clean_hours").c_str());
}

void APCore::LogInfoSetting::SaveXML(XML::Node &node) const
{
    XML::Node commentChildNode = node.AppendCommentChildNode();
    commentChildNode.SetText(LOG_INFO_COMMENT);
    XML::Node childNode = node.AppendChildNode("log-info");
    childNode.SetAttribute("log_on", m_logOn ? "true" : "false");
    childNode.SetAttribute("log_path", m_logPath);
    childNode.SetAttribute("clean_hours", QString::number(m_cleanHours).toStdString());
}

void APCore::LogInfoSetting::setLogOn(bool bLogOn)
{
    m_logOn = bLogOn;
}

void APCore::LogInfoSetting::setLogPath(const std::string &sLogPath)
{
    m_logPath = sLogPath;
}

void APCore::LogInfoSetting::setCleanHours(qint64 nCleanHours)
{
    m_cleanHours = nCleanHours;
}

bool APCore::LogInfoSetting::getLogOn() const
{
    return m_logOn;
}

std::string APCore::LogInfoSetting::getLogPath() const
{
    return m_logPath;
}

qint64 APCore::LogInfoSetting::getCleanHours() const
{
    return m_cleanHours;
}

APCore::LogInfoSetting *APCore::LogInfoSetting::FromXML(const XML::Node &node)
{
    return new LogInfoSetting(node);
}

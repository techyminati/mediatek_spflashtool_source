#ifndef COMMANDSETTINGVALIDATOR_H
#define COMMANDSETTINGVALIDATOR_H

#include "Config.h"
#include "CommandLineArguments.h"

namespace ConsoleMode
{

typedef enum RSC_EXIST_STATUS
{
    res_rsc_not_exist = 0,
    res_rsc_exist,
    res_rsc_not_need
} RSC_EXIST_STATUS_T;

class CommandSettingValidator
{
public:
    CommandSettingValidator(const Config &config, const CommandLineArguments& args);
    virtual ~CommandSettingValidator();

    bool Validate() const;

    bool needRSCSetting() const;

private:
    bool ValidateRSCSetting() const;

    RSC_EXIST_STATUS_T checkRSCFileStatus() const;
    QString getRSCFileName() const;
    bool validRSCIndex() const;

private:
    const Config &m_config;
    const CommandLineArguments& m_args;
};
}

#endif // COMMANDSETTINGVALIDATOR_H


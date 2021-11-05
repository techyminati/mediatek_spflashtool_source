#ifndef CONSOLEMODEENTRY_H
#define CONSOLEMODEENTRY_H

#include "../Public/AppCore.h"
#include "../Utility/LogFilesClean.h"

namespace ConsoleMode
{

class ConsoleModeEntry
{
public:
    ConsoleModeEntry();
    ~ConsoleModeEntry();

    int Run(int argc, char *argv[]);

private:
    ConsoleModeEntry(const ConsoleModeEntry &rhs);
    ConsoleModeEntry & operator=(const ConsoleModeEntry &rhs);
    void CheckNewVersion();
    void CleanLogFiles(const std::string &log_path, qint64 clean_hours);

private:
    LogCleanThread *m_cleanThread;
};

}

#endif // CONSOLEMODEENTRY_H

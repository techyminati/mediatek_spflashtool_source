#ifndef WRITEMEMORYCOMMAND_H
#define WRITEMEMORYCOMMAND_H

#include "ICommand.h"
#include "../Conn/Connection.h"
#include "../Arg/WriteMemoryArg.h"

namespace APCore
{
class WriteMemoryCommand:public ICommand
{
public:
     typedef void (__stdcall * void_callback)();

    WriteMemoryCommand(APKey key);
    ~WriteMemoryCommand();

    virtual void exec(const QSharedPointer<Connection> &conn);
    void set_write_memory_init(void_callback cb)
    {
          this->cb_write_memory_init = cb;
    }

    void set_is_by_sram(bool by_sram)
    {
        is_by_sram_ = by_sram;
    }

    bool is_by_sram(){return is_by_sram_;}

    void set_is_scatter_ver2(bool is_scatter_ver2)
    {
        m_is_scatter_ver2 = is_scatter_ver2;
    }

    static int __stdcall cb_write_memory_init_console();
private:
    friend class WriteMemorySetting;
    WriteMemoryArg wm_arg_;
    void_callback cb_write_memory_init;
    bool is_by_sram_;
    bool m_is_scatter_ver2;
};
}

#endif // WRITEMEMORYCOMMAND_H

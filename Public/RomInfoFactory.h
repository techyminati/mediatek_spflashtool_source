#ifndef ROMINFOFACTORY_H
#define ROMINFOFACTORY_H
#include "Host.h"
#include "../BootRom/flashtool_handle.h"

#include <list>
#include <QSharedPointer>

namespace APCore {
    class Connection;
}

class RomInfoFactory
{
public:
    RomInfoFactory(APKey key);

    int GetRomList(std::list<ROM_INFO> &rom_info_list,
                   _BOOL bWithPreloader= _FALSE);

    int GetPartitionList(const QSharedPointer<APCore::Connection> &conn,
        std::list<PART_INFO> &partition_info_list,
            _BOOL bWithPreloader= _FALSE);

    int GetScatterRomList(std::list<ROM_INFO> &rom_info_list);

private:
    APKey key_;

};

#endif // ROMINFOFACTORY_H

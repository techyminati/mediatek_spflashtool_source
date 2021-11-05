#ifndef ROMFACTORY_H
#define ROMFACTORY_H

#include "fw_defs.h"
#include <QSharedPointer>

class IRomBase;

#define RomBaseList std::list<QSharedPointer<IRomBase> >

class CRomFactory
{
public:
    CRomFactory(RomBaseList &rom_list, bool format_whole = true, bool isSec11 = false);

    ERROR_T produce(const ROM_INFO&, Download_Scene);

private:
    ERROR_T CreateCompositeBL(const ROM_INFO&);
    ERROR_T CreateMT6582CompositeBL(const ROM_INFO&);

    ERROR_T CreateInvisibleRom(const ROM_INFO&, Download_Scene);

private:
    RomBaseList &rom_list_;
    IRomBase *composite_rom_;
    bool format_whole_flash_;
    bool is_security_11_;
};

#endif // ROMFACTORY_H

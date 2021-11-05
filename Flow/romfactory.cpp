#include <list>
#include "../Public/AppCore.h"
#include "romfactory.h"
#include "concreterom.h"
#include "../Logger/Log.h"
#include "../Host/Inc/RuntimeMemory.h"

#define SHR_ROM_PTR(p) QSharedPointer<IRomBase>(p)

CRomFactory::CRomFactory(RomBaseList &rom_list, bool format_whole, bool isSec11)
    : rom_list_(rom_list), composite_rom_(NULL),
      format_whole_flash_(format_whole),
      is_security_11_(isSec11)
{
}

ERROR_T CRomFactory::produce(const ROM_INFO &rom,
                             Download_Scene scene)
{
    //if USE security1.1 (old arch chip), skip "seccfg" partition
    if(is_security_11_ && stricmp(rom.name, "seccfg")==0)
    {
        LOGI("skip \"seccfg\" partition process");
        return ERROR_OK;
    }

    switch (rom.operation_type)
    {
    case OPER_BOOTLOADERS:
        // if AB system, loader_ext_b filename is null, not enalbe, DO NOT produce it
        // otherwise will make fw error: "loader_ext_a, loader_ext_b should select together"
        if(std::string(rom.filepath) == "" && rom.enable == _FALSE)
            return ERROR_OK;

        if(format_whole_flash_)
            return CreateCompositeBL(rom);
        else
            return CreateMT6582CompositeBL(rom);

    case OPER_INVISIBLE:
        return CreateInvisibleRom(rom, scene);

    case OPER_BINREGION:
    //    rom_list_.push_back(SHR_ROM_PTR(new CBinRegionRom(rom)));
    //    break;

    case OPER_PROTECTED:
        rom_list_.push_back(SHR_ROM_PTR(new CProtectedRom(rom)));
        break;

    case OPER_RESERVED:
        // nothing to do
        break;

    case OPER_UPDATE:
        rom_list_.push_back(SHR_ROM_PTR(new CDefaultRom(rom)));
        break;

    default:
        return ERROR_UNKNOWN_ROM;
    }
    return ERROR_OK;
}

ERROR_T CRomFactory::CreateInvisibleRom(const ROM_INFO &rom,
                                        Download_Scene scene)
{
    ROM_INFO tmp = rom;

    if (FORMAT_ALL_DOWNLOAD == scene ||
        FIRMWARE_UPGRADE == scene)
    {
        tmp.enable = _TRUE;     // to format
    }
    else
    {
        tmp.enable = _FALSE;    // not to touch it
    }

    rom_list_.push_back(SHR_ROM_PTR(new CInvisibleRom(tmp)));

    return ERROR_OK;
}

ERROR_T CRomFactory::CreateCompositeBL(const ROM_INFO &rom)
{
    CCompositeBLRom *p =
            static_cast<CCompositeBLRom*>(composite_rom_);

    if (p == NULL)
    {
        p = new CCompositeBLRom;
        rom_list_.push_back(SHR_ROM_PTR(p));
        composite_rom_ = p;
    }

    if (!p->attach(rom))
    {
        return ERROR_UNKNOWN_ROM;
    }
    else
    {
        return ERROR_OK;
    }
}

ERROR_T CRomFactory::CreateMT6582CompositeBL(const ROM_INFO &rom)
{
    CMT6582CompositeBLRom *p =
            static_cast<CMT6582CompositeBLRom*>(composite_rom_);

    if (p == NULL)
    {
        p = new CMT6582CompositeBLRom();
        rom_list_.push_back(SHR_ROM_PTR(p));
        composite_rom_ = p;
    }

    if (!p->attach(rom))
    {
        return ERROR_UNKNOWN_ROM;
    }
    else
    {
        return ERROR_OK;
    }
}

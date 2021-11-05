#include "../Utility/Utils.h"
#include "../Public/Host.h"
#include "../Err/Exception.h"
#include "../Logger/Log.h"
#include "../Host/Inc/RuntimeMemory.h"
#include "IniItem.h"
#include "FileUtils.h"
#include "version.h"
#include <QRegExp>
#include <QTime>
#include <iostream>
#include <algorithm>
#include <stdio.h>
#include <QSettings>
#include <QPainter>
#include <QTextStream>
#include <QMessageBox>
#include <QApplication>
#include <QAbstractButton>

#ifdef _WIN32
#include <Windows.h>
#include <MAPI.h>
#include <vector>
#include <TlHelp32.h>
#include <wchar.h>
#endif

const static std::map<QString, U32>::value_type INIT_VALUE[]=
{
    std::map<QString, U32>::value_type("EMMC_BOOT_1", EMMC_PART_BOOT1),
    std::map<QString, U32>::value_type("EMMC_BOOT_2", EMMC_PART_BOOT2),
    std::map<QString, U32>::value_type("EMMC_RPMB", EMMC_PART_RPMB),
    std::map<QString, U32>::value_type("EMMC_USER", EMMC_PART_USER),
    std::map<QString, U32>::value_type("UFS_LU0", UFS_PART_LU0),
    std::map<QString, U32>::value_type("UFS_LU1", UFS_PART_LU1),
    std::map<QString, U32>::value_type("UFS_LU2", UFS_PART_LU2),
    std::map<QString, U32>::value_type("UFS_LU0_LU1", UFS_PART_LU0_LU1),
    std::map<QString, U32>::value_type("BOOT_1", BOOT1_PART),
    std::map<QString, U32>::value_type("BOOT_2", BOOT2_PART),
    std::map<QString, U32>::value_type("USER", USER_PART),
    std::map<QString, U32>::value_type("EMMC_BOOT1_BOOT2", EMMC_BOOT1_BOOT2)
};
const static std::map<QString, U32> REGION_MAP(INIT_VALUE, INIT_VALUE+12);

std::map<unsigned int, std::string>romstring_map;

QString EnumToRomString(HW_StorageType_E storage_,  U32 id, bool scatter_ver2_readBack /*= false*/)
{
    LOG("EnumToRomString: storage_(%d),region_id(%d) ",storage_, id);
    if(HW_STORAGE_EMMC == storage_)
    {
        switch(id)
        {
        case EMMC_PART_BOOT1:
            return scatter_ver2_readBack? "BOOT_1": "EMMC_BOOT_1";

        case EMMC_PART_BOOT2:
            return scatter_ver2_readBack? "BOOT_2": "EMMC_BOOT_2";

        case EMMC_PART_RPMB:
            return "EMMC_RPMB";

        case EMMC_PART_GP1:
            return "EMMC_GP1";

        case EMMC_PART_GP2:
            return "EMMC_GP2";

        case EMMC_PART_GP3:
            return "EMMC_GP3";

        case EMMC_PART_GP4:
            return "EMMC_GP4";

        case EMMC_PART_USER:
            return scatter_ver2_readBack? "USER": "EMMC_USER";

        case EMMC_PART_BOOT1_BOOT2:
            return scatter_ver2_readBack? "BOOT_1": "EMMC_BOOT1_BOOT2";
        }
    }
    else if (HW_STORAGE_UFS == storage_)
    {
        switch(id)
        {
        case UFS_PART_LU0:
            return scatter_ver2_readBack? "BOOT_1": "UFS_LU0";

        case UFS_PART_LU1:
            return scatter_ver2_readBack? "BOOT_2": "UFS_LU1";

        case UFS_PART_LU2:
            return scatter_ver2_readBack? "USER": "UFS_LU2";

        case UFS_PART_LU0_LU1:
            return scatter_ver2_readBack? "BOOT_1": "UFS_LU0_LU1";
        }
    }

    return "";
}

std::string DownloadScenetEnumToString(Download_Scene scene)
{
    switch(scene) {
        ENUM_TO_STRING(FORMAT_ALL_DOWNLOAD)
        ENUM_TO_STRING(FIRMWARE_UPGRADE)
        ENUM_TO_STRING(DOWNLOAD_ONLY)
        ENUM_TO_STRING(WIPE_DATA)
        default:
            return "??";
        }
}

std::string EfuseExtraKeyToString(EFUSE_KEY key)
{
    switch(key) {
    ENUM_TO_STRING(C_LOCK)
    ENUM_TO_STRING(C_CTRL3_LOCK)
    ENUM_TO_STRING(C_CTRL2_LOCK)
    ENUM_TO_STRING(C_CTRL1_LOCK)
    ENUM_TO_STRING(C_CTRL0_LOCK)
    ENUM_TO_STRING(C_DATA0_1_LOCK)
    ENUM_TO_STRING(C_DATA0_LOCK)
    ENUM_TO_STRING(C_DATA1_LOCK)
    ENUM_TO_STRING(C_DATA2_LOCK)
    ENUM_TO_STRING(C_DATA3_LOCK)
    ENUM_TO_STRING(C_DATA4_LOCK)
    ENUM_TO_STRING(C_DATA5_LOCK)
    ENUM_TO_STRING(CTRL1)
    ENUM_TO_STRING(SEC_CTRL1)
    ENUM_TO_STRING(C_CTRL_0)
    ENUM_TO_STRING(C_CTRL_1)
    ENUM_TO_STRING(C_CTRL_2)
    ENUM_TO_STRING(C_CTRL_3)
    ENUM_TO_STRING(C_DATA_0)
    ENUM_TO_STRING(C_DATA_1)
    ENUM_TO_STRING(C_DATA_2)
    ENUM_TO_STRING(C_DATA_3)
    ENUM_TO_STRING(C_DATA_4)
    ENUM_TO_STRING(C_DATA_5)
    ENUM_TO_STRING(SEC_CAP)
    ENUM_TO_STRING(PROD_EN)
    ENUM_TO_STRING(CUSTK)
    ENUM_TO_STRING(C_3P_PID_LOCK)
    ENUM_TO_STRING(C_3P_EPPK_LOCK)
    ENUM_TO_STRING(C_3P_CPD_LOCK)
    ENUM_TO_STRING(C_3P_OID_LOCK)
    ENUM_TO_STRING(C_3P_SV0_KEY_LOCK)
    ENUM_TO_STRING(C_3P_SV1_KEY_LOCK)
    ENUM_TO_STRING(C_3P_JTAG_UNLOCK_KEY_LOCK)
    ENUM_TO_STRING(C_3P_RSA_PUBK_LOCK0)
    ENUM_TO_STRING(C_3P_RSA_PUBK_LOCK1)
    ENUM_TO_STRING(C_3P_RSA_PUBK_LOCK2)
    ENUM_TO_STRING(C_3P_RSA_PUBK_LOCK3)
    ENUM_TO_STRING(C_3P_RSA_PUBK_LOCK4)
    ENUM_TO_STRING(C_3P_RSA_PUBK_LOCK5)
    ENUM_TO_STRING(C_3P_RSA_PUBK_LOCK6)
    ENUM_TO_STRING(C_3P_RSA_PUBK_LOCK7)
    ENUM_TO_STRING(C_3P_RSA_PUBK_LOCK_LOCK)
    ENUM_TO_STRING(C_3P_DATA_LOCK_LOCK)
    ENUM_TO_STRING(CUST_CRYPT_DATA)
    ENUM_TO_STRING(CUST_DATA)
    ENUM_TO_STRING(C_3P_PID)
    ENUM_TO_STRING(C_3P_EPPK)
    ENUM_TO_STRING(C_3P_CPD)
    ENUM_TO_STRING(C_3P_OID)
    ENUM_TO_STRING(C_3P_SV0_KEY)
    ENUM_TO_STRING(C_3P_SV1_KEY)
    ENUM_TO_STRING(C_3P_JTAG_UNLOCK_KEY)
    ENUM_TO_STRING(C_RSA_PUBK0)
    ENUM_TO_STRING(C_RSA_PUBK1)
    ENUM_TO_STRING(C_RSA_PUBK2)
    ENUM_TO_STRING(C_RSA_PUBK3)
    ENUM_TO_STRING(C_RSA_PUBK4)
    ENUM_TO_STRING(C_RSA_PUBK5)
    ENUM_TO_STRING(C_RSA_PUBK6)
    ENUM_TO_STRING(C_RSA_PUBK7)
    ENUM_TO_STRING(C_SW_VER0)
    ENUM_TO_STRING(C_SW_VER1)
    ENUM_TO_STRING(C_SW_VER2)
    ENUM_TO_STRING(C_SW_VER3)
    ENUM_TO_STRING(C_SW_VER_LOCK)
    ENUM_TO_STRING(C_SW_VER_LOCK_LOCK)
    default:
        return "??";
    }
}
/*
EMMC_Part_E GetPartID(int index)
{
    EMMC_Part_E ret = EMMC_PART_UNKNOWN;

    switch(index)
    {
      case 0:
        ret = EMMC_PART_BOOT1;
        break;

    case 1:
        ret = EMMC_PART_BOOT2;
        break;

    case 2:
        ret = EMMC_PART_RPMB;
        break;

    case 3:
        ret = EMMC_PART_USER;
        break;
    }

    return ret;
}
*/
int Utils::GetPortNumFromStr(const QString& str)
{
    QRegExp regex("COM(\\d+)");
    if(regex.indexIn(str)!= -1){
        QString port = regex.cap(1);
        return port.toInt();
    }
    return 0;
}

U64 Utils::ConvertStrToAddr(const QString addr_str)
{
    bool convert_ok = true;

    U64 addr = addr_str.toULongLong(&convert_ok,16);

    return convert_ok?addr:0;
}

QString Utils::GetAddrFromIniFile(const BBCHIP_TYPE  bbchip, const QString &file_path)
{
    IniItem item("BromAdapterTool.ini", "ETT_START_ADDRESS_", "bbchip");

    for(int i = 0; i < MAX_SUPPORT_BB; i++)
    {
        QString sectionName = QString("ETT_START_ADDRESS_%1").arg(QString::number(i));

        item.SetSectionName(sectionName);

        QString chip = item.GetStringValue().toLower();
        QString path = file_path.toLower();

        if((path.length() > 0 && path.indexOf(chip) >= 0)
                || BBChipTypeToName(bbchip) == chip)
        {
            item.SetItemName("address");

            return item.GetStringValue();
        }
    }
    return QString();
}

QString Utils::GetPlatformFromScatter(const QString& scatter_file)
{
    QString short_name = ExtractFileFromPath(scatter_file);

    QRegExp regex("[m|M][t|T](\\d{4})");
    if(regex.indexIn(short_name)!= -1){
        QString platform_id = regex.cap(1);
        LOG("get platform from scatter: MT%s",platform_id.toAscii().constData());
        return "MT"+platform_id;
    }
    LOG("[Error]get platform from scatter failed!(%s)", scatter_file.toAscii().constData());
    return QString::null;
}

HW_StorageType_E Utils::GetStorageTypeFromScatter(const QString& scatter_file)
{
    QString short_name = ExtractFileFromPath(scatter_file);

    int emmc_pos = short_name.indexOf("EMMC",0,Qt::CaseInsensitive);
    if(emmc_pos != -1)
    {
        return HW_STORAGE_EMMC;
    }

    int sdmmc_pos = short_name.indexOf("SDMMC",0,Qt::CaseInsensitive);
    if(sdmmc_pos != -1)
    {
        return HW_STORAGE_SDMMC;
    }

    int nor_pos = short_name.indexOf("NOR",0,Qt::CaseInsensitive);
    if(nor_pos != -1)
    {
        return HW_STORAGE_NOR;
    }

    int ufs_pos = short_name.indexOf("UFS",0,Qt::CaseInsensitive);
    if(ufs_pos != -1)
    {
        return HW_STORAGE_UFS;
    }

    return HW_STORAGE_NAND;
}


bool Utils::IsFoundDLImageByScatterFile(DL_HANDLE_T p_dl_handle,
                                   ROM_INFO *p_rom_info,
                                        const std::string &rom_name)
{
    Q_ASSERT((p_dl_handle != NULL) && "IsFindDLImage(): p_dl_handle is NULL!");
    Q_ASSERT((p_rom_info != NULL) && "IsFindDLImage(): p_rom_info is NULL!");
    Q_ASSERT((!rom_name.empty()) && "IsFindDLImage(): rom_name is empty!");

    LOG("IsFoundDLImageByScatterFile(): image to be found is (%s).", rom_name.c_str());
    int ret = 0;
    unsigned short count = 0;
    bool is_found = false;
    if (S_DONE == (ret = DL_GetCount(p_dl_handle, &count)))
    {
        ROM_INFO rom[MAX_LOAD_SECTIONS];
        if (S_DONE == (ret = DL_Rom_GetInfoAll(p_dl_handle, rom, MAX_LOAD_SECTIONS)))
        {
            for (int i = 0; i < count; i++)
            {
                if( 0 == rom_name.compare(rom[i].name) )
                {
                    memcpy(p_rom_info, (rom + i), sizeof(ROM_INFO));
                    LOG("IsFoundDLImageByScatterFile(): \
                        Original rom info: rom name(%s), begin_addr(0x%x), \
                        image info: image name(%s), begin_addr(0x%x).",
                        rom[i].name, rom[i].begin_addr,
                        p_rom_info->name, p_rom_info->begin_addr);
                    is_found = true;
                    break;
                }
            }
        }
        else
        {
            LOG("ERROR: IsFindDLImage(): DL_Rom_GetInfoAll() failed, brom error code: %s(%d)!", StatusToString(ret), ret);
            return is_found;
        }
    }
    else
    {
        LOG("ERROR: IsFindDLImage(): DL_GetCount() failed, brom error code: %s(%d)!", StatusToString(ret), ret);
        return is_found;
    }

    return is_found;
}

int Utils::GetPartitionInfo(const FLASHTOOL_API_HANDLE_T &flashtool_handle,
                            const std::string &part_name,
                            PART_INFO &part_info)
{
    Q_ASSERT( NULL != flashtool_handle );
    int ret;
    unsigned int rom_count = 0;
    bool isImgNameMatch = false;
    PART_INFO *pPartInfo = NULL;

    do {
        ret = FlashTool_ReadPartitionCount(flashtool_handle, &rom_count);
        if(S_DONE != ret) {
            LOG("ERROR: FlashTool_ReadPartitionCount fail, errorcode is %s(%d).", StatusToString(ret), ret);
            break;
        }
        pPartInfo = new PART_INFO[rom_count];

        ret = FlashTool_ReadPartitionInfo(flashtool_handle, pPartInfo, rom_count);
        if(S_DONE != ret) {
            LOG("ERROR: FlashTool_ReadPartitionInfo fail!, errorcode is %s(%d)", StatusToString(ret), ret);
            break;
        }

        std::string rom_name;
        for(unsigned int i = 0; i < rom_count; i++){
            rom_name = pPartInfo[i].name;
            if(ToUppercaseString(rom_name) == part_name){
                part_info.begin_addr= pPartInfo[i].begin_addr;
                part_info.image_length = pPartInfo[i].image_length;
                isImgNameMatch = true;
                break;
            }
        }

        if(!isImgNameMatch){
            LOG("ERROR: Image that is wanted to read back does not be found!");
            ret = S_FT_READBACK_FAIL;
            break;
        }
    }while(0);

    if(NULL != pPartInfo){
        delete[] pPartInfo;
        pPartInfo = NULL;
    }

    return ret;
}

/**
 * Return string to uppercase
 *
 * @param std::string or char*
 *
 * @return std::string with all characters are uppercase
 */
//template <typename T>
std::string ToUppercaseString(const std::string& str)
{
    std::string tmpstr(str);
    std::transform(tmpstr.begin(), tmpstr.end(), tmpstr.begin(), toupper);
    return tmpstr;
}

std::string ToLowercaseString(const std::string& str)
{
    std::string tmpstr(str);
    std::transform(tmpstr.begin(), tmpstr.end(), tmpstr.begin(), tolower);
    return tmpstr;
}

#if 0
// this func is not implemented right!
std::string WStr2Str(const std::wstring& ws)
{
    std::string temp(ws.length(), '\0');
    std::copy(ws.begin(), ws.end(), temp.begin());
    return temp;
}
#endif

std::wstring Str2WStr(const std::string& s)
{
    const size_t newsize = (s.size())*(sizeof(wchar_t)/sizeof(char)) + 1;
    wchar_t* _dest = NULL;
    int ret = 0;
    std::wstring wcString;

    try {
        _dest = new wchar_t[newsize];
        wmemset(_dest, 0, newsize);
        ret = mbstowcs(_dest, s.c_str(), newsize);
        if ( -1 == ret) {
             LOG("Str2WStr(): mbstowcs() failed!");
        }
        wcString = _dest;
    } catch (std::exception& ex)    {
        LOG("Str2WStr(): Standard exception: %s!", ex.what());
        if (NULL != _dest) {
            delete[] _dest;
            _dest = NULL;
        }
    }

    if (NULL != _dest) {
        delete[] _dest;
        _dest = NULL;
    }

    return wcString;
}

std::string Utils::ReplaceAllSubstring(const std::string &ori_str, const std::string &sub_str, const std::string &new_sub_str)
{
    std::string temp(ori_str);
    size_t pos(0);
    size_t len_of_sub_str(sub_str.length());
    size_t len_of_new_sub_str(new_sub_str.length());
    while(1) {
        pos = temp.find(sub_str, pos);
        if (pos == std::string::npos) {
            break;
        } else {
            temp.replace(pos, len_of_sub_str, new_sub_str);
            pos += len_of_new_sub_str;
        }
    }
    return temp;
}

std::string Utils::PrettyByteString(U64 bytes)
{
    size_t i = 0;
    char buf[32];
    const char unit[] = "BKMGT";

    // potential overflow!
    bytes *= 100;

    while (bytes >= 1000*100 && i<sizeof(unit)-1)
    {
        bytes /= 1024;
        ++ i;
    }

    sprintf(buf, "%d.%02d%c",
            (int)bytes/100,
            (int)bytes%100,
            (char)unit[i]);

    return buf;
}

QString Utils::ULLToHex(U64 src, unsigned int size)
{
     char buf[24];

     memset(buf, 0, sizeof(buf));

     #ifdef _WIN32
       _snprintf(buf, size, "0x%016I64x", src);
     #else
       snprintf(buf, size, "0x%016llx", src);
     #endif

     return QString::fromAscii(buf);
}

int Utils::GetRomFilesTotalSize(DL_HANDLE_T &dl_handle, U64 *file_size)
{
     U64 file_size_temp(0);
     int ret = S_DONE;
     ROM_INFO roms_info[MAX_LOAD_SECTIONS];

     unsigned short rom_count(0);
     ret = DL_GetCount(dl_handle, &rom_count);

     if(ret != S_DONE)
     {
          LOGE("Get rom count failed, error code: %s(%d).", StatusToString(ret), ret);
          return ret;
     }

     ret = DL_Rom_GetInfoAll(dl_handle, roms_info, MAX_LOAD_SECTIONS);
     if(ret != S_DONE)
     {
          LOGE("Get Rom Info failed, error code: %s(%d).", StatusToString(ret), ret);
          return ret;
     }

     for(int i =0; i < rom_count; i++)
     {
          if(roms_info[i].enable)
          {
               file_size_temp += roms_info[i].filesize;
          }
     }

     (*file_size) = file_size_temp;

     LOGD("Total size should be download is %sB.", PrettyByteString(*file_size).c_str());

     return S_DONE;

}

const QString Utils::ExtClockToName(const EXT_CLOCK  ext_clock) {
    switch(ext_clock)
    {
    case EXT_13M:
        return "EXT_13M";

    case EXT_26M:
        return QString("EXT_26M");

    case EXT_39M:
        return "EXT_39M";

    case EXT_52M:
        return "EXT_52M";

    case AUTO_DETECT_EXT_CLOCK:
        return "AUTO_DETECT_EXT_CLOCK";

    case UNKNOWN_EXT_CLOCK:
        return "UNKNOWN_EXT_CLOCK";

    default:
        return "??";
    }
}

const QString Utils::ramType(HW_RAMType_E type)
{
    switch (type)
    {
    case HW_RAM_SRAM:
        return "SRAM";

    case HW_RAM_DRAM:
        return "DRAM";

    default:
        return QString("Unknown");
    }
}

std::string Utils::GetSpecifalFolderPath()
{
#ifdef _WIN32
    QSettings reg("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                       QSettings::NativeFormat);
    QString commonAppDir = reg.value("Common AppData").toString();



    if (!commonAppDir.isEmpty()) {
        return commonAppDir.toStdString() + "\\SP_FT_Logs";
    } else {
        return ABS_PATH("SP_FT_Logs");
    }
#else
    return "/tmp/SP_FT_Logs";
#endif
}

QImage Utils::AddStringToImage(const QString& imagePath, const QString& str, const QString& owner)
{
    QImage image = QPixmap(imagePath).toImage();

    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);


    QPen pen = painter.pen();
    QColor color;
    color.setRgb(237, 145,33);
    pen.setColor(color);
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(35);

    painter.setPen(pen);
    painter.setFont(font);

    painter.drawText(image.rect(), Qt::AlignCenter, str);

    font.setBold(false);
    font.setPixelSize(20);
    painter.setFont(font);

    painter.drawText(5, image.height()-100, owner);

    return image;
}

bool Utils::parserJSONFile(const QString filePath, QtJson::JsonObject &jsonObject)
{
    QFile file(filePath);
    QString json;
    if(!file.open(QFile::ReadOnly | QFile::Text))
        json = "";
    else
    {
        QTextStream in(&file);
        json = in.readAll();
    }

    if(json.isEmpty()){
        LOGE("Could not read json file %s\n", filePath.toLocal8Bit().constData());
        return false;
    }

    bool ok = false;
    jsonObject = QtJson::parse(json, ok).toMap();

    if(!ok){
        LOGE("An error occured during parsing json file %s\n", filePath.toLocal8Bit().constData());
    }

    return ok;
}

void grabWindow()
{
    QFile file("c:\\errorImage.jpg");
    if (file.exists()){
        file.remove();
    }

    WId winID = QApplication::activeWindow()->winId();
    QPixmap map = QPixmap::grabWindow(winID);
    if(!map.save("c:\\errorImage.jpg"))
        LOGE("grabWindow Fail!");
}

static int message_box_information(QWidget *parent,
                                    const QString &title,
                                    const QString &errMsg,
                                    const QString &btn0_txt)
{
    return QMessageBox::information(parent, title, errMsg, btn0_txt);
}

static int message_box_warning(QWidget *parent,
                               QAbstractButton** pConnectButton,
                               const QString &title,
                               const QString &errMsg,
                               const QString &btn0_txt,
                               const QString &btn1_txt,
                               const QString &btn2_txt,
                               bool report)
{
    int ret = -1;

#ifdef _WIN32
    if(report && !ToolInfo::IsCustomerVer())
    {
        QMessageBox warning_box(QMessageBox::Warning,
                                title,
                                errMsg,
                                QMessageBox::NoButton,
                                parent);
        warning_box.addButton(btn0_txt, QMessageBox::AcceptRole);
        if (!btn1_txt.trimmed().isEmpty())
        {
            warning_box.addButton(btn1_txt, QMessageBox::HelpRole);
        }
        if (!btn2_txt.trimmed().isEmpty())
        {
            warning_box.addButton(btn2_txt, QMessageBox::ActionRole);
        }

        if(pConnectButton && !btn1_txt.trimmed().isEmpty() && !btn2_txt.trimmed().isEmpty())
        {
            *pConnectButton = warning_box.buttons()[2];
            if(*pConnectButton)
            {
                (*pConnectButton)->installEventFilter(parent);
            }
        }

        ret = warning_box.exec();

    }
    else
        ret =  QMessageBox::warning(parent,
                                    title,
                                    errMsg,
                                    btn0_txt, //ok
                                    btn1_txt); //help
#else
    ret =  QMessageBox::warning(parent,
                                title,
                                errMsg,
                                btn0_txt, //ok
                                btn1_txt); //help
#endif

    return ret;
}

static int message_box_critical(QWidget *parent,
                     QAbstractButton** pConnectButton,
                     const QString &title,
                     const QString &errMsg,
                     const QString &btn0_txt,
                     const QString &btn1_txt)
{
    int ret = -1;

#ifdef _WIN32
    if(!ToolInfo::IsCustomerVer())
    {
        QMessageBox critical_box(QMessageBox::Critical,
                                title,
                                errMsg,
                                QMessageBox::NoButton,
                                parent);
        critical_box.addButton(btn0_txt, QMessageBox::AcceptRole);
        if (!btn1_txt.trimmed().isEmpty())
        {
            critical_box.addButton(btn1_txt, QMessageBox::ActionRole);
        }

        if(pConnectButton && !btn1_txt.trimmed().isEmpty())
        {
            *pConnectButton = critical_box.buttons()[1];
            if(*pConnectButton)
            {
                (*pConnectButton)->installEventFilter(parent);
            }
        }

        ret = critical_box.exec();
    }
    else
        ret =  QMessageBox::critical(parent,
                                    title,
                                    errMsg,
                                    btn0_txt); //ok

#else
    ret =  QMessageBox::critical(parent,
                                title,
                                errMsg,
                                btn0_txt); //ok
#endif

    return ret;
}

static int message_box_question(QWidget *parent,
                     const QString &title,
                     const QString &errMsg,
                     const QString &btn0_txt,
                     const QString &btn1_txt)
{
    return QMessageBox::question(parent,
                                 title,
                                 errMsg,
                                 btn0_txt,  //yes, ok
                                 btn1_txt); //no, cancel
}

#ifdef _WIN32

std::vector<std::string> getLogFile(){
    int i = 0;
    QString log = QString::fromLocal8Bit(Logger::GetSPFlashToolLogFolder().c_str());
    QDir dir(log);
    QStringList nameFilter("SP_FT_Dump_*");
    dir.setNameFilters(nameFilter);
    dir.setSorting(QDir::Reversed | QDir::Name);
    QString logDir = dir.entryList().first();

    std::vector<std::string> fileList;

    logDir = log.append("\\").append(logDir);

    LOGI(logDir.toStdString().c_str());

    QDir subDir(logDir);
    QFileInfoList file_list = subDir.entryInfoList(QDir::Files);

    for(; i < file_list.size(); i++){
        QString name = file_list.at(i).absoluteFilePath();
        LOGI(name.toStdString().c_str());
        fileList.push_back(name.toStdString());
    }

    return fileList;
}

static void sendmail(std::string receiver_name)
{
    HINSTANCE hMapI = ::LoadLibraryA("MAPI32.dll");
    if(!hMapI){
        LOGE("Load MAPI32.DLL error");
        return;
    }

    ULONG (PASCAL *SendMail)(ULONG, ULONG_PTR, MapiMessage*, FLAGS, ULONG);
    (FARPROC&)SendMail = GetProcAddress(hMapI, "MAPISendMail");

    if(!SendMail){
        LOGE("Get SendMail function error");
        ::FreeLibrary(hMapI);
        return ;
    }

    std::vector<std::string> logFiles = getLogFile();
    logFiles.push_back("c:\\errorImage.jpg");
    LOGI("Log file count:%d", logFiles.size());

    std::vector<MapiFileDesc> filedesc;

  //  MapiFileDesc fileDesc[5];
    std::string pathName[5];
    std::string fileName[5];

    for(uint i = 0; i < logFiles.size(); i++){
        MapiFileDesc fileDesc;
        ::ZeroMemory(&fileDesc, sizeof(fileDesc));

        fileDesc.nPosition = (ULONG)-1;
        pathName[i] = logFiles.at(i);
        char* path1 = (char*)pathName[i].c_str();

        fileDesc.lpszPathName = path1;
        QFileInfo file(pathName[i].c_str());
        fileName[i] = file.fileName().toStdString();
        char*name1 = (char*)fileName[i].c_str();

        fileDesc.lpszFileName = name1;

        filedesc.push_back(fileDesc);

    }

    MapiMessage message;
    ::ZeroMemory(&message, sizeof(message));
    message.lpszSubject = "Flash tool issue";
    message.lpszNoteText = "Issue submit";
    message.lpFiles = &filedesc[0];
    message.nFileCount = filedesc.size();

    MapiRecipDesc recv = {0};
    recv.ulRecipClass = MAPI_TO;
    std::string receiver_mail_addr = "SMTP:"+receiver_name+"@mediatek.com";
    LOGI(receiver_mail_addr.c_str());
    recv.lpszAddress = const_cast<char*>(receiver_mail_addr.c_str());
    recv.lpszName = const_cast<char*>(receiver_name.c_str());

    message.lpRecips = &recv;
    message.nRecipCount = 1;

    int ret = SendMail(0, 0, &message, /*MAPI_LOGON_UI|*/MAPI_DIALOG, 0);
    if(ret != SUCCESS_SUCCESS )
    {
        LOGE("Failed to send mail, error code: %d", ret);
        if(ret == MAPI_E_ATTACHMENT_OPEN_FAILURE)
        {
            LOGE("Failed to open attachment file.");
        }
    }

    ::FreeLibrary(hMapI);
}
#endif

int flashtool_message_box(QWidget *parent,
                          QAbstractButton** pConnectButton,
                           MessageBox_Type msgbox_type,
                           const QString &title,
                           const QString &errMsg,
                           const QString &btn0_txt,
                           const QString &btn1_txt,
                           const QString &btn2_txt,
                           Assistant* assistant_,
                           const QString &page,
                           std::string receiver_name,
                           bool report)
{
    int ret = -1;

    switch(msgbox_type)
    {
    case STANDRAD_MSGBOX:
    case INFORMATION_MSGBOX:
        return message_box_information(parent,
                                title,
                                errMsg,
                                btn0_txt);

    case WARNING_MSGBOX:
        ret = message_box_warning(parent,
                                  pConnectButton,
                                  title,
                                  errMsg,
                                  btn0_txt,
                                  btn1_txt,
                                  btn2_txt,
                                  report);
        if(ret == IDC_BUTTON_HELP)
        {
            assistant_->ShowDocumentation(page);
        }
    #ifdef _WIN32
        else if(report && ret == IDC_BUTTON_SENDREPORT)
        {
            sendmail(receiver_name);
        }
    #endif
        return ret;

    case CRITICAL_MSGBOX:
        ret = message_box_critical(parent,
                                   pConnectButton,
                                   title,                                   
                                   errMsg,
                                   btn0_txt,
                                   btn1_txt);
    #ifdef _WIN32        
        if(report && ret == 1) //report
        {
            sendmail(receiver_name);
        }
    #endif

        return ret;

    case QUESTION_MSGBOX:
        return message_box_question(parent,
                                    title,
                                    errMsg,
                                    btn0_txt,
                                    btn1_txt);

    default:
        return ret;
    }
}


U32 Utils::getRegionPartId(QString region_str)
{
    U32 ret = 0;

    std::map<QString,U32>::const_iterator it = REGION_MAP.find(region_str);
    if(it != REGION_MAP.end())
    {
        ret = it->second;
    }
    else
    {
        Q_ASSERT(0 && "unknown region");
    }
    return ret;
}

QString adaptorRegionID(U32 nRegionId, HW_StorageType_E en_storage_type)
{
    switch (nRegionId)
    {
    case 1:
        return en_storage_type == HW_STORAGE_UFS ? "UFS_LU0" : "EMMC_BOOT_1";
    case 2:
        return en_storage_type == HW_STORAGE_UFS ? "UFS_LU1" : "EMMC_BOOT_2";
    case 3:
        return en_storage_type == HW_STORAGE_UFS ? "UFS_LU2" : "EMMC_RPMB";
    default:
        return "";
    }
}


QString Utils::getRegionStr(U32 regionId, HW_StorageType_E en_storage_type)
{
    QString sRegionStr = adaptorRegionID(regionId, en_storage_type);
    if (sRegionStr.isEmpty())
    {
        std::map<QString,U32>::const_iterator it;
        for (it = REGION_MAP.begin(); it != REGION_MAP.end(); ++it)
        {
            if (it->second == regionId)
            {
                sRegionStr = it->first;
                break;
            }
        }
    }
    return sRegionStr;
}

//print usb driver version
#ifdef _WIN32

#include <windows.h>
#include <SetupAPI.h>
#include <string>
#include <algorithm>

#pragma comment(lib, "Setupapi.lib")

static std::string StringToUpper(std::string strToConvert)
{
    std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), ::toupper);

    return strToConvert;
}

static bool found(std::string src, std::string sub)
{
    std::size_t pos = StringToUpper(src).find(StringToUpper(sub));
    return (pos != std::string::npos);
}

static bool find_usbdev(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA *DeviceInfoData, std::string vid, std::string pid)
{
    char szDeviceInstanceID[MAX_PATH] = {0};

    BOOL r = SetupDiGetDeviceInstanceIdA(DeviceInfoSet, DeviceInfoData, szDeviceInstanceID, MAX_PATH, NULL);
    if(r && found(std::string(szDeviceInstanceID), vid) && found(std::string(szDeviceInstanceID), pid))
    {
        LOGD("szDeviceInstanceID : %s", szDeviceInstanceID);
        return true;
    }
    else
       return false;
}

void print_driver_version(void)
{
    GUID guid = {0x4d36e978, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}}; //comport

    HDEVINFO DeviceInfoSet = SetupDiGetClassDevs(
                                    &guid,
                                    NULL,
                                    NULL,
                                    DIGCF_PRESENT);

    SP_DEVINFO_DATA DeviceInfoData;
    ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    unsigned int DeviceIndex = 0;

    while (SetupDiEnumDeviceInfo(
                             DeviceInfoSet,
                             DeviceIndex,
                             &DeviceInfoData))
   {
        DeviceIndex++;

        SP_DRVINFO_DATA drvInfo;
        drvInfo.cbSize = sizeof(SP_DRVINFO_DATA);


        // Build a list of driver info items that we will retrieve below
        if (!SetupDiBuildDriverInfoList(DeviceInfoSet,
                                        &DeviceInfoData, SPDIT_COMPATDRIVER))
            return; // Exit on error

        if (!SetupDiEnumDriverInfo(DeviceInfoSet, &DeviceInfoData, SPDIT_COMPATDRIVER, 0, &drvInfo))
        {
            int err_code = GetLastError();
            LOGD("err : %d\n", err_code);
        }
        else
        {

            //find device by PID/VID, support 0e8d_0003(bootrom), 0e8d_2000(preloader)
            if(find_usbdev(DeviceInfoSet, &DeviceInfoData, "0e8d", "0003")
                || find_usbdev(DeviceInfoSet, &DeviceInfoData, "0e8d", "2000"))
            {
                unsigned high = (unsigned)(drvInfo.DriverVersion >> 32);
                unsigned low = (unsigned)(drvInfo.DriverVersion & 0xffffffffULL);
                LOGD("Driver version is %d.%d.%d.%d",
                       (high >> 16),(high & 0xffffu),(low >> 16),(low & 0xffffu));
            }
        }

    }

    if (DeviceInfoSet)
    {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }
    return;
}
#endif

//end of print usb driver version


HW_StorageType_E getDeviceStorageType(const DA_REPORT_T *p_da_report)
{
    Q_ASSERT(NULL != p_da_report);
    if(p_da_report->m_nor_ret == S_DONE)
    {
        return HW_STORAGE_NOR;
    }
    else if (p_da_report->m_nand_ret == S_DONE)
    {
        return HW_STORAGE_NAND;
    }
    else if (p_da_report->m_emmc_ret == S_DONE)
    {
        return HW_STORAGE_EMMC;
    }
    else if (p_da_report->m_sdmmc_ret == S_DONE)
    {
        return HW_STORAGE_SDMMC;
    }
    else if (p_da_report->m_ufs_ret == S_DONE)
    {
        return HW_STORAGE_UFS;
    }
    else
    {
        Q_ASSERT(false);
        return HW_STORAGE_NONE;
    }
}

bool FilePatternContain(QString filename, QString pattern)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text))
        return false;

    QRegExp reg(pattern);
    QTextStream in(&file);
    while(!in.atEnd())
    {
        QString line = in.readLine();
        if(reg.indexIn(line) != -1)
            return true;
    }

    return false;
}

#ifdef _WIN32
bool IsMoreThanOneProcessByName(const QString &processName)
{
    int nProcessCnt = 0;
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        LOGD("CreateToolhelp32Snapshot fail");
        return nProcessCnt > 1;
    }

    PROCESSENTRY32 processEntry32;
    processEntry32.dwSize = sizeof(PROCESSENTRY32);
    bool bProcess = Process32First(hProcessSnap, &processEntry32);
    while (bProcess)
    {
        if (_wcsicmp(processEntry32.szExeFile, processName.toStdWString().c_str()) == 0)
        {
            LOGD("ProcessName: %s, ProcessID: %d",
                 QString::fromWCharArray(processEntry32.szExeFile).toStdString().c_str(), processEntry32.th32ProcessID);
            ++nProcessCnt;
        }
        bProcess = Process32Next(hProcessSnap, &processEntry32);
    }
    LOGD("Flash Tool total opened process count: %d", nProcessCnt);
    CloseHandle(hProcessSnap);
    return nProcessCnt > 1;
}
#endif

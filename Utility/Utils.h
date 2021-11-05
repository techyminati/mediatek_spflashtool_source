#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QDir>
#include <QTextCodec>
#include <QImage>
#include <QAbstractButton>
#include <sstream>
#include <map>
#include "../BootRom/brom.h"
#include "JSONParser/json.h"
#include "constString.h"
#include "../UI/src/Assistant.h"

using QtJson::JsonObject;

#define MAX_SUPPORT_BB      50

#ifdef _WIN32
    #define stricmp _stricmp
#else
    #define stricmp strcasecmp
#endif

typedef enum
{
    NORMAL_SCATTER,
    SCI_SCATTER
}DL_SCATTER_TYPE;

typedef enum{
    FORMAT_ALL_DOWNLOAD = 0,
    FIRMWARE_UPGRADE,
    DOWNLOAD_ONLY,
    WIPE_DATA,
    UNKOWN_DOWNLOAD_SCENE
}Download_Scene;

typedef enum{
    STANDRAD_MSGBOX = 0,
    INFORMATION_MSGBOX = 1,
    WARNING_MSGBOX = 2,
    CRITICAL_MSGBOX = 3,
    QUESTION_MSGBOX = 4
}MessageBox_Type;

#define ENUM_TO_STRING(case_id)\
    case case_id:\
        return #case_id;

template<typename T>
std::string NumberToString(T number) {   //Usage: NumberToString ( Number );
    std::ostringstream ss;
    ss << number;
    return ss.str();
}

std::string ToUppercaseString(const std::string& str);
std::string ToLowercaseString(const std::string& str);
QString EnumToRomString(HW_StorageType_E storage_, U32 id, bool scatter_ver2_readBack = false);
std::string DownloadScenetEnumToString(Download_Scene scene);
std::string EfuseExtraKeyToString(EFUSE_KEY key);
//EMMC_Part_E GetPartID(int index);
#if 0
std::string WStr2Str(const std::wstring& ws);
#endif
std::wstring Str2WStr(const std::string& s);

namespace Utils
{
#if 0
bool ScanNewCreatedUSBPort(std::string &device_path, std::string &friendly_name,
                                  const std::string& pid, const std::string& vid,
                                  int *stop_flag = NULL,
                                  unsigned int timeout_ms = 0xFFFFFFFF);
#endif
int GetPortNumFromStr(const QString& str);

int GetPartitionInfo(const FLASHTOOL_API_HANDLE_T &flashtool_handle,
                     const std::string &part_name,
                     PART_INFO &part_info);

inline QString ExtractFileFromPath(const QString& path)
{
    QString short_name;
    int sep = path.lastIndexOf(QDir::separator());
    if(sep != -1)
    {
        short_name = path.mid(sep+1);
    }
    else
    {
        short_name = path;
    }
    return short_name;
}

QString GetPlatformFromScatter(const QString& scatter_file);
QString GetAddrFromIniFile(const BBCHIP_TYPE bbchip, const QString &file_path = "");
U64 ConvertStrToAddr(const QString str_addr);

HW_StorageType_E GetStorageTypeFromScatter(const QString& scatter_file);
std::string ReplaceAllSubstring(const std::string &ori_str, const std::string &sub_str, const std::string &new_sub_str);
bool IsFoundDLImageByScatterFile(DL_HANDLE_T p_dl_handle, ROM_INFO *p_rom_info,const std::string &rom_name);

std::string PrettyByteString(U64 bytes);
QString ULLToHex(U64 src, unsigned int size);
int GetRomFilesTotalSize(DL_HANDLE_T &dl_handle, U64 *file_size);

const QString ExtClockToName(const EXT_CLOCK  ext_clock);
const QString ramType(HW_RAMType_E type);

std::string GetSpecifalFolderPath();
QImage AddStringToImage(const QString& imagePath, const QString& str, const QString& owner="");
bool parserJSONFile(const QString filePath, QtJson::JsonObject &jsonObject);

inline void SetTextCodec(void) {
#ifdef _WIN32
    QTextCodec *coder = QTextCodec::codecForName("System");
#else
    QTextCodec *coder = QTextCodec::codecForName("UTF-8");
#endif
    QTextCodec::setCodecForTr(coder);
    QTextCodec::setCodecForLocale(coder);
    QTextCodec::setCodecForCStrings(coder);
}

inline QTextCodec * GetTextCodec(void) {
#ifdef _WIN32
    static QTextCodec *codec = QTextCodec::codecForName("System");
#else
    static QTextCodec *codec = QTextCodec::codecForName("UTF-8");
#endif
    return codec;
}

U32 getRegionPartId(QString region_str);
QString getRegionStr(U32 regionId, HW_StorageType_E en_storage_type);


}
#if (!defined  _WIN32)
        const std::string WIN_HEX_FORMAT("%016I64");
        const std::string LINUX_HEX_FORMAT("%016ll");
#endif

void grabWindow();

int flashtool_message_box(QWidget *parent,
                          QAbstractButton **connectButton,
                           MessageBox_Type msgbox_type,
                           const QString &title,
                           const QString &errMsg,
                           const QString &btn0_txt,
                           const QString &btn1_txt = "",
                           const QString &btn2_txt = "",
                           Assistant* assistant_ = 0,
                           const QString &page = "",
                           std::string receiver_name = "",
                           bool report = false);

HW_StorageType_E getDeviceStorageType(const DA_REPORT_T *p_da_report);
bool FilePatternContain(QString filename, QString pattern);


#ifdef _WIN32
void print_driver_version(void);
bool IsMoreThanOneProcessByName(const QString &processName);
#endif

#endif // UTILS_H

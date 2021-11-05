#include "FileUtils.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <sys/io.h>
#include <sys/utsname.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cerrno>
#include <cstring>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>

#include "../Host/Inc/RuntimeMemory.h"
#include "../Utility/Utils.h"
//Log files
#include "../Logger/Log.h"
#include "../BootRom/brom.h"

#ifdef __BORLANDC__
#pragma warn -8004
#endif

#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif

namespace FileUtils{
    using std::string;
    const std::string kBackupFolder("BackupData");
    const unsigned int kChipIDLength(16);
#if defined(_LINUX)
#define RULES_PATH "/etc/udev/rules.d"
#define TTYACMS_FILE "99-ttyacms.rules"
#define KERNEL_INVALID_MIN_VERSION std::string("4.11.12")
#define KERNEL_INVALID_MAX_VERSION std::string("4.19.14")
#endif
#define HISTORY_INI_FILE "history.ini"
}

E_CHECK_DIR_STATUS FileUtils::CheckDirectory(const std::string &dir, bool createIfNotExist) {
    QDir dir_obj(QString::fromStdString(dir));
    if(dir_obj.exists()){
        LOG("%s(): the dir(%s) already exists.", __FUNCTION__, qPrintable(dir_obj.absolutePath()));
        return DIR_EXIST;
    }

    if(!createIfNotExist){
        LOG("%s(): the dir(%s) does not exist.", __FUNCTION__, qPrintable(dir_obj.absolutePath()));
        return DIR_NOT_EXIST;
    }

    if(!dir_obj.mkpath(dir_obj.absolutePath())){
        LOG("%s(): create the dir(%s) failed.", __FUNCTION__, qPrintable(dir_obj.absolutePath()));
        return CREATE_DIR_FAIL; //create directory fail
    }
    else
    {
#ifdef _WIN32
        _chmod(dir.c_str(), _S_IREAD | _S_IWRITE);
#else
        chmod(dir.c_str(),  S_IXUSR |S_IRUSR | S_IWUSR |
              S_IXGRP |S_IRGRP | S_IWGRP |
              S_IXOTH |S_IROTH | S_IWOTH );
#endif
        LOG("%s(): create the dir(%s) succeed.", __FUNCTION__, qPrintable(dir_obj.absolutePath()));
        return CREATE_DIR_SUCCESS; //create directory successfully
    }
}

bool FileUtils::CreateFiles(std::string fileName)
{
    QFile file(fileName.c_str());
    file.open(QIODevice::ReadWrite);
    file.close();

    return true;
}

/**
 * Check whether the directory exists
 *
 * @param directory to be checked
 *
 * @return whether the directory exists
 */
bool FileUtils::IsDirectoryExist(const  string & dir) {
    struct stat fileInfo;
    int ret = stat(dir.c_str(), &fileInfo);
    return ( (ret == 0) && ((fileInfo.st_mode & S_IFMT) == S_IFDIR));
}

/**
 * Check whether the file exists
 *
 * @param file to be checked
 *
 * @return whether the file exists
 */
bool FileUtils::IsFileExist(const  string & file) {
    struct stat fileInfo;
    int ret = stat(file.c_str(), &fileInfo);
    return ( (ret == 0) && ((fileInfo.st_mode & S_IFMT) == S_IFREG));
}

bool FileUtils::QIsFileExist(const std::string &file)
{
     QFile filePath(QString::fromLocal8Bit(file.c_str()));
     if(!filePath.exists())
         return false;

     return true;
}

/**
 * Delete a directory
 *
 * @param directory to be deleted
 *
 * @return whether the directory is deleted
 */

bool FileUtils::QDeleteDirectory(const std::string &dir_name)
{
    bool result = true;
    QString dir_temp(dir_name.c_str());
    QDir dir(dir_temp);

    if (dir.exists(dir_temp)) {
        Q_FOREACH(QFileInfo info,
                  dir.entryInfoList(QDir::NoDotAndDotDot
                                    | QDir::System
                                    | QDir::Hidden
                                    | QDir::AllDirs
                                    | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                result = QDeleteDirectory(info.absoluteFilePath().toLocal8Bit().constData());
            }
            else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dir_temp);
    }

    return result;
}

bool FileUtils::QDeleteFile(const std::string &file)
{
    if(file.empty())
    {
        LOG("File does not exist.");
        return true;
    }
    QFile filePath(QString::fromLocal8Bit(file.c_str()));

    return filePath.remove();
}

const std::string &FileUtils::GetAppDirectory()
{
    return g_app_path;
}

void FileUtils::UpdateAppPath(const std::string &app_path) {
    g_app_path = app_path;
}

std::string FileUtils::AbsolutePath(const char *name)
{
    return g_app_path + C_SEP_STR + name;
}

std::string FileUtils::AbsolutePath(const std::string &name)
{
    return g_app_path + C_SEP_STR + name;
}

#if 0    //deprecated function
E_CHECK_DIR_STATUS FileUtils::CheckDirByCOMPor.t(int port, std::string &dir,
                                                bool createIfNotExist){
    string parentDir = FileUtils::GetAppDirectory()+"\\BackupNvram";
    E_CHECK_DIR_STATUS status = FileUtils::CheckDirectory(parentDir,createIfNotExist);
    if(status == DIR_NOT_EXIST || status == CREATE_DIR_FAIL){
        return status;
    }
    dir = parentDir + "\\" + NumberToString(port);
    return FileUtils::CheckDirectory(dir,createIfNotExist);
}

E_CHECK_DIR_STATUS FileUtils::CheckDirByChipID(unsigned char* chipId, unsigned int length,
                                               std::string &dir, bool createIfNotExist){
    std::ostringstream oss;
    oss<<std::hex;
    for(unsigned int i=0;i<length;++i){
        oss<<std::setfill('0')<<std::setw(2)<<static_cast<unsigned int>(chipId[i]);
    }
    string chipName = oss.str();

    string parentDir = FileUtils::GetAppDirectory()+"\\BackupNvram";
    E_CHECK_DIR_STATUS status = FileUtils::CheckDirectory(parentDir, createIfNotExist);
    if(status == DIR_NOT_EXIST || status == CREATE_DIR_FAIL){
        return status;
    }
    dir = parentDir + "\\" + chipName;
    return FileUtils::CheckDirectory(dir, createIfNotExist);
}
#endif

bool FileUtils::MakeParentBackupDir(const string& folder)
{
    std::string fullpath = ABS_PATH(folder);

    E_CHECK_DIR_STATUS backup_dir_status =
            FileUtils::CheckDirectory(fullpath, true);

    return ( DIR_EXIST ==  backup_dir_status ||
             CREATE_DIR_SUCCESS == backup_dir_status )
            ? true : false;
}

std::string FileUtils::ComposeDirNameByChipID(const unsigned char* chipId, const unsigned int length, const string& folder, bool createIfNotExist)
{
    std::ostringstream oss;
    oss<<std::hex;
    for(unsigned int i=0;i<length;++i){
        oss<<std::setfill('0')<<std::setw(2)<<static_cast<unsigned int>(chipId[i]);
    }
    string chipName = oss.str();

    string dirName = ABS_PATH(folder + C_SEP_STR + chipName);

    QString path = QString::fromLocal8Bit(dirName.c_str());
    path = QDir::toNativeSeparators(path);

    LOG("%s(): folder name: %s. create it...", __FUNCTION__, path.data());

    CheckDirectory(string((const char *)path.toLocal8Bit()), createIfNotExist);

    return string((const char *)path.toLocal8Bit());

}

bool FileUtils::FindFile(const std::string& path, std::list<std::string> &files,
                         const std::string& pattern, COMPARE_METHOD compare_m, bool recursive)
{
    QDir dir(path.c_str());

    if(!dir.exists())
        return false;

    dir.setFilter(QDir::Dirs | QDir::Files);

    QFileInfoList list = dir.entryInfoList();
    int i = 0;
    bool isDirectory;
    std::string fileName;

    if(list.empty())
        return false;

    do
    {
        QFileInfo fileInfo = list.at(i);

        if(fileInfo.fileName() == "." || fileInfo.fileName() == "..")
        {
            i++;
            continue;
        }

        isDirectory = fileInfo.isDir();

        if(isDirectory && recursive)
        {
            FindFile(fileInfo.filePath().toStdString(), files, pattern, compare_m, recursive);
        }
        else
        {
            fileName = fileInfo.fileName().toStdString();

            if(compare_m == NULL)
            {
                if(pattern == "")
                {
                    files.push_back(fileName);
                }
                else if(pattern == fileName)
                {
                    files.push_back(fileName);
                }
            }
            else
            {
                if(compare_m(fileName, pattern))
                {
                    files.push_back(fileName);
                }
            }
        }

        i++;
    }while(i < list.size());

    return true;
}

/**
 * Get the size of the given file.
 *
 * @param filename a filename
 *
 * @return size of the given file
 */
unsigned int FileUtils::GetFileSize(const std::string &filename)
{
    unsigned long size = -1;
    struct stat statbuff;

    if(stat(filename.c_str(), &statbuff) == 0)
        size = statbuff.st_size;

    return size;

 /*   unsigned int size;
    std::ifstream is(filename.c_str(), std::ios::in | std::ios::binary);

    is.seekg(0, std::ios::end);
    size = is.tellg();
    is.close();

    return size;*/
}

/**
 * Get the content of the given file. The caller is responsible for memory
 * deallocation.
 *
 * @param filename a filename
 *
 * @return content of the given file
 */
unsigned char* FileUtils::GetFileContent(const std::string &filename)
{
    std::ifstream is(filename.c_str(), std::ios::in | std::ios::binary);
    is.seekg(0, std::ios::end);

    const unsigned int size = is.tellg();
    unsigned char *content = new unsigned char[size];

    is.seekg(0, std::ios::beg);
    is.read((char*)content, size);
    is.close();

    return content;
}

/**
 * Set debug log file on
 *
 * @param NULL
 *
 * @return NULL
 */
void FileUtils::DebugLogsOn( void )
{
    std::string dll_log;
    Logger::SetSPFlashToolDumpFileFolder();
    Logger::DebugOff();
    Logger::DebugOn();
#ifdef _WIN32
    dll_log = Logger::GetSPFlashToolDumpFileFolder() + "\\BROM_DLL_V5.log";
#else
    dll_log = Logger::GetSPFlashToolDumpFileFolder() + "/BROM_DLL_V5.log";
#endif
    Brom_Debug_SetLogFilename(dll_log.c_str());
    Brom_DebugOn();
    Brom_GetDLLInfo(NULL, NULL, NULL, NULL);
}

/**
 * Set debug log file on
 *
 * @param NULL
 *
 * @return NULL
 */
void FileUtils::DebugLogsOff( void ) {
    Logger::DebugOff();
    Brom_DebugOff();
}

bool FileUtils::ComputeCloneCheckSum(const std::string &src_file, RomMemChecksumArg  &chksum_arg)
{
    FILE *file = fopen ( src_file.c_str() , "rb" );
    if (file==NULL) {
        LOG("file open failed: %s.", src_file.c_str());
        return false;
    }
    chksum_arg.m_cb_rom_mem_checksum_init(NULL, src_file.c_str());
    chksum_arg.chksum= 0;

#ifndef _WIN32
#define _stat64 stat64
#define _fstat64 fstat64
#define _fileno fileno
#endif

    struct _stat64  file_stat;
    memset(&file_stat, 0, sizeof(file_stat));

    if( _fstat64(_fileno(file), &file_stat) ) {
        fclose(file);
        return false;
    }

    unsigned int file_len = 0;
    if( 0 >= (file_len=file_stat.st_size) ) {
        fclose(file);
        return false;
    }

    unsigned char buffer[1024];
    unsigned int read_size = 0;
    static unsigned char lastPercentage = 0;
    unsigned char percentage = 0;
    unsigned int position = 0;
    while(!feof(file))
    {
        read_size = fread(buffer, 1, 1024, file);
        position = ftell(file);
        percentage = static_cast<unsigned char>((static_cast<float> (position)/file_len)*100);
        if (lastPercentage != percentage)
        {
            lastPercentage = percentage;
            chksum_arg.m_cb_rom_mem_checksum(percentage,
                                             position, read_size, chksum_arg.m_cb_rom_mem_checksum_arg);
        }
        for(unsigned int i=0;i<read_size;i++)
            chksum_arg.chksum += buffer[i];
    }

    chksum_arg.m_cb_rom_mem_checksum(100, file_len, file_len, NULL);

    LOG("check sum result(%u).", chksum_arg.chksum);
    fclose (file);
    return true;
}

bool FileUtils::ComputeCheckSum(const std::string &src_file, unsigned char &chksum)
{
    unsigned char buffer[1024];

    FILE *file = fopen ( src_file.c_str() , "rb" );
    if (file==NULL)
    {
        LOG("%s(): file open failed: %s.", __FUNCTION__, src_file.c_str());
        return false;
    }

    chksum = 0;
    unsigned int read_size = 0;
    while(!feof(file)){
        read_size = fread(buffer, 1, 1024, file);

        for(unsigned int i=0;i<read_size;i++){
            chksum += buffer[i];
        }
    }

    LOG("%s(): check sum result(%u).", __FUNCTION__, chksum);
    fclose (file);
    return true;
}

bool FileUtils::LoadCheckSum(const std::string &chksum_file, unsigned char &chksum)
{
    FILE *file = fopen ( chksum_file.c_str() , "rb" );
    if (file==NULL) {
        LOG("%s(): file open failed: %s.", __FUNCTION__, chksum_file.c_str());
        return false;
    }

    unsigned char buffer[2];
    unsigned int read_size = 0;
    read_size = fread(buffer, 1, sizeof(buffer), file);
    if(read_size <= 0)
    {
        LOG("%s(): read file error: %s", __FUNCTION__, chksum_file.c_str());
        fclose(file);
        return false;
    }

    chksum = buffer[0];

    LOG("%s(): check sum result(%u).", __FUNCTION__, chksum);
    fclose (file);
    return true;
}

bool FileUtils::SaveCheckSum(const std::string &chksum_file, const unsigned char chksum)
{
    unsigned char buffer[2] = {0};
    buffer[0] = chksum;

    FILE *file = fopen ( chksum_file.c_str() , "wb" );
    if (file==NULL) {
        LOG("%s(): file create/open failed: %s.", __FUNCTION__, chksum_file.c_str());
        return false;
    }

    fwrite (buffer , 1 , sizeof(buffer) , file );

    LOG("%s(): check sum result(%u).", __FUNCTION__, chksum);
    fclose (file);
    return true;
}

int FileUtils::LoadFile(const char *pathname, unsigned char **pp_buf, unsigned int *p_nFileLength, long *p_last_modify_time)
{
    if(pp_buf == NULL)
    {
        LOG("pp_buf is null.");
        return 1;
    }

    unsigned int ret = LoadFileInfo(pathname, p_nFileLength, p_last_modify_time);
    if(ret) return ret;

    FILE *fp;
    FileUtils::FileOpenSentry file_sentry(pathname, "rb", fp);

    unsigned int buf_len = *p_nFileLength;
    unsigned char *buf;

    if( NULL == (buf=(unsigned char *)malloc(buf_len)) ) {
        LOG("buf malloc fails.");
        return 5;
    }

    size_t result = fread(buf, 1, buf_len, fp);
    if(result != buf_len && !feof(fp))
    {
        free(buf);
        buf = NULL;
        LOGD("fread error.");
        return 1;
    }

    *pp_buf = buf;

    return 0;

}

int FileUtils::LoadFileInfo(const char *pathname, unsigned int *p_nFileLength, long* p_last_modify_time)
{
    FILE *fp;

    if(pathname == NULL || pathname[0] == '\0' || p_nFileLength == NULL) return 1;

    FileOpenSentry file_sentry(pathname, "rb", fp);

    Q_ASSERT(fp != NULL);

#ifdef _WIN32
    struct _stat file_stat = {0};
    if(_fstat(_fileno(fp), &file_stat)) return 2;
    *p_nFileLength = _filelength(_fileno(fp));
#else
    struct stat file_stat;
    memset(&file_stat, 0, sizeof(struct stat));
    if(fstat(fileno(fp), &file_stat)) return 2;
    fseek(fp, 0, SEEK_END);
    *p_nFileLength = ftell(fp);
#endif

    Q_ASSERT((*p_nFileLength) > 0);

    if( NULL != p_last_modify_time ) {
        *p_last_modify_time = file_stat.st_mtime;
    }

    return 0;
}

int FileUtils::SaveFile(const char*filepath, unsigned char*p_buf, unsigned int file_len)
{
     FILE *fp;

     if(filepath == NULL || p_buf == NULL || file_len == 0)
     {
         LOGD("File path or content buffer or file length is abnormal, please check it first.");
         return 1;
     }

     FileOpenSentry file_sen(filepath, "wb", fp);
     if(fp == NULL)
     {
         LOGD("Open file failed.");
         return 2;
     }

     if(file_len > fwrite(p_buf, 1, file_len, fp))
     {
         LOGD("Write to file failed.");
         return 3;
     }

     return 0;
}

#if defined(_LINUX)
void FileUtils::copy_99ttyacms_file(bool need_check_first_run/* = false*/)
{
    if (need_check_first_run) {
        QString history_file = QString::fromStdString(AbsolutePath(std::string(HISTORY_INI_FILE)));
        history_file = QDir::toNativeSeparators(history_file);
        if (IsFileExist(history_file.toStdString())) { // not the first to run flash tool GUI, skip copy
            LOG("skip to copy 99-ttyacms.rules file, reason: the tool not the first time to run!");
            return;
        }
    }
    // linux modemmanager issue fixed: copy 99-ttyacms.rules file to /etc/udev/rules.d directory.
    QString src_file = QString::fromStdString(AbsolutePath(std::string(TTYACMS_FILE)));
    src_file = QDir::toNativeSeparators(src_file);
    if (IsFileExist(src_file.toStdString())) {
        if (IsDirectoryExist(std::string(RULES_PATH)) && QFileInfo(QString(RULES_PATH)).isWritable()) {
            QString dest_file = QString(RULES_PATH) + QDir::separator().toLatin1() + QString(TTYACMS_FILE);
            dest_file = QDir::toNativeSeparators(dest_file);
            if (!IsFileExist(dest_file.toStdString())) {
                if (QFile::copy(src_file, dest_file)) {
                    LOG("copy 99-ttyacms.rules file successfully!");
                } else {
                    LOG("copy 99-ttyacms.rules file failed, reason: %s", strerror(errno));
                }
            } else {
                LOG("skip to copy 99-ttyacms.rules file, reason: %s has already exist!", dest_file.toStdString().c_str());
            }
        } else {
            LOG("skip to copy 99-ttyacms.rules file, reason: %s has no write permission!", std::string(RULES_PATH).c_str());
        }
    } else {
        LOG("skip to copy 99-ttyacms.rules file, reason: %s not exist!", src_file.toStdString().c_str());
    }
}

void DumpKernelInfo(const struct utsname &kernel_info)
{
    printf("sysname:%s\n nodename:%s\n release:%s\n version:%s\n machine:%s\n \n ",\
                        kernel_info.sysname,\
                        kernel_info.nodename,\
                        kernel_info.release,\
                        kernel_info.version,\
                        kernel_info.machine);
#if _UTSNAME_DOMAIN_LENGTH - 0
#ifdef __USE_GNU
    printf(" domainame:%s\n ",kernel_info.domainname);
    //char domainname[_UTSNAME_DOMAIN_LENGTH];
#else
    printf(" __domainame:%s\n ",kernel_info.__domainname);
    //char __domainname[_UTSNAME_DOMAIN_LENGTH];
#endif
#endif
}

bool FileUtils::IsValidKernelVersion()
{
    struct utsname buf;
    int ret = uname(&buf);
    if (ret < 0) {
        perror("uname function call failed.");
        return true; //ignore to check linux kernel version
    }
    else {
        DumpKernelInfo(buf);

        std::string kernel_release = buf.release;
        std::size_t found = kernel_release.find('-');
        if (found == std::string::npos) {
            perror("invalid kernel release string");
            return true;
        }
        std::string cur_kernel_version = kernel_release.substr(0, found);
        std::cout << "cur_kernel_version = " << cur_kernel_version << std::endl;
        bool is_valid_kernel = (KERNEL_INVALID_MIN_VERSION > cur_kernel_version
                || cur_kernel_version >= KERNEL_INVALID_MAX_VERSION);
        return is_valid_kernel;
    }
}
#endif

#include "ReadbackSettingAssist.h"
#include <QString>
#include <QFileInfo>
#include "../BootRom/host.h"
#include "../Utility/FileUtils.h"
#include "../Err/Exception.h"
#include "../Err/FlashToolErrorCodeDef.h"
#include "../Logger/Log.h"

namespace APCore
{

std::string APCore::ReadbackSettingAssist::GenerateValidPath(const std::string &path)
{
    if (QString(path.c_str()).trimmed().isEmpty())
    {
        LOGE("read file name error: readback file name has empty!\n");
        THROW_APP_EXCEPTION(FT_FILE_IS_NOT_EXIST, "readback file name has empty!");
    }
    QFileInfo fileInfo(path.c_str());
    std::string dirName = fileInfo.absolutePath().toStdString();
    E_CHECK_DIR_STATUS check_dir_status = FileUtils::CheckDirectory(dirName, true);
    std::string _path = fileInfo.absoluteFilePath().toStdString();
    if (check_dir_status == CREATE_DIR_FAIL)
    {
        _path = FileUtils::AbsolutePath(QFileInfo(path.c_str()).fileName().toStdString());
    }
    return _path;
}

}

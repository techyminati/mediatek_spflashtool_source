#include "OptionDialog.h"
#include "ui_OptionDialog.h"
#include "MainWindow.h"
#include "MainController.h"
#include "../../Setting/PlatformSetting.h"

#include "../../Setting/ConnBromUARTSetting.h"
#include "../../Setting/ConnBromUSBSetting.h"
#include "../src/MainWindowCallback.h"
#include "../../Utility/USBCOMFinderWrapper.h"
#include "../../Utility/constString.h"
#include "../../Utility/IniItem.h"
#include "../../Conn/Connection.h"
#include "../../BootRom/DOWNLOAD.H"
#include "../../BootRom/internal_api.h"

#include <QPushButton>
#include <QSettings>
#include <QFileDialog>
#include <QLibrary>
#include <QDateTime>
#include "../../Utility/FileUtils.h"
#include "../../Utility/LogFilesClean.h"
#include "../../Host/Inc/RuntimeMemory.h"
#include "../../Utility/version.h"
#include "AsyncUARTComPortInfo.h"

#ifdef _WIN32
#include <Dbt.h>
#include <WinUser.h>
#include <Windows.h>
#endif

#include <QSplitter>

#include <QWindowsStyle>
#include <QWindowsXPStyle>
#include <QWindowsVistaStyle>
#include <QMacStyle>
#include <QPlastiqueStyle>
#include <QMotifStyle>
#include <QFileDialog>

#include <algorithm>

#define DBUS_HAL_SERVICE            "org.freedesktop.Hal"
#define DBUS_HAL_PATH               "/org/freedesktop/Hal/Manager"
#define DBUS_HAL_INTERFACE          "org.freedesktop.Hal.Manager"
#define DBUS_HAL_DEVICE_PATH        "/org/freedesktop/Hal/Devices"
#define DBUS_HAL_DEVICE_INTERFACE   "org.freedesktop.Hal.Device"

class map_value_finder
{
public:
       map_value_finder(const std::string &cmp_string):m_s_cmp_string(cmp_string){}
       bool operator ()(const std::map<std::string, std::string>::value_type &pair)
       {
            return pair.second == m_s_cmp_string;
       }
private:
        const std::string &m_s_cmp_string;
};

typedef struct
{
    Command_Category category;
    int cmd;
}short_cut;

OptionDialog::OptionDialog(QWidget *parent, MainWindow *mainWindow) :
    QDialog(parent),
    main_window_(mainWindow),
    ui(new Ui::OptionDialog),
    app_translator(),
    platform_setting(),
    m_workerThread(NULL)
{
    ui->setupUi(this);
    this->setFixedSize(this->width(), this->height());

    QStringList items;

    items << LoadQString(LANGUAGE_TAG, IDS_STRING_ONEDAY)
          << LoadQString(LANGUAGE_TAG, IDS_STRING_ONEWEEK)
          << LoadQString(LANGUAGE_TAG, IDS_STRING_ONEMONTH)
          << LoadQString(LANGUAGE_TAG, IDS_STRING_THREEMONTH);

    ui->comboBox__LogDate->addItems(items);

    items.clear();
    items << tr("trace") << tr("debug") << tr("info")
          << tr("warning") << tr("error") << tr("fatal");
    ui->comboBox_da_log_level->addItems(items);

    items.clear();
    items << tr("NONE") << tr("UART") << tr("USB") << tr("UART && USB");
    ui->comboBox_da_log_channel->addItems(items);

    items.clear();
    items << tr("921600") << tr("460800") << tr("230400")
          << tr("115200") << tr("57600") << tr("38400")
          << tr("19200") << tr("9600");
    ui->comboRate->addItems(items);

    items.clear();
    items << tr("2") << tr("5") << tr("30") << tr("60");
    ui->comboTimeout->addItems(items);

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ShowPhysicalFormatItem(false);

    ShowCheckRPMBItem(false);

    InitUpdateOptions();

    ui->groupBox_DA_log->setVisible(false);

    ui->treeWidget->expandAll();

    Logger::SetSPFlashToolLogFolder();

    ui->lineEdit_logPath->setText(QString::fromLocal8Bit(Logger::GetSPFlashToolLogFolder().c_str()));

    InitThemeItems();

    InitKeyMapInfo();

    InitShortKeyTable();

    ShowCustomThemeSetting(false);

    connect(ui->radioUSB, SIGNAL(toggled(bool)), this, SLOT(onToggleUSB_UART(bool)));
    connect(ui->radioUART, SIGNAL(toggled(bool)), this,SLOT(onToggleUSB_UART(bool)));

    ui->radioUSB->setAutoExclusive(false);
    ui->radioUART->setAutoExclusive(false);

    InitSettings();

    // start thread to clean log
    startCleanThread();
}

OptionDialog::~OptionDialog()
{
    SaveSettings();

    if(NULL != m_workerThread){
        if (m_workerThread->isRunning())
        {
            m_workerThread->quit();
            m_workerThread->wait();
        }
        delete m_workerThread;
        m_workerThread = NULL;
    }

    delete ui;
    ui = NULL;
}

void OptionDialog::InitShortKeyTable()
{
    int size = ui->tableWidget->rowCount();
     for(int i = 0; i < size; i++)
     {
         ui->tableWidget->item(i, 1)->setText(bind_vector[i].c_str());
     }

     ui->comboBox_KeyModify->setCurrentIndex(-1);
     ui->comboBox_key->setCurrentIndex(-1);
}

void OptionDialog::InitKeyMapInfo()
{
    QSettings settings(ABS_PATH_C("key.ini"), QSettings::IniFormat);

    int size = settings.beginReadArray("Shortcuts");
    for (int i = 0; i < size; ++i) {
         settings.setArrayIndex(i);
         bind_vector.push_back(settings.value("Binding").toString().toStdString());
     }

    settings.endArray();
}

void OptionDialog::InitSettings()
{
    IniItem item("option.ini", "WelcomePage", "ShowWelcome");

    if(item.GetBooleanValue())
        ui->checkBox_ShowWelcome->setCheckState(Qt::Checked);
    else
        ui->checkBox_ShowWelcome->setCheckState(Qt::Unchecked);

    item.SetSectionName("RestoreState");
    item.SetItemName("Restore");

    if(item.GetBooleanValue())
        ui->checkBox_RestoreStartup->setCheckState(Qt::Checked);
    else
        ui->checkBox_RestoreStartup->setCheckState(Qt::Unchecked);

    item.SetSectionName("Download");
    item.SetItemName("PhyFmt");

    if(item.GetBooleanValue())
        ui->checkBox_physicalFormat->setCheckState(Qt::Checked);
    else
        ui->checkBox_physicalFormat->setCheckState(Qt::Unchecked);

    item.SetItemName("DAChksum");
    int select = item.GetIntValue();
    bool enableUSBChksum = (bool)(select & DHNDL_FLAG_CHKSUM_LEVEL_USB);
    if(enableUSBChksum)
        ui->checkBox_usbchksum->setCheckState(Qt::Checked);

    bool enableEMMCChksum = (bool)((select & DHNDL_FLAG_CHKSUM_LEVEL_STORAGE) >> 1);
    if(enableEMMCChksum)
        ui->checkBox_emmcchksum->setCheckState(Qt::Checked);

    item.SetItemName("StorageLifeCycleCheck");
    if(item.GetBooleanValue())
        ui->checkBox_StorageLifeCycle->setCheckState(Qt::Checked);
    else
        ui->checkBox_StorageLifeCycle->setCheckState(Qt::Unchecked);

#ifdef _WIN32
    if (!ToolInfo::IsCustomerVer())
    {
        item.SetSectionName("CheckUpdate");
        item.SetItemName("Update");

        if(item.GetBooleanValue())
            ui->checkBox->setCheckState(Qt::Checked);
        else
            ui->checkBox->setCheckState(Qt::Unchecked);
    }
#endif

    LoadGeneralSettings();

    LoadConnSettings();
}

void OptionDialog::SaveSettings()
{
    SaveGeneralSettings();

    SaveConnSettings();
}

void OptionDialog::LoadGeneralSettings()
{
    IniItem item("option.ini", "Trace", "EnableTrace");

    bool check = item.GetBooleanValue();

    ui->checkBox_enableTracing->setCheckState(check ? Qt::Checked : Qt::Unchecked);

    item.SetItemName("CleanOlder");
    int cleanLogIndex = item.GetIntValue();

    if(cleanLogIndex != -1)
        ui->comboBox__LogDate->setCurrentIndex(cleanLogIndex);
    else
        ui->comboBox__LogDate->setCurrentIndex(2); //default: one month

    item.SetItemName("DALogLevel");
    int daLogLevelIndex = item.GetIntValue();
    ui->comboBox_da_log_level->setCurrentIndex(daLogLevelIndex == -1 ? 2 : daLogLevelIndex); //defalut: info

    item.SetItemName("DALogChannel");
    int daLogChannelIndex = item.GetIntValue();
    ui->comboBox_da_log_channel->setCurrentIndex(daLogChannelIndex == -1 ? 1 : daLogChannelIndex); //defalut: uart

#ifdef _WIN32
    if(ToolInfo::IsCustomerVer())
    {
        IniItem checkItem("option.ini", "Download", "CheckLibDAMatch");
        bool libDAMatchCheck = checkItem.GetBooleanValue();
        ui->checkBox_libDAMatch->setChecked(libDAMatchCheck ? Qt::Checked : Qt::Unchecked);
    }
    else
        ui->checkBox_libDAMatch->setChecked(Qt::Unchecked);
#else
    ui->checkBox_libDAMatch->setEnabled(false);
    ui->checkBox_libDAMatch->setVisible(false);
#endif
}

void OptionDialog::SaveGeneralSettings()
{
    IniItem item("option.ini", "Trace", "EnableTrace");

    if(ui->checkBox_enableTracing->checkState() == Qt::Checked)
        item.SaveBooleanValue(true);
    else
        item.SaveBooleanValue(false);

    int index = ui->comboBox__LogDate->currentIndex();

    item.SetItemName("CleanOlder");
    item.SaveIntValue(index);

    index = ui->comboBox_da_log_level->currentIndex();
    item.SetItemName("DALogLevel");
    item.SaveIntValue(index);

    index = ui->comboBox_da_log_channel->currentIndex();
    item.SetItemName("DALogChannel");
    item.SaveIntValue(index);

    QString sLogPath = ui->lineEdit_logPath->text();
    item.SetItemName("LogPath");
    item.SaveStringValue(sLogPath);

#ifdef _WIN32
    if(ToolInfo::IsCustomerVer())
    {
        bool libDAMatchCheck = ui->checkBox_libDAMatch->checkState() == Qt::Checked;
        IniItem checkItem("option.ini", "Download", "CheckLibDAMatch");
        checkItem.SaveBooleanValue(libDAMatchCheck);
    }
#endif
}

#define APCore_Conn_BromUART    0
#define APCore_Conn_BromUSB     1
#define APCore_Conn_BromHS      2

void OptionDialog::EnableConnControl(int index)
{
    if(index == APCore_Conn_BromUART)
    {
        EnableControl(true);
    }
    else
    {
        EnableControl(false);
    }
}

void OptionDialog::EnableControl(bool enable)
{
    ui->comboPort->setEnabled(enable);
    ui->comboRate->setEnabled(enable);

    ui->groupBox_speed->setEnabled(!enable);
    ui->groupBox_battery->setEnabled(!enable);
    ui->comboTimeout->setEnabled(!enable);
}

void OptionDialog::LoadConnSettings()
{
    IniItem item("option.ini", "Conn", "Type");

    int index = item.GetIntValue();

    int conn_type = index;

    if(index == APCore_Conn_BromUART )
        ui->radioUART->setChecked(true);
    else
        ui->radioUSB->setChecked(true);

    item.SetItemName("ComPort");
    index = item.GetIntValue();
    if(index != -1)
        ui->comboPort->setCurrentIndex(index);
    else
        ui->comboPort->setCurrentIndex(0);

    item.SetItemName("BaudRate");
    index = item.GetIntValue();
    if(index != -1)
        ui->comboRate->setCurrentIndex(index);
    else
        ui->comboRate->setCurrentIndex(3);
    item.SetItemName("USBHighSpeed");
    bool selec = item.GetBooleanValue();
    if(selec)
    {
        ui->radioSpeedFull->setChecked(false);
        ui->radioSpeedHigh->setChecked(true);
    }
    else
    {
        ui->radioSpeedHigh->setChecked(false);
        ui->radioSpeedFull->setChecked(true);
    }

    item.SetItemName("SupportAutoDetect");
    selec = item.GetBooleanValue();
    if(selec)
        ui->radioButton_Autodetect->setVisible(true);
    else
        ui->radioButton_Autodetect->setVisible(false);


    item.SetItemName("Battery");
    int check = item.GetIntValue();
    if(check == APCore::WithBattery)
    {
        ui->radioButton_WithBattery->setChecked(true);
    }
    else if(check == APCore::WithoutBattery)
    {
        ui->radioButton_WithoutBattery->setChecked(true);
    }
    else if (selec)
    {
        ui->radioButton_Autodetect->setChecked(true);
    }
    else
    {
        ui->radioButton_WithoutBattery->setChecked(true);
    }

    item.SetItemName("Timeout");
    index = item.GetIntValue();
    if(index != -1)
        ui->comboTimeout->setCurrentIndex(index);
    else
        ui->comboTimeout->setCurrentIndex(3);

    EnableConnControl(conn_type);
}

void OptionDialog::SaveConnSettings()
{
    IniItem item("option.ini", "Conn", "Type");

    int index = 0;

    if(ui->radioUART->isChecked())
    {
        item.SaveIntValue(APCore_Conn_BromUART);

        index = ui->comboPort->currentIndex();

        item.SetItemName("ComPort");

        item.SaveIntValue(index);

        index = ui->comboRate->currentIndex();

        item.SetItemName("BaudRate");

        item.SaveIntValue(index);
    }
    else
    {
        item.SaveIntValue(APCore_Conn_BromUSB);

        item.SetItemName("USBHighSpeed");

        if(ui->radioSpeedHigh->isChecked())
            item.SaveBooleanValue(true);
        else
            item.SaveBooleanValue(false);

        item.SetItemName("SupportAutoDetect");
        if(ui->radioButton_Autodetect->isHidden())
            item.SaveBooleanValue(false);
        else
            item.SaveBooleanValue(true);

        item.SetItemName("Battery");

        if(ui->radioButton_WithBattery->isChecked())
            item.SaveIntValue(APCore::WithBattery);
        else if(ui->radioButton_WithoutBattery->isChecked())
            item.SaveIntValue(APCore::WithoutBattery);
        else
            item.SaveIntValue(APCore::AutoDetect);

        item.SetItemName("Timeout");
        index = ui->comboTimeout->currentIndex();
        item.SaveIntValue(index);
        LOGI("comboTimeout index(%d) saved.", index);
    }
}

void OptionDialog::InitUARTInfo()
{
    AsyncUARTComPortInfo *get_uart_info_task = new AsyncUARTComPortInfo();
    qRegisterMetaType<std::list<std::string> >("std::list<std::string>");
    connect(get_uart_info_task, SIGNAL(signal_send_uart_infos(const std::list<std::string> &)),
            this, SLOT(slot_get_uart_com_ports(const std::list<std::string> &)));
    connect(get_uart_info_task, SIGNAL(finished()), get_uart_info_task, SLOT(deleteLater()));
    get_uart_info_task->start();
}

void OptionDialog::slot_get_uart_com_ports(const std::list<std::string> &uart_com_ports)
{
    ui->comboPort->clear();

    std::list<std::string>::const_iterator it = uart_com_ports.begin();

    for(; it != uart_com_ports.end(); ++it)
    {
        QString str = QString::fromStdString(*it);
        //remove absoluted path charactors, for windows, windows com port name will be "\\.\COMxx".
#ifdef _WIN32
        str.replace("\\\\.\\", "");
#endif
        ui->comboPort->addItem(str);
    }
}

#ifdef _WIN32
bool OptionDialog::winEvent(MSG *message, long *result)
{
    if (this->isVisible() && message->message == WM_DEVICECHANGE)
    {
        bool b_need_refresh_uart_ports = false;
        switch(message->wParam)
        {
        case DBT_DEVICEARRIVAL:
        {
            LOGI("USB Device plug in.");
            b_need_refresh_uart_ports = true;
        }
            break;

        case DBT_DEVICEREMOVECOMPLETE:
        {
            LOGI("USB Device Removed.");
            b_need_refresh_uart_ports = true;
        }
            break;
        }

        if (b_need_refresh_uart_ports)
        {
            InitUARTInfo();
            *result = 1;
            return true;
        }
    }

    return QDialog::winEvent(message, result);
}
#endif

void OptionDialog::showEvent(QShowEvent *event)
{
    if (ui->radioUART->isChecked())
    {
        InitUARTInfo();
    }
    QDialog::showEvent(event);
}

QSharedPointer<APCore::ConnSetting> OptionDialog::CreateConnSetting()
{
    if(ui->radioUART->isChecked())
    {
        QString port_str = ui->comboPort->currentText();
#ifdef _WIN32
        port_str = QString("\\\\.\\") + port_str;
#endif
        QSharedPointer<APCore::ConnBromUARTSetting> uart_setting(
                    new APCore::ConnBromUARTSetting(port_str.toStdString()));

        uart_setting->set_cb_dl_da_init(MainWindowCallback::DownloadDAInit);
        uart_setting->set_cb_dl_da(MainWindowCallback::DownloadDAProgress);
        QString baudrate_str = ui->comboRate->currentText();
        unsigned int baudrate = atoi(baudrate_str.toStdString().c_str());
        uart_setting->set_baudrate(baudrate);
        unsigned int log_level = ui->comboBox_da_log_level->currentIndex();
        uart_setting->set_da_log_level(log_level);
        unsigned int log_channel = ui->comboBox_da_log_channel->currentIndex();
        uart_setting->set_da_log_channel(log_channel);
        uart_setting->set_storage_life_cycle_check(this->CheckStorageLifeCycle());
        return uart_setting;
    }
    else
    {
        bool high_speed = ui->radioSpeedHigh->isChecked();
        APCore::USBPower power = GetPowerControl();

        QSharedPointer<APCore::ConnBromUSBSetting> usb_setting(
                    new APCore::ConnBromUSBSetting(
                        high_speed?APCore::USBHighSpeed:APCore::USBFullSpeed,
                        power));
        QString timeout_str = ui->comboTimeout->currentText();
        int timeout = atoi(timeout_str.toStdString().c_str());
        usb_setting->set_timeout(timeout*60*1000);

        usb_setting->set_cb_dl_da_init(MainWindowCallback::DownloadDAInit);
        usb_setting->set_cb_dl_da(MainWindowCallback::DownloadDAProgress);
        usb_setting->set_cb_checkusbstatus_init(MainWindowCallback::CheckUSBStatusInit);
        unsigned int log_level = ui->comboBox_da_log_level->currentIndex();
        usb_setting->set_da_log_level(log_level);
        unsigned int log_channel = ui->comboBox_da_log_channel->currentIndex();
        usb_setting->set_da_log_channel(log_channel);
        usb_setting->set_storage_life_cycle_check(this->CheckStorageLifeCycle());
        return usb_setting;
    }
}

QSharedPointer<APCore::LogInfoSetting> OptionDialog::CreateLogInfoSetting()
{
    QSharedPointer<APCore::LogInfoSetting> log_setting(new APCore::LogInfoSetting());
    log_setting->setLogOn(ui->checkBox_enableTracing->checkState() == Qt::Checked);
    log_setting->setLogPath(ui->lineEdit_logPath->text().toStdString());
    //convert the negative hour value to positive, for user convenience
    log_setting->setCleanHours(-GetHoursFromDateEnum());
    return log_setting;
}

QString OptionDialog::GetDescription()
{
    QString info;
    if(ui->radioUART->isChecked())
    {
        info.append("UART: ").append(ui->comboPort->currentText())
                .append(", ").append(ui->comboRate->currentText());
    }
    else
    {
        info.append("USB: ");

        info.append("DA Download All");
        if(ui->radioSpeedHigh->isChecked())
        {
            info.append("(high speed,");
        }
        else
        {
            info.append("(full speed,");
        }

        if(ui->radioButton_WithBattery->isChecked())
        {
            info.append("with bat)");
        }
        else if(ui->radioButton_WithoutBattery->isChecked())
        {
            info.append("without bat)");
        }
        else if(ui->radioButton_Autodetect->isChecked())
        {
            info.append("auto detect)");
        }
    }

    return info;

}

QString OptionDialog::GetUSBSpeed()
{
    return (ui->radioSpeedHigh->isChecked()) ?
           QString("High Speed"):QString("Full Speed");
}

void OptionDialog::onPlatformChanged()
{
    ReInitItems();
}

void OptionDialog::ShowPhysicalFormatItem(bool show)
{
    ui->checkBox_physicalFormat->setVisible(show);
}

void OptionDialog::ShowCheckRPMBItem(bool show)
{
    ui->groupBox_rpmb_setting->setVisible(show);
}

bool OptionDialog::isRPMBChecked()
{
    return ui->checkBox_rpmb->checkState() == Qt::Checked;
}

void OptionDialog::onToggleUSB_UART(bool toggled)
{
    QRadioButton *radio = qobject_cast<QRadioButton*>(sender());
    if(radio == ui->radioUSB)
    {
        ui->radioUART->setChecked(!toggled);
    }
    else if(radio == ui->radioUART)
    {
        ui->radioUSB->setChecked(!toggled);
        if (toggled)
        {
            InitUARTInfo();
        }
    }

    EnableControl(ui->radioUART->isChecked());
}

APCore::USBPower OptionDialog::GetPowerControl()
{
     APCore::USBPower ret = APCore::AutoDetect;

     if(ui->radioButton_Autodetect->isChecked())
     {
         ret = APCore::AutoDetect;
     }

     if(ui->radioButton_WithBattery->isChecked())
     {
         ret = APCore::WithBattery;
     }

     if(ui->radioButton_WithoutBattery->isChecked())
     {
         ret = APCore::WithoutBattery;
     }

     return ret;
}

void OptionDialog::SetPowerSetting(bool autoDetect)
{
    if(autoDetect)
        ui->radioButton_Autodetect->setChecked(true);
    else
        ui->radioButton_WithBattery->setChecked(true);

}

void OptionDialog::SetPlatformSetting(const QSharedPointer<APCore::PlatformSetting> &platform_setting)
{
     this->platform_setting = platform_setting;
    // ReInitItems();
}

void OptionDialog::ReInitItems()
{
    bool isSupportAutoDetect = this->platform_setting->getPlatformConfig().supportDLAutoDetectBattery();
    if(ui->radioButton_Autodetect->isHidden() && isSupportAutoDetect)
    {
       LOG("old flatform don't support auto detect, but new flatform support it.");
       ui->radioButton_Autodetect->setVisible(true);
       ui->radioButton_Autodetect->setChecked(true);
    }
    else if (ui->radioButton_Autodetect->isChecked() && !isSupportAutoDetect)
    {
       LOG("old flatform auto detect checked, but new flatform don't support it.");
       ui->radioButton_Autodetect->setVisible(false);
       ui->radioButton_WithoutBattery->setChecked(true);
    }

    bool isPhysicalRBFMTVisible(false);
    isPhysicalRBFMTVisible = (main_window_->main_controller()->GetPlatformSetting()->getFlashToolStorageConfig().IsNandOperation())
                        &&( main_window_->main_controller()->advance_on());
    ShowPhysicalFormatItem(isPhysicalRBFMTVisible);

    std::string platform_name = main_window_->main_controller()->GetPlatformSetting()->getLoadPlatformName();
    if(FlashTool_Chip_isOldArch(platform_name.c_str()))
    {
        ui->groupBox_DA_log->setVisible(false);
    }
    else
    {
        ui->groupBox_DA_log->setVisible(true);
    }

    bool isCheckRPMBVisible = main_window_->main_controller()->GetPlatformSetting()->is_check_rpmb_support()
            && main_window_->main_controller()->advance_on();
    if(!isCheckRPMBVisible)
    {
        IniItem item("option.ini", "Download", "CheckRPMB");
        item.SaveBooleanValue(false);
    }
    ShowCheckRPMBItem(isCheckRPMBVisible);
}

void OptionDialog::on_treeWidget_currentItemChanged(
        QTreeWidgetItem *current, QTreeWidgetItem * /*previous*/)
{
    int index = ui->treeWidget->indexOfTopLevelItem(current);

    if(index < 0)
    {
       QTreeWidgetItem *parent = current->parent();
       index = parent->indexOfChild(current);

       index += 4;
    }

    switch(index)
    {
    case 0:
        ui->stackedWidget_view->setCurrentWidget(ui->page_General);
        break;

    case 1:
        ui->stackedWidget_view->setCurrentWidget(ui->page_Conn);
        break;

    case 2:
        ui->stackedWidget_view->setCurrentWidget(ui->page_Download);
        break;

    case 3:
        ui->stackedWidget_view->setCurrentWidget(ui->page_Update);
        break;

    case 4:
        ui->stackedWidget_view->setCurrentWidget(ui->page_Appearance);
        break;

    case 5:
        ui->stackedWidget_view->setCurrentWidget(ui->page_Keys);
        break;

    case 6:
        ui->stackedWidget_view->setCurrentWidget(ui->page_Language);
        break;

    case 7:
        ui->stackedWidget_view->setCurrentWidget(ui->page_StartupandShutdown);
        break;

    case 8:
        ui->stackedWidget_view->setCurrentWidget(ui->page_Tracing);
        break;

    default:
        ui->stackedWidget_view->setCurrentWidget(ui->page_General);
        break;
    }
}

// Appearance page

void OptionDialog::on_comboBoxTheme_currentIndexChanged(int index)
{
    ShowCustomThemeSetting(false);

    switch(index)
    {
#ifdef _WIN32
    case 0:
        QApplication::setStyle(new QWindowsXPStyle());
        break;

    case 1:
        QApplication::setStyle(new QWindowsVistaStyle());
        break;

    case 2:
        QApplication::setStyle(new QPlastiqueStyle());
        break;

    case 3:
        QApplication::setStyle(new QMotifStyle());
        break;

    case 4:
        main_window_->setStyleSheet("background-image:url(:/images/background.jpg);");
        break;

    case 5:
        QApplication::setStyle(new QWindowsStyle);

        ShowCustomThemeSetting(true);

        break;
#else
    case 0:
        QApplication::setStyle(new QPlastiqueStyle());
        break;

    case 1:
        QApplication::setStyle(new QMotifStyle());
        break;

    case 2:
        main_window_->setStyleSheet("background-image:url(:/images/background.jpg);");
        break;

    case 3:
        QApplication::setStyle(new QWindowsStyle);

        ShowCustomThemeSetting(true);

        break;
#endif
    }

#ifdef _WIN32
    if(index != 4)
        main_window_->setStyleSheet("");
#else
    if(index != 2)
        main_window_->setStyleSheet("");
#endif
}

void OptionDialog::InitThemeItems()
{
    QStringList itemList;

#ifdef _WIN32
    itemList << "Window XP" << "Window Vista"
             << "Plastique" << "Motif"
             << "Classic";
#else
    itemList << "Plastique" << "Motif";
#endif

    ui->comboBoxTheme->addItems(itemList);

    ui->comboBoxTheme->setCurrentIndex(0);
}

void OptionDialog::ShowCustomThemeSetting(bool show)
{
    ui->label_qssFile->setVisible(show);
    ui->lineEdit_qssFilePath->setVisible(show);
    ui->pushButton_loadQssFile->setVisible(show);
}

void OptionDialog::on_comboBox_Language_activated(int index)
{
    IniItem item("option.ini", "Language", "CurrentLang");

    item.SaveIntValue(index);

    main_window_->ChangeLanguage(index);
}

void OptionDialog::UpdateUI()
{
    ui->retranslateUi(this);
}

void OptionDialog::SetLabelText(int index)
{
    ui->comboBox_Language->setCurrentIndex(index);
}

void OptionDialog::on_checkBox_ShowWelcome_clicked()
{
    IniItem item("option.ini", "WelcomePage", "ShowWelcome");

    if(ui->checkBox_ShowWelcome->checkState() == Qt::Checked)
        item.SaveBooleanValue(true);
    else
        item.SaveBooleanValue(false);
}

void OptionDialog::on_checkBox_physicalFormat_clicked()
{
    IniItem item("option.ini", "Download", "PhyFmt");

    if(ui->checkBox_physicalFormat->checkState() == Qt::Checked)
        item.SaveBooleanValue(true);
    else
        item.SaveBooleanValue(false);
}

void OptionDialog::on_checkBox_usbchksum_toggled(bool checked)
{
    LOGD("Enable DA USB Checksum(%d)", checked);

    IniItem item("option.ini", "Download", "DAChksum");

    int chksum = checked ? DHNDL_FLAG_CHKSUM_LEVEL_USB : DHNDL_FLAG_CHKSUM_LEVEL_NONE;

    if(ui->checkBox_emmcchksum->checkState() == Qt::Checked)
        chksum |= DHNDL_FLAG_CHKSUM_LEVEL_STORAGE;

    item.SaveIntValue(chksum);

    emit signal_enable_DAChksum(chksum);
}

void OptionDialog::on_checkBox_emmcchksum_toggled(bool checked)
{
    LOGD("Enable DA EMMC Checksum(%d)", checked);

    IniItem item("option.ini", "Download", "DAChksum");

    int chksum = checked ? DHNDL_FLAG_CHKSUM_LEVEL_STORAGE : DHNDL_FLAG_CHKSUM_LEVEL_NONE;

    if(ui->checkBox_usbchksum->checkState() == Qt::Checked)
        chksum |= DHNDL_FLAG_CHKSUM_LEVEL_USB;

    item.SaveIntValue(chksum);

    emit signal_enable_DAChksum(chksum);
}

void OptionDialog::slot_SetUARTBaudrateIndex(unsigned int index)
{
    ui->comboRate->setCurrentIndex(index);
}

QString OptionDialog::GetBindFromVector(const int index)
{
    LOG("get binding shortcut(%s) from index (%d)", bind_vector[index].c_str(), index);
    return bind_vector[index].c_str();
}

void OptionDialog::SetCurrentKey(int row)
{
    QString key = ui->tableWidget->item(row, 1)->text();

    QStringList keyList = key.split(" ");

    QString modify = keyList.at(0);

    int index = -1;

    if(modify.compare("Alt") == 0)
        index = 1;
    else if(modify.compare("Shift") == 0)
        index = 3;
    else if(modify.compare("Ctrl") == 0)
    {
        if(keyList.contains("Alt"))
        {
            if(keyList.contains("Shift"))
                index = 6;
            else
                index = 4;
        }
        else if(keyList.contains("Shift"))
            index = 5;
        else
            index = 2;
    }
    else
        index = 0;

    ui->comboBox_KeyModify->setCurrentIndex(index);

    QString keyBorad = keyList.at(keyList.size() - 1);

    bool ok;

    if(keyBorad.length() == 2)
    {
        index = keyBorad.right(1).toInt(&ok) - 1;
    }
    else if(keyBorad.length() == 3)
    {
        index = keyBorad.right(2).toInt(&ok) - 1;
    }
    else if(keyBorad.at(0).isLetter())
    {
        index = keyBorad.at(0).toAscii() - 'A' + 12;
    }
    else
    {
        index = keyBorad.toInt(&ok) + 38;
    }

    ui->comboBox_key->setCurrentIndex(index);
}

void OptionDialog::ResetShortKey(int row)
{
    if(row < 0)
        return;

    QString binding = ui->comboBox_KeyModify->currentText();

    if(binding.isEmpty())
        binding = ui->comboBox_key->currentText();
    else
        binding += " + " + ui->comboBox_key->currentText();

    std::map<std::string,std::string>::iterator map_it;
    std::string std_binding = binding.toStdString();
    map_it = std::find_if(default_bind_map.begin(), default_bind_map.end(), map_value_finder(std_binding));
    if (map_it != default_bind_map.end() )
    {
        LOG("Shortcut [%s] has binded to (%s).", map_it->second.c_str(), map_it->first.c_str());
        return;
    }

    std::vector<std::string>::iterator it;
    it = std::find(bind_vector.begin(), bind_vector.end(), std_binding);
    if (it == bind_vector.end() )// Binding not used
    {
        ui->tableWidget->item(row, 1)->setText(binding);
        bind_vector[row] = std_binding;

        QSettings settings(ABS_PATH_C("key.ini"), QSettings::IniFormat);
        settings.beginWriteArray("Shortcuts");
        settings.setArrayIndex(row);
        settings.setValue("Binding",binding);
        settings.setArrayIndex(bind_vector.size()-1);
        settings.endArray();

        LOG("row (%d) is binding [%s].", row , std_binding.c_str());
    }
    else
    {
        LOG("Shortcut [%s] has been binded before.", std_binding.c_str());
    }

}

void OptionDialog::SetShortcut(int row)
{
    static short_cut shortCuts[] =
    {
        {WINDOW, -1},
        {OPTIONS, -1},
        {HELP, CMD_CONTENTS},
        {HELP, CMD_INDEX},
        {DOWNLOAD, DL_BEGIN},
        {DOWNLOAD, DL_STOP},
        {DOWNLOAD, DL_CERT},
        {FORMAT, FMT_START},
        {FORMAT, FMT_STOP},
        {READBACK, RB_ADD},
        {READBACK, RB_REMVOE},
        {READBACK, RB_START},
        {READBACK, RB_STOP},
        {MEMORYTEST, MT_START},
        {MEMORYTEST, MT_STOP},
        {PARAMETER, PARA_UPDATE},
        {PARAMETER, PARA_STOP},
        {WRITEMEMORY, WM_START},
        {WRITEMEMORY, WM_STOP},
        {BAT, BROM_DOWNLOAD},
        {BAT, BROM_STOP},
        {BAT, BORM_JUMP}
    };

    if(row < 0)
        return;

    short_cut cur = shortCuts[row];

    main_window_->SetShortCut(cur.category, cur.cmd, row);
}

int OptionDialog::GetChecksumLevel()
{
    int usbChecked = (Qt::Checked == ui->checkBox_usbchksum->checkState());
    int storageChecked = (Qt::Checked == ui->checkBox_emmcchksum->checkState());
    return (usbChecked<<0) | (storageChecked <<1);
}

bool OptionDialog::LibDAMatchChecked()
{
    return (Qt::Checked == ui->checkBox_libDAMatch->checkState());
}

void OptionDialog::on_tableWidget_currentCellChanged(int currentRow, int /*currentColumn*/, int previousRow, int /*previousColumn*/)
{
    QString key = ui->tableWidget->item(currentRow, 0)->text();

    ui->lineEdit_commandName->setText(key);

    QString key_description = ui->tableWidget->item(currentRow, 2)->text();

    ui->lineEdit_KeyDescription->setText(key_description);

    ui->lineEdit_binding->setText(GetBindFromVector(currentRow));

    ResetShortKey(previousRow);

    SetCurrentKey(currentRow);

    SetShortcut(previousRow);
}

QString OptionDialog::GetKeyboardModify(int row)
{
    QString ret = QString::fromStdString(bind_vector[row]);
    ret = ret.remove(" ");

    return ret;
}

void OptionDialog::on_checkBox_RestoreStartup_clicked()
{
     QSettings settings(ABS_PATH_C("option.ini"), QSettings::IniFormat);

     settings.beginGroup("RestoreState");

     if(ui->checkBox_RestoreStartup->checkState() == Qt::Checked)
         settings.setValue("Restore", QVariant(true));
     else
         settings.setValue("Restore", QVariant(false));

     settings.endGroup();

     settings.sync();
}

void OptionDialog::on_checkBox_clicked()
{
    IniItem item("option.ini", "CheckUpdate", "Update");

    if(ui->checkBox->checkState() == Qt::Checked)
        item.SaveBooleanValue(true);
    else
        item.SaveBooleanValue(false);
}

void OptionDialog::startCleanThread()
{
    QString path = ui->lineEdit_logPath->text();
    m_workerThread = new LogCleanThread(path, GetHoursFromDateEnum());
    m_workerThread->start(QThread::LowPriority);
}

qint64 OptionDialog::GetHoursFromDateEnum() const
{
    int nDateIndex = ui->comboBox__LogDate->currentIndex();
    QDateTime date;
    switch (nDateIndex)
    {
    case 0:
        date = QDateTime::currentDateTime().addDays(-1);
        break;
    case 1:
        date = QDateTime::currentDateTime().addDays(-7);
        break;
    case 2:
        date = QDateTime::currentDateTime().addMonths(-1);
        break;
    case 3:
        date = QDateTime::currentDateTime().addMonths(-3);
        break;
    default:
        Q_ASSERT(false);
    }
    qint64 nDays = QDateTime::currentDateTime().daysTo(date);
    return nDays * 24;
}

bool OptionDialog::CheckStorageLifeCycle() const
{
    return ui->checkBox_StorageLifeCycle->checkState() == Qt::Checked;
}

void OptionDialog::InitUpdateOptions()
{
    bool bNeedRemove = true;
#ifdef _WIN32
    if (!ToolInfo::IsCustomerVer())
    {
        bNeedRemove = false;
    }
#endif
    if (bNeedRemove)
    {
        QList<QTreeWidgetItem *> updateItems = ui->treeWidget->findItems("Install/Update", Qt::MatchFixedString | Qt::MatchCaseSensitive);
        Q_ASSERT(updateItems.count() == 1);
        updateItems[0]->setHidden(true);
        for (size_t i = 0; i < ui->stackedWidget_view->count(); ++i)
        {
            QWidget * cur_widget = ui->stackedWidget_view->widget(i);
            Q_ASSERT(NULL != cur_widget);
            if (cur_widget->objectName() == "page_Update")
            {
                ui->stackedWidget_view->removeWidget(cur_widget);
                break;
            }
        }
    }
}

void OptionDialog::on_checkBox_enableTracing_clicked()
{
    if(ui->checkBox_enableTracing->checkState() == Qt::Checked)
        main_window_->UpdateWindowTitle(true);
    else
        main_window_->UpdateWindowTitle(false);
}

void OptionDialog::on_checkBox_rpmb_clicked()
{
    IniItem item("option.ini", "Download", "CheckRPMB");

    if(ui->checkBox_rpmb->checkState() == Qt::Checked)
        item.SaveBooleanValue(true);
    else
        item.SaveBooleanValue(false);
}

void OptionDialog::on_btn_logPath_clicked()
{
    Logger::SetSPFlashToolLogFolder();
    QString sCurrentLogPath = QString::fromLocal8Bit(Logger::GetSPFlashToolLogFolder().c_str());
    QString sSelectedLogPath = QFileDialog::getExistingDirectory(this, LoadQString(LANGUAGE_TAG, IDS_STRING_SELECT_LOG_PATH),
                                                                 sCurrentLogPath, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!sSelectedLogPath.trimmed().isEmpty())
    {
        sSelectedLogPath = QDir::toNativeSeparators(sSelectedLogPath);
        ui->lineEdit_logPath->setText(sSelectedLogPath);
    }
}

void OptionDialog::on_checkBox_StorageLifeCycle_clicked()
{
    IniItem item("option.ini", "Download", "StorageLifeCycleCheck");

    if(ui->checkBox_StorageLifeCycle->checkState() == Qt::Checked)
        item.SaveBooleanValue(true);
    else
        item.SaveBooleanValue(false);
}

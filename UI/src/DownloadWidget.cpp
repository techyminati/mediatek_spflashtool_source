#include "DownloadWidget.h"
#include "ui_DownloadWidget.h"
#include "MainWindow.h"
#include "CheckHeader.h"
#include "ICallback.h"
#include "ProcessingDialog.h"
#include "../../Utility/Utils.h"
#include "../../Utility/FileUtils.h"
#include "../../Utility/constString.h"
#include "../../Utility/IniItem.h"
#include "../../Host/Inc/RuntimeMemory.h"
#include "../../Utility/version.h"
#include "../../Err/Exception.h"

#include <QtDebug>
#include <QtGui>
#include <QTextCodec>
#include <QRegExp>
#include <algorithm>

const static std::map<int, int>::value_type init_values[] = {
    std::map<int, int>::value_type(FORMAT_ALL_DOWNLOAD, IDS_STRING_SCENE_FMTALLDL),
    std::map<int, int>::value_type(FIRMWARE_UPGRADE, IDS_STRING_SCENE_FIRMWAREUPGRADE),
    std::map<int, int>::value_type(DOWNLOAD_ONLY, IDS_STRING_SCENE_DOWNLOADONLY),
    std::map<int, int>::value_type(WIPE_DATA, IDS_STRING_SCENE_WIPE_DATA)
};

const static std::map<int, int> g_download_scene_string_tag_map(init_values, init_values + 4);

DownloadWidget::DownloadWidget(QTabWidget *parent, MainWindow *window) :
    TabWidgetBase(2, tr("&Download"), parent),
    main_window_(window),
    ui_(new Ui::DownloadWidget),
    scatterFile_historyList_(),
    authFile_historyList_(),
    certFile_historyList_(),
    scene_(UNKOWN_DOWNLOAD_SCENE),
    header_(new CheckHeader(Qt::Horizontal, this)),
    storage_(HW_STORAGE_EMMC),
    mStopFlag(0),
    mCurLocationRow(-1)
{
    memset(m_rsc_p_info, 0, sizeof(m_rsc_p_info));
    m_rsc_cnt = 0;

    ui_->setupUi(this);

    setAcceptDrops(true);

    ui_->tableWidget->setHorizontalHeader(header_);

    QObject::connect(header_,SIGNAL(sectionClicked(int)),
                     this, SLOT(slot_OnHeaderView_click(int)));

    connect(this, SIGNAL(signal_load_finished(bool)), SLOT(slot_OnLoadByScatterEnd(bool)));
    connect(this, SIGNAL(signal_load_failed()), SLOT(slot_OnLoadByScatterFailed()));
    connect(this, SIGNAL(signal_load_canceled()), SLOT(slot_OnLoadByScatterCanceled()));

    connect(this, SIGNAL(signal_load_rom_done()),SLOT(slot_OnLoadRomDone()));
    connect(this, SIGNAL(signal_load_rom_failed()), SLOT(slot_OnLoadRomFailed()));
    connect (main_window_->processing_dialog(), SIGNAL(user_cancel_processing()),this, SLOT(slot_OnUserCancelLoadScatter()));

    ui_->tableWidget->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);

    main_window_->main_controller()->GetPlatformSetting()->addObserver(this);
    main_window_->scatter_observer()->addObserver(this);

    updateDownloadSceneByConfig();

    //LoadDefaultDA();

    ShowUnavailableItems(false);

    ShowRSCItem(false);

    UpdateImageList(true, scene_);

    header_->SetChecked(true);
    header_->SetCheckedEnabled(this->checkStateEnabledByConfig());

    ui_->tableWidget->setColumnHidden(columnRegion, true);
}

DownloadWidget::~DownloadWidget()
{
    IniItem item("history.ini", "RecentOpenFile", "scatterHistory");

    item.SaveStringListValue(scatterFile_historyList_);

    item.SetItemName("authHistory");
    item.SaveStringListValue(authFile_historyList_);

    UpdateCustomSceneSelectItems();

    delete ui_;
    ui_ = NULL;
}

void DownloadWidget::on_pushButton_downloadAgent_clicked()
{
    IniItem item("history.ini", "LastDAFilePath", "lastDir");

    QString last_dir = item.GetStringValue();

    if(last_dir.isEmpty())
        last_dir = FileUtils::GetAppDirectory().c_str();

    QString file_name = QFileDialog::getOpenFileName(
                this,
                LoadQString(LANGUAGE_TAG, IDS_STRING_OPEN_DA),
                last_dir,
                LoadQString(LANGUAGE_TAG, IDS_STRING_BIN));

    if(!file_name.isEmpty())
    {
        file_name = QDir::toNativeSeparators(file_name);

        item.SaveStringValue(file_name);

        default_da = file_name;

        //load DA
        if(main_window_->main_controller()->LoadDA(file_name))
        {
            ui_->lineEdit_agentFilePath->setText(file_name);
        }
        else
        {
            ui_->lineEdit_agentFilePath->setText("");
        }
    }
}

void DownloadWidget::on_pushButton_scatterLoading_clicked()
{
    IniItem item("history.ini", "RecentOpenFile", "lastDir");

    QString last_dir = item.GetStringValue();

    QString file_name = QFileDialog::getOpenFileName(
                this,
                LoadQString(LANGUAGE_TAG, IDS_STRING_OPEN_SCATTER),
                last_dir,
                LoadQString(LANGUAGE_TAG, IDS_STRING_MAP_FILE));


    if(!file_name.isEmpty())
    {
        item.SaveStringValue(QDir::toNativeSeparators(file_name));
        UpdateScatterFile(file_name);
    }
}

void DownloadWidget::LoadAuthFile(const QString& filename)
{
    QString file_name = filename;

    file_name = QDir::toNativeSeparators(file_name);
    LOG("file_name: %s", file_name.toLocal8Bit().constData());

    if(main_window_->main_controller()->LoadAuthFile(file_name))
    {
        int index = authFile_historyList_.indexOf(file_name);
        //always set select item to index 0
        //1. new, insert at 0
        //2. old, not at 0, move to 0
        //3. old, at 0, do nothing
        if(index < 0)
        {
            authFile_historyList_.push_front(file_name);
        }
        else if(index != 0)
        {
            authFile_historyList_.move(index, 0);
        }
        authFile_historyList_.append("");
        authFile_historyList_.removeDuplicates();

        ui_->comboBox_authFilePath->clear();
        ui_->comboBox_authFilePath->insertItems(0, authFile_historyList_);
        ui_->comboBox_authFilePath->setCurrentIndex(0);
    }
    else
    {
        authFile_historyList_.removeFirst();
        authFile_historyList_.append("");
        authFile_historyList_.removeDuplicates();

        ui_->comboBox_authFilePath->clear();
        ui_->comboBox_authFilePath->insertItems(0, authFile_historyList_);
        ui_->comboBox_authFilePath->setCurrentIndex(-1);
    }

    LOG("The authFile history list size is %d.\n", authFile_historyList_.size());
}

void DownloadWidget::LoadLastAuthFile()
{
    IniItem item("history.ini", "RecentOpenFile", "authHistory");
    authFile_historyList_ = item.GetStringListValue();
    if(!authFile_historyList_.isEmpty())
    {
        QString lastAuth = authFile_historyList_[0];
        LoadAuthFile(lastAuth);
    }
}

void DownloadWidget::on_pushButton_authFile_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(
                this,
                LoadQString(LANGUAGE_TAG, IDS_STRING_OPEN_AUTHFILE),
                "",
                LoadQString(LANGUAGE_TAG, IDS_STRING_AUTH_BIN));

    LoadAuthFile(file_name);
}

void DownloadWidget::LoadCertFile(const QString& filename)
{
    QString file_name = filename;

    file_name = QDir::toNativeSeparators(file_name);
    LOG("cert file_name: %s", file_name.toLocal8Bit().constData());

    if(main_window_->main_controller()->LoadCertFile(file_name))
    {
        int index = certFile_historyList_.indexOf(file_name);
        //always set select item to index 0
        //1. new, insert at 0
        //2. old, not at 0, move to 0
        //3. old, at 0, do nothing
        if(index < 0)
        {
            certFile_historyList_.push_front(file_name);
        }
        else if(index != 0)
        {
            certFile_historyList_.move(index, 0);
        }
        certFile_historyList_.append("");
        certFile_historyList_.removeDuplicates();

        ui_->comboBox_certFilePath->clear();
        ui_->comboBox_certFilePath->insertItems(0, certFile_historyList_);
        ui_->comboBox_certFilePath->setCurrentIndex(0);
    }
    else
    {
        certFile_historyList_.removeFirst();
        certFile_historyList_.append("");
        certFile_historyList_.removeDuplicates();

        ui_->comboBox_certFilePath->clear();
        ui_->comboBox_certFilePath->insertItems(0, certFile_historyList_);
        ui_->comboBox_certFilePath->setCurrentIndex(-1);
    }

    LOG("The certFile history list size is %d.\n", certFile_historyList_.size());
}

void DownloadWidget::LoadLastCertFile()
{
    IniItem item("history.ini", "RecentOpenFile", "certHistory");
    certFile_historyList_ = item.GetStringListValue();
    if(!certFile_historyList_.isEmpty())
    {
        QString lastCert = certFile_historyList_[0];
        LoadCertFile(lastCert);
    }
}


void DownloadWidget::on_pushButton_CertFile_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(
                this,
                LoadQString(LANGUAGE_TAG, IDS_STRING_OPEN_SECFILE),
                "",
                LoadQString(LANGUAGE_TAG, IDS_STRING_SEC_BIN));

    LoadCertFile(file_name);
}

void DownloadWidget::slot_start_download()
{
    StartDownload();
}

void DownloadWidget::StartDownload()
{
    main_window_->main_controller()->SetPlatformSetting();
    main_window_->main_controller()->SetConnSetting(main_window_->CreateConnSetting());

    //check rpmb written before download
    bool enable_check_rpmb = main_window_->CheckRPMBEnable();
    if(enable_check_rpmb)
        main_window_->main_controller()->QueueAJob(main_window_->CreateCheckRPMBSetting());
    //end of check rpmb

    main_window_->main_controller()->QueueAJob(main_window_->CreateDLSetting());

    bool enable_set_boot_mode = main_window_->main_controller()->GetPlatformSetting()->is_set_boot_mode_support()
         && main_window_->main_controller()->advance_on()
         && ui_->checkbox_set_boot_mode_to_meta->checkState() == Qt::Checked;
    if(enable_set_boot_mode)
        main_window_->main_controller()->QueueAJob(main_window_->CreateSetBootModeSetting());

    if(IsRSCEnabled())
        main_window_->main_controller()->QueueAJob(main_window_->CreateRSCSetting());

    if(!ToolInfo::IsCustomerVer())
        main_window_->main_controller()->QueueAJob(main_window_->CreateWatchDogSetting());

    main_window_->main_controller()->StartExecuting(
                    new SimpleCallback<MainWindow>(main_window_,&MainWindow::DoFinished));
}

void DownloadWidget::on_pushButton_download_clicked()
{
    IniItem item("option.ini", "Download", "RiskReminder");
    bool isRiskReminderOn = item.GetBooleanValue();
    if(isRiskReminderOn)
    {
        int ristRet = flashtool_message_box(this,
                                            0,
                                            QUESTION_MSGBOX,
                                            LoadQString(LANGUAGE_TAG, IDS_STRING_WARNING),
                                            LoadQString(LANGUAGE_TAG, IDS_STRING_DLRISK_REMINDER),
                                            LoadQString(LANGUAGE_TAG, IDS_STRING_CONTINUE),
                                            LoadQString(LANGUAGE_TAG, IDS_STRING_CANCEL));
        if(ristRet == 1) //cancel
            return;
    }

    if(ValidateBeforeDownload())
    {
        StartDownload();
        main_window_->LockOnUI();
        main_window_->GetOkDialog()->setWindowTitle(LoadQString(LANGUAGE_TAG, IDS_STRING_DOWNLOAD_OK));
    }
}

void DownloadWidget::on_toolButton_Certification_clicked()
{
    if(ValidateBeforeCertDL())
    {
        main_window_->main_controller()->SetConnSetting(main_window_->CreateConnSetting());
        main_window_->main_controller()->QueueAJob(main_window_->CreateCertDLSetting());
        main_window_->main_controller()->StartExecuting(
                    new SimpleCallback<MainWindow>(main_window_, &MainWindow::DoFinished));
        main_window_->LockOnUI();
        main_window_->GetOkDialog()->setWindowTitle(LoadQString(LANGUAGE_TAG, IDS_STRING_CERT_OK));
    }
}

void DownloadWidget::on_pushButton_stop_clicked()
{
    ui_->pushButton_stop->setEnabled(false);
    main_window_->main_controller()->StopByUser();
}

void DownloadWidget::slot_OnHeaderView_click(int index)
{
    if(index == 0 && this->checkStateEnabledByConfig())
    {
        bool checked = header_->GetChecked();

        if(!checked)
        {
            UpdateImageList(false, scene_);
        }
        else
        {
                UpdateImageList(true, scene_);
        }

        UpdateScene();
    }
}

void DownloadWidget::on_tableWidget_cellClicked(int row, int column)
{
    if (column == ColumnLocation)
    {
        choose_rom_file(row);
    }
    else if (column == ColumnEnable && checkStateEnabledByConfig())
    {
        RomEnabledChanged(row);
    }
}

void DownloadWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasFormat("text/uri-list"))
    {
        QPoint point = event->pos();

        point = ui_->tableWidget->mapFromParent(event->pos());

        QRect rect = ui_->tableWidget->geometry();

        LOG("The  table widget rect x1 = %d, y1 = %d, x2 = %d, y2 = %d\n",
            rect.x(), rect.y(), rect.width(), rect.height());

        if(rect.contains(point))
        {
            event->acceptProposedAction();
        }
    }
}

void DownloadWidget::dropEvent(QDropEvent *event)
{
    if(!ui_->tableWidget->underMouse())
    {
        return;
    }

    QList<QUrl> urls = event->mimeData()->urls();

    if(urls.isEmpty())
        return;

    QString fileName = urls.first().toLocalFile();
    if(fileName.isEmpty())
        return;

    UpdateScatterFile(fileName);
}

void DownloadWidget::OnLoadByScatterEnd()
{
    emit signal_load_finished(false);//to update UI accrossing threads.
}

void DownloadWidget::OnLoadByScatterFailed()
{
    emit signal_load_failed();
}

void DownloadWidget::OnLoadByScatterCanceled()
{
    emit signal_load_canceled();
}

void DownloadWidget::OnLoadRomDone()
{
    emit signal_load_rom_done();//to update UI accrossing threads.
}

void DownloadWidget::OnLoadRomFailed()
{
    emit signal_load_rom_failed();
}

void DownloadWidget::UpdateUIStatus()
{
    QString strPath = ui_->comboBox_scatterFilePath->currentText();
    scatterFile_historyList_.removeOne(strPath);
    ui_->comboBox_scatterFilePath->removeItem(ui_->comboBox_scatterFilePath->currentIndex());
    ui_->comboBox_scatterFilePath->setEditText("");
    ui_->comboBox_scatterFilePath->setCurrentIndex(-1);
    main_window_->ResetStatus();

    IniItem item("history.ini", "RecentOpenFile", "lastDir");
    item.SaveStringValue(tr(""));

    SetDACheckSum();
}

unsigned char DownloadWidget::GetBootMode_ComType()
{
    if(ui_->com_type_usb->isChecked())
        return 2;
    else if(ui_->com_type_uart->isChecked())
        return 1;
    else if(ui_->com_type_wifi->isChecked())
        return 3;
    else
        return 0;
}

unsigned char DownloadWidget::GetBootMode_ComId()
{
    int id = 0;
    if(!ui_->checkbox_mobile_log_on->isChecked())
        id |= 2;

    if(!ui_->checkbox_adb_on->isChecked())
        id |= 1;

    return id;
}

unsigned char DownloadWidget::GetBootMode()
{
    return (unsigned char)ui_->checkbox_set_boot_mode_to_meta->isChecked();
}

void DownloadWidget::EnableBootMode_ComType(bool enable)
{
    ui_->groupBox_com_type->setEnabled(enable);
}

void DownloadWidget::EnableBootMode_ComId(bool enable)
{
    ui_->groupBox_com_id->setEnabled(enable);
}

void DownloadWidget::Enable_groupBox_boot_mode_flag(bool enable)
{
    ui_->groupBox_boot_mode_flag->setEnabled(enable);
    ui_->groupBox_boot_mode_flag->setVisible(enable);
}

int DownloadWidget::GetRSCIndex(std::string rscProjName)
{
    for(uint i=0; i<m_rsc_cnt; i++)
    {
        if(rscProjName == std::string((char*)m_rsc_p_info[i].full_project_name))
            return m_rsc_p_info[i].dtbo_index;
    }

    return -1; //not found
}

std::string DownloadWidget::GetRSCProjectName(void)
{
    return ui_->comboBox_rsc->currentText().toLocal8Bit().constData();
}

std::string DownloadWidget::GetRSCOperatorName(std::string rscProjName)
{
    for(uint i=0; i<m_rsc_cnt; i++)
    {
        if(rscProjName == std::string((char*)m_rsc_p_info[i].full_project_name))
            return std::string((char*)m_rsc_p_info[i].op_name);
    }

    return ""; //not found
}

void DownloadWidget::Init_RSC_list(void)
{
    ui_->comboBox_rsc->clear();
    for(uint i = 0; i<m_rsc_cnt; i++)
    {
        ui_->comboBox_rsc->addItem((char*)m_rsc_p_info[i].full_project_name);
    }

    ui_->comboBox_rsc->setCurrentIndex(0);
}

void DownloadWidget::Clear_RSC_Info(void)
{
    ui_->comboBox_rsc->clear();
    memset(m_rsc_p_info, 0, sizeof(m_rsc_p_info));
    m_rsc_cnt = 0;
}

#define INSTALLED_FILES_VENDOR_NAME "installed-files-vendor.txt"
#define RSC_FILE_PATTERN "^\\s+\\d+\\s+/vendor/etc/rsc/\\S+/ro.prop$"
void DownloadWidget::Process_rsc(QString rsc_filename)
{
    if(!ToolInfo::IsCustomerVer())
    {
        //internal flashtool, check rsc.xml exist or not
        if(QFileInfo(rsc_filename).exists())
        {
            LOG("rsc exist");
            ShowRSCItem(true);
            std::string rsc = rsc_filename.toLocal8Bit().constData();
            LOG("rsc file: %s", rsc.c_str());

            try{
                unsigned int ret = FlashTool_GetRSCCnt(rsc.c_str(), &m_rsc_cnt);
                if(STATUS_OK != ret)
                {
                    char err_msg[512] = {0};
                    FlashTool_GetLastErrorMessage(NULL, err_msg);
                    flashtool_message_box(main_window_,
                                          main_window_->getConnectButton(),
                                          CRITICAL_MSGBOX,
                                          "Smart Phone Flash Tool",
                                          err_msg,
                                          "OK");
                    ui_->comboBox_rsc->clear();
                }
                LOG("rsc_cnt: %d", m_rsc_cnt);
                if(m_rsc_cnt > MAX_RSC_PROJ_CNT)
                {
                    flashtool_message_box(main_window_,
                                          main_window_->getConnectButton(),
                                          CRITICAL_MSGBOX,
                                          "Smart Phone Flash Tool",
                                          "rsc_cnt overseed MAX_RSC_PROJ_CNT (64)",
                                          "OK");
                    Clear_RSC_Info();
                    return;
                }
                if(m_rsc_cnt == 0)
                {
                    flashtool_message_box(main_window_,
                                          main_window_->getConnectButton(),
                                          CRITICAL_MSGBOX,
                                          "Smart Phone Flash Tool",
                                          "rsc_cnt is 0",
                                          "OK");
                    Clear_RSC_Info();
                    return;
                }

                ret = FlashTool_GetRSCInfo(rsc.c_str(), m_rsc_p_info, m_rsc_cnt);
                if(STATUS_OK != ret)
                {
                    char err_msg[512] = {0};
                    FlashTool_GetLastErrorMessage(NULL, err_msg);
                    flashtool_message_box(main_window_,
                                          main_window_->getConnectButton(),
                                          CRITICAL_MSGBOX,
                                          "Smart Phone Flash Tool",
                                          err_msg,
                                          "OK");
                    Clear_RSC_Info();
                    return;
                }
                Init_RSC_list();

            }catch(const BaseException& exception)
            {
                LOGE(exception.err_msg().c_str());
            }
        }
        else
        {
            LOG("rsc NOT exist");
            QDir install_files_dir = QFileInfo(rsc_filename).dir();
            QString install_files_name = install_files_dir.absoluteFilePath(INSTALLED_FILES_VENDOR_NAME);

            //installed-files-vendor.txt contains vendor/etc/rsc/xxx/ro.prop, xxx is the project name in rsc.xml
            if(FilePatternContain(install_files_name, RSC_FILE_PATTERN))
            {
                ShowRSCItem(true);
                char err_msg[512] = {"rsc.xml is missing, please download an integrated load or contact with BM."};
                flashtool_message_box(main_window_,
                                      main_window_->getConnectButton(),
                                      CRITICAL_MSGBOX,
                                      "Smart Phone Flash Tool",
                                      err_msg,
                                      "OK");
                Clear_RSC_Info();
                return;
            }
            else
            {
                LOG("installed-files-vendor says NO need rsx.xml; or installed-files-vendor NOT exist");
                ShowRSCItem(false);
            }

        }
    }
}

void DownloadWidget::slot_OnLoadByScatterEnd(bool bScatterVer2LoadRom)
{
    main_window_->processing_dialog()->hide();

    if(InitialPlatform())
    {
        //show rom image infos...
        std::list<ImageInfo> image_list;
        main_window_->main_controller()->GetImageInfoList(image_list, scene_);
        main_window_->ResetStatus();
        Platform platform = main_window_->main_controller()->GetPlatformSetting()
                ->getPlatformConfig();

        main_window_->setAutoPollingUpperLimit(platform.autoPollingUpperLimit());
        main_window_->setIsAutoPollingEnable(platform.isAutoPollingEnable());

        if (bScatterVer2LoadRom)
        {
            restoreImageRegionInfo(image_list);
        }
        UpdateImageList(image_list);

        SetDACheckSum();

        //start set baudrate=921600 for MTxxxx, fix me in future
        if(platform.getSimpleName().compare("MTxxxx")==0)
            main_window_->SetUARTBaudrateIndex(0);
        else
            main_window_->SetUARTBaudrateIndex(3);
    }
    else
    {
        UpdateUIStatus();
    }
}

void DownloadWidget::slot_OnLoadByScatterCanceled()
{
    main_window_->processing_dialog()->hide();
    ui_->comboBox_scatterFilePath->setEditText("");
    ui_->comboBox_scatterFilePath->setCurrentIndex(-1);
    main_window_->ResetStatus();
}

void DownloadWidget::slot_OnLoadByScatterFailed()
{
    main_window_->processing_dialog()->hide();

    UpdateUIStatus();
}

void DownloadWidget::slot_OnLoadRomDone()
{
    if (main_window_->IsScatterVer2())
    {
        bool isBeginAddrHidden = ui_->tableWidget->isColumnHidden(ColumnBeginAddr);
        bool isEndAddrHidden = ui_->tableWidget->isColumnHidden(ColumnEndAddr);
        bool isRegionHidden = ui_->tableWidget->isColumnHidden(columnRegion);
        QString sStorageText = main_window_->storageLabelText();
        slot_OnLoadByScatterEnd(true);
        ui_->tableWidget->setColumnHidden(ColumnBeginAddr, isBeginAddrHidden);
        ui_->tableWidget->setColumnHidden(ColumnEndAddr, isEndAddrHidden);
        ui_->tableWidget->setColumnHidden(columnRegion, isRegionHidden);
        main_window_->setStorageLabel(sStorageText);
    }
    else
    {
        slot_OnLoadByScatterEnd(false);
    }
}

void DownloadWidget::slot_OnLoadRomFailed()
{
    main_window_->processing_dialog()->hide();

    Platform platform = main_window_->main_controller()->GetPlatformSetting()->getPlatformConfig();
    std::string sOwnerName = platform.getOwnerName();

    flashtool_message_box(main_window_,
                          main_window_->getConnectButton(),
                          CRITICAL_MSGBOX,
                          LoadQString(LANGUAGE_TAG, IDS_STRING_TOOL_NAME),
                          LoadQString(LANGUAGE_TAG, IDS_STRING_ROMFILE_WARNING),
                          LoadQString(LANGUAGE_TAG, IDS_STRING_OK),
                          LoadQString(LANGUAGE_TAG, IDS_STRING_SEND_REPORT),
                          "",
                          0,
                          "",
                          sOwnerName,
                          true);

    int row = ui_->tableWidget->currentRow();

    QTableWidgetItem *tableItem = ui_->tableWidget->item(row, ColumnLocation);
    if (tableItem != NULL) {
        tableItem->setText(tr(""));
        QTableWidgetItem *p_enable_item = ui_->tableWidget->item(row, ColumnEnable);
        Q_ASSERT(NULL != p_enable_item);
        p_enable_item->setFlags(tableItem->flags()&~Qt::ItemIsUserCheckable);
        p_enable_item->setToolTip("Please select valid rom file first");
        if (p_enable_item->checkState() != Qt::Unchecked)
        {
            p_enable_item->setCheckState(Qt::Unchecked);
            RomEnabledChanged(row);
        }
    }
}

void DownloadWidget::LoadLastScatterFile()
{
    IniItem item("history.ini", "RecentOpenFile", "scatterHistory");

    scatterFile_historyList_ = item.GetStringListValue();

    item.SetItemName("lastDir");

    QString last_dir = item.GetStringValue();

    LOG("Init read the history list size is %d.\n", scatterFile_historyList_.size());

    if(!scatterFile_historyList_.empty())
    {
        int index = 0;

        if(last_dir.length() > 0)
        {
            index = scatterFile_historyList_.indexOf(last_dir);

            if(index < 0)
            {
                scatterFile_historyList_.insert(0, QDir::toNativeSeparators(last_dir));
                index = 0;
            }
        }
        QString path = QDir::toNativeSeparators((scatterFile_historyList_.at(index)));

        QFileInfo info(path);

        if(info.exists())
            LoadScatterFile(path);
        else
        {
            main_window_->slot_show_err(STATUS_SCATTER_FILE_NOT_FOUND, "The scatter file cannot find, please make sure the file is exist before download.");

            scatterFile_historyList_.removeAt(index);
            ui_->comboBox_scatterFilePath->clear();
            ui_->comboBox_scatterFilePath->insertItems(0, scatterFile_historyList_);
            ui_->comboBox_scatterFilePath->setCurrentIndex(-1);
            ui_->comboBox_scatterFilePath->setEditText("");

            item.SetItemName("lastDir");
            item.SaveStringValue("");

            index = ui_->comboBox_scatterFilePath->findText(path);
            if(index >= 0)
                ui_->comboBox_scatterFilePath->removeItem(index);

            return;
        }

        if(main_window_->main_controller()->GetPlatformSetting()->is_scatter_file_valid())
        {
            ui_->comboBox_scatterFilePath->clear();

            ui_->comboBox_scatterFilePath->insertItems(0, scatterFile_historyList_);

            ui_->comboBox_scatterFilePath->setCurrentIndex(index);

            ui_->comboBox_scatterFilePath->setEditText(path);
        }
    }
}

void DownloadWidget::LoadScatterFile(const QString &file_name)
{
    QString name = QDir::toNativeSeparators(file_name);

    main_window_->processing_dialog()->showCancelButton(true);
    main_window_->processing_dialog()->show();

    mStopFlag = 0;
    main_window_->main_controller()->SetChksumSetting(
                main_window_->CreateChksumSetting());
    main_window_->main_controller()->SetStopLoadFlag(&mStopFlag);
    main_window_->main_controller()->LoadScatterAsync(
                name,
                new SimpleCallback<DownloadWidget>(this,&DownloadWidget::OnLoadByScatterEnd),
                new SimpleCallback<DownloadWidget>(this,&DownloadWidget::OnLoadByScatterFailed),
                new SimpleCallback<DownloadWidget>(this,&DownloadWidget::OnLoadByScatterCanceled));
}

void DownloadWidget::slot_OnUserCancelLoadScatter()
{
    UserCancelLoadScatter();
}

void DownloadWidget::UserCancelLoadScatter()
{
    mStopFlag = AP_STOP;
}

bool DownloadWidget::IsRSCEnabled() const
{
    return ui_->comboBox_rsc->isVisible() && (!ui_->comboBox_rsc->currentText().isEmpty());
}

bool DownloadWidget::InitialPlatform()
{
    DL_PlatformInfo info;
    std::string error_hint;
    bool is_validate(false);

    memset(&info, 0, sizeof(info));

    //initial platform setting
    main_window_->main_controller()->GetPlatformInfo(info);

    is_validate = main_window_->main_controller()->GetPlatformSetting()
             ->initByNewScatterFile(info, error_hint);

    if(!is_validate)
    {
        main_window_->ShowHelpContents(this, QString(QString::fromLocal8Bit(error_hint.c_str())),
                                       "E_APP_SCATTER_FAILED.htm", true);

         return false;
    }

    return true;
}

void DownloadWidget::ShowUnavailableItems(bool show)
{
    ui_->label_certFile->setVisible(show);
    ui_->comboBox_certFilePath->setVisible(show);
    ui_->pushButton_CertFile->setVisible(show);
    ui_->toolButton_Certification->setVisible(show);

    //chip after Talbot, support download cert to storage
    std::string platform = main_window_->main_controller()->GetPlatformSetting()->getLoadPlatformName();
    BBCHIP_TYPE bbchip = BBChipTypeFromBBChipName(platform.c_str());
    bool support_dl_cert_to_storage = (bbchip >= MT6768 && bbchip < BBCHIP_TYPE_END);
    LOGI("support_dl_cert_to_storage: %d", support_dl_cert_to_storage);
    ui_->groupBox_download_cert_to->setVisible(support_dl_cert_to_storage ? show : false);

    if(show)
        ui_->comboBox_Scene->addItem(LoadQString(LANGUAGE_TAG, IDS_STRING_SCENE_WIPE_DATA), WIPE_DATA);
    else
        ui_->comboBox_Scene->removeItem(ui_->comboBox_Scene->count() - 1);

    if (ui_->comboBox_Scene->count() > 0) { // unlock downloadwidget page
        ui_->comboBox_Scene->setVisible(true);
        this->DoFinished();
        if (ui_->comboBox_Scene->findData(scene_) == -1) {
            changeDownloadSceneToDefault();
        }
    }
    else { // lock downloadwidget page
        ui_->comboBox_Scene->setVisible(false);
        this->LockOnUI();
        ui_->pushButton_stop->setEnabled(false);
        scene_ = UNKOWN_DOWNLOAD_SCENE;
    }
}

void DownloadWidget::ShowRSCItem(bool show)
{
    ui_->groupBox_rsc->setVisible(show);
}

void DownloadWidget::LoadDefaultDA()
{
    IniItem item("history.ini", "LastDAFilePath", "lastDir");

    default_da = item.GetStringValue();

    if(default_da.isEmpty())
        default_da = Utils::GetTextCodec()->toUnicode(ABS_PATH_C("MTK_AllInOne_DA.bin"));

    if(FileUtils::IsFileExist(default_da.toLocal8Bit().data()))
    {
        //load DA
        if(main_window_->main_controller()->LoadDA(default_da))
        {
            default_da = QDir::toNativeSeparators(default_da);
            ui_->lineEdit_agentFilePath->setText(
                        default_da);
        }
        else
        {
            ui_->lineEdit_agentFilePath->setText("");
        }
    }
    else
    {
        LOGW("Load default DA(%s) failed.", default_da.toLocal8Bit().data());
    }

}

int DownloadWidget::FindIndex(const QString &filePath)
{
    for(int i = 0; i < ui_->comboBox_scatterFilePath->count(); i++)
    {
        QString text = ui_->comboBox_scatterFilePath->itemText(i);

        if(filePath.compare(text) == 0)
            return i;
    }

    return -1;
}

void DownloadWidget::UpdateImageList(std::list<ImageInfo> &image_list)
{
    SCATTER_Head_Info info;
    main_window_->main_controller()->GetScatterHeadInfo(&info);
    QString scatter_ver = QString(info.version);
    bool scatter_ver2 = scatter_ver.indexOf("V2", 0, Qt::CaseInsensitive) != -1;
    bool showRegion = false;
    if((scatter_ver2 || (scatter_ver.indexOf("V1", 0, Qt::CaseInsensitive) != -1 && stricmp(info.version, "v1.1.1") > 0))
            && (stricmp(info.storage, "EMMC") == 0 || stricmp(info.storage, "UFS") == 0))
        showRegion = true;

    string strOwnerName = "";
    if(!ToolInfo::IsCustomerVer()) //not show owner name in customer version tool
    {
        Platform platform = main_window_->main_controller()->GetPlatformSetting()->getPlatformConfig();
        string sOwnerName = platform.getOwnerName();
        strOwnerName = sOwnerName=="" ? "" : ("Owner: "+sOwnerName);
    }
    QString str = QString((char*)(strOwnerName.c_str()));
    main_window_->UpdatePlatformImageString(info.platform, str);
    main_window_->scatter_observer()->notifyObservers(showRegion, scatter_ver2);
    main_window_->UpdateReadbackList((strcmp(info.storage, "EMMC") == 0 || strcmp(info.storage, "UFS") == 0) ? false : true);
/** onPlatformChanged End**/

    if (scatter_ver2)
    {
        ui_->tableWidget->setColumnHidden(ColumnBeginAddr, true);
        ui_->tableWidget->setColumnHidden(ColumnEndAddr, true);
        ui_->tableWidget->setColumnHidden(columnRegion, true);
        main_window_->setStorageLabel("Combo");
    }
    else
    {
        ui_->tableWidget->setColumnHidden(ColumnBeginAddr, false);
        ui_->tableWidget->setColumnHidden(ColumnEndAddr, false);
        ui_->tableWidget->setColumnHidden(columnRegion, !showRegion);
    }

    ImageStatusMap oRegionStatusMap;
    QList<UIImageStruct> oUIRegionList = FilterVisibleImages(image_list, oRegionStatusMap);
    ui_->tableWidget->setRowCount(oUIRegionList.count());
    RefreshTableWidget(oUIRegionList);

    UpdateScene();

    if(scene_ == DOWNLOAD_ONLY)
        UpdateCustomSceneSelectItems();
}

void DownloadWidget::UpdateImageList()
{
    IniItem item("option.ini", "CustomScene", "CheckedItems");

    QStringList items = item.GetStringListValue();

     if(items.size() == 0)
     {
         UpdateImageList(true, DOWNLOAD_ONLY);
         header_->SetChecked(true);
     }
     else
     {
         bool ok;
         for(int i = 0; i < items.size(); i++)
         {
             int index = items.at(i).toInt(&ok);

             if(ok && index >= 0)
             {
                 QTableWidgetItem *item = ui_->tableWidget->item(index, ColumnEnable);
                 Q_ASSERT(NULL != item);
                 if(item->checkState() != Qt::Checked)
                 {
                     item->setCheckState(Qt::Checked);
                     int item_index = main_window_->main_controller()->QueryROMIndex(item->row());

                     main_window_->main_controller()->EnableROM(item_index, true);
                 }
             }
         }
     }
}

void DownloadWidget::UpdateImageList(bool checked, Download_Scene scene)
{
    bool CheckAll = checked;

    IniItem item("option.ini", "Download", "CheckLoadIntegrity");
    bool b_check_load_integrity = true;
    if (item.hasKey())
    {
        b_check_load_integrity = item.GetBooleanValue();
    }

    for(int row = 0; row < ui_->tableWidget->rowCount(); row++)
    {
        bool imageExist = !ui_->tableWidget->item(row, ColumnLocation)->text().isEmpty();
        if(b_check_load_integrity && (scene == FORMAT_ALL_DOWNLOAD || scene == FIRMWARE_UPGRADE) && !imageExist)
        {
            QString part_name = ui_->tableWidget->item(row, ColumnName)->text();
            QString err_msg = "Partition ["+part_name+"] No image file Exist!";
            flashtool_message_box(main_window_,
                                  main_window_->getConnectButton(),
                                  CRITICAL_MSGBOX,
                                  "Smart Phone Flash Tool",
                                  err_msg,
                                  "OK");
            UpdateScene();
            return;
        }

        QTableWidgetItem *item = ui_->tableWidget->item(row, ColumnEnable);

        Qt::CheckState new_state = (CheckAll && imageExist) ? Qt::Checked : Qt::Unchecked;
        if (new_state != item->checkState())
        {
            item->setCheckState(new_state);
            int item_index = main_window_->main_controller()->QueryROMIndex(item->row());
            main_window_->main_controller()->EnableROM(item_index, checked);
        }
    }
}

void DownloadWidget::UpdateScatterFile(const QString &file_name)
{
    LoadScatterFile(file_name);

    if(main_window_->main_controller()->GetPlatformSetting()->is_scatter_file_valid())
    {
        int  index = FindIndex(QDir::toNativeSeparators(file_name));

        if(index < 0)
        {
            scatterFile_historyList_.insert(0, QDir::toNativeSeparators(file_name));
            LOG("The history list size is %d.\n", scatterFile_historyList_.size());

            ui_->comboBox_scatterFilePath->insertItem(0, QDir::toNativeSeparators(file_name));

            ui_->comboBox_scatterFilePath->setCurrentIndex(0);
        }
        else
        {
            ui_->comboBox_scatterFilePath->setCurrentIndex(index);
        }

        ui_->comboBox_scatterFilePath->setEditText(QDir::toNativeSeparators(file_name));
    }
}

void DownloadWidget::SetRomAddress(int row, int column, U64 address)
{
    QTableWidgetItem *tableItem = ui_->tableWidget->item(row, column);
    if (tableItem == NULL) {
        tableItem = new QTableWidgetItem();
        ui_->tableWidget->setItem(row, column,tableItem);
    }
    tableItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    tableItem->setText(QString("0x%1").arg(address,16,16,QChar('0')));
}

void DownloadWidget::SetRomAddress(int row, int column, const QString &sAddress)
{
    QTableWidgetItem *tableItem = ui_->tableWidget->item(row, column);
    if (tableItem == NULL) {
        tableItem = new QTableWidgetItem();
        ui_->tableWidget->setItem(row, column, tableItem);
    }
    tableItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    tableItem->setText(sAddress);
}

bool DownloadWidget::hasUncheckedRomInfo()
{
    for(int row = 0; row < ui_->tableWidget->rowCount(); row++)
    {
        QTableWidgetItem *item = ui_->tableWidget->item(row, ColumnEnable);

        if(item == NULL)
            return false;

        if(item->checkState() == Qt::Unchecked)
            return true;
    }

    return false;
}

void DownloadWidget::UpdateScene()
{
    if (ui_->comboBox_Scene->count() == 0)
    {
        flashtool_message_box(main_window_,
                              0,
                              WARNING_MSGBOX,
                              LoadQString(LANGUAGE_TAG, IDS_STRING_TOOL_NAME),
                              LoadQString(LANGUAGE_TAG, IDS_STRING_DOWNLOAD_SCENE_SWITCH_INFO),
                              LoadQString(LANGUAGE_TAG, IDS_STRING_OK));
        return ;
    }
    changeDownloadSceneToDefault();
}

void DownloadWidget::UpdateCustomSceneSelectItems()
{
    QStringList items;

    for(int i = 0; i < ui_->tableWidget->rowCount(); i++)
    {
        QTableWidgetItem *item = ui_->tableWidget->item(i, ColumnEnable);

        if(item && item->checkState() == Qt::Checked)
        {
            items.append(QString::number(i));
        }
    }

    IniItem ini_item("option.ini", "CustomScene", "CheckedItems");

    ini_item.SaveStringListValue(items);
}

void DownloadWidget::UpdateHeadviewCheckState()
{
    if(scene_ == FORMAT_ALL_DOWNLOAD || scene_ == FIRMWARE_UPGRADE)
    {
        header_->SetChecked(true);
    }
    else if(scene_ == DOWNLOAD_ONLY)
    {
        if(hasUncheckedRomInfo())
            header_->SetChecked(false);
        else
            header_->SetChecked(true);
    }
    else
    {
        header_->SetChecked(false);
    }
}

void DownloadWidget::LockOnUI()
{
    //TODO
    ui_->toolButton_Certification->setEnabled(false);
    ui_->pushButton_download->setEnabled(false);
    ui_->pushButton_stop->setEnabled(true);
    ui_->FileLoadFrame->setEnabled(false);
    ui_->tableWidget->setEnabled(false);
}

void DownloadWidget::DoFinished()
{
    ui_->toolButton_Certification->setEnabled(true);
    ui_->pushButton_download->setEnabled(true);
    ui_->pushButton_stop->setEnabled(false);
    ui_->FileLoadFrame->setEnabled(true);
    ui_->tableWidget->setEnabled(true);
}

void DownloadWidget::UpdateUI()
{
    ui_->retranslateUi(this);

    if(FileUtils::IsFileExist(default_da.toLocal8Bit().data()))
            ui_->lineEdit_agentFilePath->setText(
                         QDir::toNativeSeparators(default_da));
}

void DownloadWidget::SetTabLabel(QTabWidget *tab_widget, int index)
{
    QString label = LoadQString(LANGUAGE_TAG, IDS_STRING_DOWNLOAD);

    tab_widget->setTabText(index, label);

    for(int i = 0; i < ui_->comboBox_Scene->count(); i++)
    {
        QVariant item_data = ui_->comboBox_Scene->itemData(i);
        std::map<int, int>::const_iterator iter = g_download_scene_string_tag_map.find(item_data.toInt());
        assert(iter != g_download_scene_string_tag_map.end());
        ui_->comboBox_Scene->setItemText(i, LoadQString(LANGUAGE_TAG, iter->second));
    }
}

void DownloadWidget::SetShortCut(int cmd, const QString &shortcut)
{
    switch(cmd)
    {
    case DL_BEGIN:
        ui_->pushButton_download->setShortcut(shortcut);
        ui_->pushButton_download->setToolTip(shortcut);

        break;

    case DL_STOP:
        ui_->pushButton_stop->setShortcut(shortcut);
        ui_->pushButton_stop->setToolTip(shortcut);
        break;
    case DL_CERT:
        ui_->toolButton_Certification->setShortcut(shortcut);
        ui_->toolButton_Certification->setToolTip(shortcut);
    }
}

void DownloadWidget::SetDACheckSum()
{
     IniItem item("option.ini", "Download", "DAChksum");

     int chksum = item.GetIntValue();
     if(chksum >= DHNDL_FLAG_CHKSUM_LEVEL_NONE && chksum <= DHNDL_FLAG_CHKSUM_LEVEL_ALL)
         main_window_->slot_enable_DAChksum(chksum);
     else
         main_window_->slot_enable_DAChksum(DHNDL_FLAG_CHKSUM_LEVEL_NONE);
}

void DownloadWidget::BuildGeneralSetting(
    QSharedPointer<ConsoleMode::GeneralSetting> &gen_setting)
{
    QSharedPointer<ConsoleMode::GeneralArg> arg = QSharedPointer<ConsoleMode::GeneralArg>( new ConsoleMode::GeneralArg());
    QString scatter_path = ui_->comboBox_scatterFilePath->currentText();

    std::string platform = main_window_->main_controller()->GetPlatformSetting()->getLoadPlatformName();
    arg->chip_name=platform;

    HW_StorageType_E storage_type = main_window_->main_controller()->GetPlatformSetting()->getFlashToolStorageConfig().GetStorageType();
    arg->storage_type=storage_type;

    arg->da_file=ui_->lineEdit_agentFilePath->text().toLocal8Bit().constData();
    arg->scatter_file=scatter_path.toLocal8Bit().constData();

    arg->auth_file=ui_->comboBox_authFilePath->currentText().toLocal8Bit().constData();
    arg->cert_file=ui_->comboBox_certFilePath->currentText().toLocal8Bit().constData();

    std::list<ConsoleMode::RomItem> items;

    std::list<ImageInfo> image_list;
    main_window_->main_controller()->GetImageInfoList(image_list);

    for(std::list<ImageInfo>::const_iterator it = image_list.begin();
        it != image_list.end(); ++it)
    {
        if(it->visible)
        {
            ConsoleMode::RomItem rom;
            rom.index  = it->index;
            rom.enable = it->enabled;
            rom.path   = it->location;
            items.push_back(rom);
        }
    }

    arg->rom_list = items;
    gen_setting->vSetGeneralArg(arg);
}

bool DownloadWidget::IsScatterLoad()
{
    if(ui_->comboBox_scatterFilePath->currentText().isEmpty())
        return false;

    return true;
}

bool DownloadWidget::ValidateBeforeFormat()
{
    if(ui_->lineEdit_agentFilePath->text().isEmpty())
    {
        main_window_->ShowHelpContents(this, IDS_STRING_WARNING_LOADDA,
                                       "E_APP_LOAD_DA.htm");

        return false;
    }

    return IsScatterFileLoad();
}

bool DownloadWidget::IsScatterFileLoad(bool showPrompt)
{
    if(ui_->comboBox_scatterFilePath->currentText().isEmpty())
    {
        if (showPrompt)
        {
            main_window_->ShowHelpContents(this, IDS_STRING_WARNING_LOADSCATTER,
                                           "E_APP_LOAD_SCATTER.htm");
        }

        return false;
    }
    return true;
}

bool DownloadWidget::ValidateBeforeCertDL()
{
    if(ui_->comboBox_certFilePath->currentText().isEmpty())
    {
        main_window_->ShowHelpContents(this, IDS_STRING_WARNING_LOADCERT,
                                       "E_APP_LOAD_AUTH.htm");

        return false;
    }

    return true;
}

bool has_pgpt_image(const ImageInfo &image)
{
    std::string image_name = image.name;
    std::transform(image_name.begin(), image_name.end(), image_name.begin(), tolower);
    return image_name == "pgpt";
}

bool is_pgpt_image_selected(const ImageInfo &image)
{
    std::string image_name = image.name;
    std::transform(image_name.begin(), image_name.end(), image_name.begin(), tolower);
    return image_name == "pgpt" && image.enabled;
}

bool DownloadWidget::ValidateBeforeDownload()
{
   if(ui_->lineEdit_agentFilePath->text().isEmpty())
   {
        main_window_->ShowHelpContents(this, IDS_STRING_WARNING_LOADDA, "E_APP_LOAD_DA.htm");

        return false;
    }

    if(ui_->comboBox_scatterFilePath->currentText().isEmpty())
    {
        main_window_->ShowHelpContents(this, IDS_STRING_WARNING_LOADSCATTER, "E_APP_LOAD_SCATTER.htm");

        return false;
    }

    bool has_rom = false;
    std::list<ImageInfo> images;
    main_window_->main_controller()->GetImageInfoList(images);
    for(std::list<ImageInfo>::const_iterator it = images.begin();
        it != images.end(); ++it)
    {
        if(it->enabled)
        {
            has_rom = true;
            break;
        }
    }

    if(!has_rom)
    {
        main_window_->ShowHelpContents(this, IDS_STRING_WARNING_ROMNOTSELECT,
                                       "E_APP_NONROM_SELECT.htm");

        return false;
    }

    if(scene_ == WIPE_DATA || scene_ == FORMAT_ALL_DOWNLOAD || scene_ == DOWNLOAD_ONLY)
    {
        int index = GetSecRoIndex();

        if(index < 0 && scene_ == WIPE_DATA)
            return false;

        index = main_window_->main_controller()->QueryROMIndex(index);

        int result = main_window_->main_controller()->CheckSecUnlockSecro(index);

        if(result == 0)   // unlock sec-ro image
        {
            if(scene_ == FORMAT_ALL_DOWNLOAD || scene_ == DOWNLOAD_ONLY)
        {
            main_window_->ShowHelpContents(this, IDS_STRING_NOTALLOWED_UNLOCK,
                                           "E_APP_SEC_UNLOCK.htm", true);

            return false;
        }
        else
        {
            int prompt = flashtool_message_box(this,
                                               0,
                                               QUESTION_MSGBOX,
                                               LoadQString(LANGUAGE_TAG, IDS_STRING_TOOL_NAME),
                                               LoadQString(LANGUAGE_TAG, IDS_STRING_SECURITY_WARNING),
                                               LoadQString(LANGUAGE_TAG, IDS_STRING_YES),
                                               LoadQString(LANGUAGE_TAG, IDS_STRING_NO));

            if(prompt == 1)
                return false;
        }
    }
        else if(result != 0 && scene_ == WIPE_DATA)
        {
            main_window_->ShowHelpContents(this, IDS_STRING_SECURITY_ERROR,
                                           "E_APP_SEC_IMAGE.htm", true);

            return false;
        }       
    }

    bool rsc_invalid = ui_->comboBox_rsc->isVisible()
            && (ui_->comboBox_rsc->currentText().isEmpty());
    if(rsc_invalid)
    {
        flashtool_message_box(main_window_,
                              main_window_->getConnectButton(),
                              CRITICAL_MSGBOX,
                              "Smart Phone Flash Tool",
                              "rsc xml file invalid, please check",
                              "OK");
        return false;
    }

    SCATTER_Head_Info scatter_head_info;
    main_window_->main_controller()->GetScatterHeadInfo(&scatter_head_info);
    if (scatter_head_info.skip_pmt_operate)
    {
        std::list<ImageInfo>::iterator iter = std::find_if(images.begin(), images.end(), has_pgpt_image);
        if (iter == images.end())
        {
            flashtool_message_box(main_window_,
                                  0,
                                  WARNING_MSGBOX,
                                  LoadQString(LANGUAGE_TAG, IDS_STRING_TOOL_NAME),
                                  LoadQString(LANGUAGE_TAG, IDS_STRING_DOWNLOAD_PGPT_NOT_EXIST),
                                  LoadQString(LANGUAGE_TAG, IDS_STRING_OK));
            return false;
        }
        iter = std::find_if(images.begin(), images.end(), is_pgpt_image_selected);
        if (iter == images.end())
        {
            flashtool_message_box(main_window_,
                                  0,
                                  WARNING_MSGBOX,
                                  LoadQString(LANGUAGE_TAG, IDS_STRING_TOOL_NAME),
                                  LoadQString(LANGUAGE_TAG, IDS_STRING_DOWNLOAD_PGPT_NOT_SELECTED),
                                  LoadQString(LANGUAGE_TAG, IDS_STRING_OK));
            return false;
        }
    }

    return true;
}

int DownloadWidget::GetSecRoIndex()
{
    QSettings settings(ABS_PATH_C("download_scene.ini"), QSettings::IniFormat);
    settings.beginGroup("Download");
    QStringList wipeList = settings.value("WIPE_DATA").toStringList();
    settings.endGroup();

    for(int i = 0; i < ui_->tableWidget->rowCount(); i++)
    {
        if(ui_->tableWidget->item(i, ColumnEnable)->checkState() == Qt::Checked &&
               wipeList.contains(ui_->tableWidget->item(i, ColumnName)->text()))
            return i;
    }

    return -1;
}

void DownloadWidget::choose_rom_file(int row)
{
    QString file = ui_->tableWidget->item(row,ColumnLocation)->text();
    QString new_file = QFileDialog::getOpenFileName(this, "Open File",
                                        file, "All File (*.*)");
    if(!new_file.isEmpty())
    {
        mCurLocationRow = row;
        new_file = QDir::toNativeSeparators(new_file);
        //loading this single rom
        main_window_->processing_dialog()->showCancelButton(false);
        main_window_->processing_dialog()->show();

        int item_index = main_window_->main_controller()->QueryROMIndex(row);
        main_window_->main_controller()->LoadROMAsync(new_file,item_index,
            new SimpleCallback<DownloadWidget>(this,&DownloadWidget::OnLoadRomDone),
            new SimpleCallback<DownloadWidget>(this,&DownloadWidget::OnLoadRomFailed));

        //process preloader/loader_ext1/loader_ext2
        char scatter_version[10] = {0};
        if(main_window_->main_controller()->GetScatterVersion(scatter_version))
        {
            LOGD("scatter version: %s", scatter_version);
        }
        else
        {
            emit signal_load_rom_failed();
            LOGE("get scatter version fail.");
            return;
        }

        char project_name[128] = {0};
        SCATTER_Head_Info scatter_head;
        if(main_window_->main_controller()->GetScatterHeadInfo(&scatter_head))
        {
            memcpy(project_name, scatter_head.project, sizeof(project_name));
            LOGD("project_name: %s", project_name);
        }
        else
        {
            emit signal_load_rom_failed();
            LOGE("get project_name fail. ");
            return;
        }

        if(std::string(scatter_version) == "V1.1.6")
        {
            std::string partition_name = ui_->tableWidget->item(row, ColumnName)->text().toLocal8Bit().constData();
            if(IsPreloaderPartition(partition_name))
            {
                std::string new_file_path = new_file.toLocal8Bit().constData();
                std::size_t position = new_file_path.find("SBOOT_DIS");
                bool is_sboot_dis = (std::string::npos != position);

                std::string new_path = new_file_path.substr(0, new_file_path.find_last_of("/\\"));
                std::string new_loader_ext_filename = new_path + "\\loader_ext-verified.img";
                if(is_sboot_dis)
                {
                    new_loader_ext_filename = new_path+"\\loader_ext_SBOOT_DIS.img";
                    if(!QFile(QString::fromStdString(new_loader_ext_filename)).exists())
                        new_loader_ext_filename = new_path + "\\loader_ext-verified.img";
                }
                if(!QFile(QString::fromStdString(new_loader_ext_filename)).exists())
                    new_loader_ext_filename = new_path + "\\loader_ext.img";

                std::string new_preloader_short_filename = "preloader_"+std::string(project_name)+".bin";
                if(is_sboot_dis)
                    new_preloader_short_filename = "preloader_"+std::string(project_name)+"_SBOOT_DIS.bin";
                std::string new_preloader_filename = new_path + "\\" + new_preloader_short_filename;
                LOGD("new_preloader_filename: %s", new_preloader_filename.c_str());
                LOGD("new_loader_ext_filename: %s", new_loader_ext_filename.c_str());

                int pl_index, loader_ext1_index, loader_ext2_index;
                main_window_->main_controller()->QueryPreloadersIndex(&pl_index, &loader_ext1_index, &loader_ext2_index);

                int pl_visible_index, loader_ext1_visible_index, loader_ext2_visible_index;
                QueryPreloadersVisbleIndex(&pl_visible_index, &loader_ext1_visible_index, &loader_ext2_visible_index);

                if(item_index != pl_index && pl_visible_index != -1)
                {
                    //load pl
                    main_window_->main_controller()->LoadROMAsync(QString::fromStdString(new_preloader_filename),
                                                              pl_index,
                        new SimpleCallback<DownloadWidget>(this,&DownloadWidget::OnLoadRomDone),
                        new SimpleCallback<DownloadWidget>(this,&DownloadWidget::OnLoadRomFailed));

                    ui_->tableWidget->item(pl_visible_index, ColumnLocation)->setText(QString::fromStdString(new_preloader_filename));
                }

                if(item_index != loader_ext1_index && loader_ext1_visible_index != -1)
                {
                    //load ext1
                    main_window_->main_controller()->LoadROMAsync(QString::fromStdString(new_loader_ext_filename),
                                                              loader_ext1_index,
                        new SimpleCallback<DownloadWidget>(this,&DownloadWidget::OnLoadRomDone),
                        new SimpleCallback<DownloadWidget>(this,&DownloadWidget::OnLoadRomFailed));

                    ui_->tableWidget->item(loader_ext1_visible_index, ColumnLocation)->setText(QString::fromStdString(new_loader_ext_filename));
                }

                if(item_index != loader_ext2_index && loader_ext2_visible_index != -1)
                {
                    //load ext2
                    main_window_->main_controller()->LoadROMAsync(QString::fromStdString(new_loader_ext_filename),
                                                              loader_ext2_index,
                        new SimpleCallback<DownloadWidget>(this,&DownloadWidget::OnLoadRomDone),
                        new SimpleCallback<DownloadWidget>(this,&DownloadWidget::OnLoadRomFailed));

                    ui_->tableWidget->item(loader_ext2_visible_index, ColumnLocation)->setText(QString::fromStdString(new_loader_ext_filename));
                }

                if(pl_visible_index != -1)
                    ui_->tableWidget->item(pl_visible_index, ColumnEnable)->setCheckState(Qt::Checked);

                if(loader_ext1_visible_index != -1)
                    ui_->tableWidget->item(loader_ext1_visible_index, ColumnEnable)->setCheckState(Qt::Checked);

                if(loader_ext2_visible_index != -1)
                    ui_->tableWidget->item(loader_ext2_visible_index, ColumnEnable)->setCheckState(Qt::Checked);
            }
        }
        //endo of process pl/ext1/ext2
    }
}

void DownloadWidget::on_comboBox_Scene_activated(int index)
{
    scene_ = static_cast<Download_Scene>(ui_->comboBox_Scene->itemData(index).toInt());

    UpdateRomInfoList(scene_);
}

void DownloadWidget::UpdateRomInfoList(Download_Scene scene)
{
    //UpdateImageList(false, scene);

    if(scene != DOWNLOAD_ONLY)
        UpdateImageList(true, scene);
    else
        UpdateImageList();

    if(scene == FORMAT_ALL_DOWNLOAD || scene == FIRMWARE_UPGRADE)
    {
        header_->SetChecked(true);
    }
    else if(scene == WIPE_DATA)
    {
        header_->SetChecked(false);
    }
}

void DownloadWidget::on_comboBox_scatterFilePath_activated(const QString &arg1)
{
    QString file_name = arg1;

    QFile file(file_name);

    if(file_name.isEmpty() || file.exists() == false)
        return;

    LoadScatterFile(file_name);

    IniItem item("history.ini", "RecentOpenFile", "lastDir");

    item.SaveStringValue(QDir::toNativeSeparators(file_name));
}

void DownloadWidget::onPlatformChanged()
{
    storage_ = main_window_->main_controller()->GetPlatformSetting()->getFlashToolStorageConfig().GetStorageType();
}

void DownloadWidget::OnScatterChanged(bool showRegion, bool scatter_ver2)
{
    Q_UNUSED(scatter_ver2)
    ui_->tableWidget->setColumnHidden(columnRegion, !showRegion);

    QString rsc_dir = (QFileInfo(ui_->comboBox_scatterFilePath->currentText())).path();
    QString rsc_filepath = rsc_dir + QDir::separator().toLatin1() + "rsc.xml";
    Process_rsc(rsc_filepath);
}

bool DownloadWidget::IsPreloaderPartition(std::string partition_name)
{
    if(partition_name == "preloader" || partition_name == "loader_ext1" || partition_name == "loader_ext2")
        return true;
    else
        return false;
}

int DownloadWidget::QueryROMVisbleIndex(std::string part_name)
{
    int row_cnt = ui_->tableWidget->rowCount();

    for(int i = 0; i<row_cnt; i++)
    {
        QTableWidgetItem *tableItem = ui_->tableWidget->item(i, ColumnName);
        if(tableItem)
        {
            std::string item_part_name = tableItem->text().toLocal8Bit().constData();
            if(item_part_name == part_name)
            {
                return i;
            }
        }
    }

    return -1;
}

void DownloadWidget::QueryPreloadersVisbleIndex(int* pl_index, int* ext1_index, int* ext2_index)
{
    int row_cnt = ui_->tableWidget->rowCount();
    int found = 0;
    *pl_index = -1;
    *ext1_index = -1;
    *ext2_index = -1;

    for(int i = 0; i<row_cnt; i++)
    {
        QTableWidgetItem *tableItem = ui_->tableWidget->item(i, ColumnName);
        if(tableItem)
        {
            std::string item_part_name = tableItem->text().toLocal8Bit().constData();
            if(item_part_name == "preloader")
            {
                *pl_index = i;

                found++;
                if(found >= 3)
                    break;
            }

            if(item_part_name == "loader_ext1")
            {
                *ext1_index = i;

                found++;
                if(found >= 3)
                    break;
            }

            if(item_part_name == "loader_ext2")
            {
                *ext2_index = i;

                found++;
                if(found >= 3)
                    break;
            }
        }
    }
}

void DownloadWidget::Enable_preloaders_partition(bool enable)
{
    int pl_index, loader_ext1_index, loader_ext2_index;
    main_window_->main_controller()->QueryPreloadersIndex(&pl_index, &loader_ext1_index, &loader_ext2_index);

    int pl_visible_index, loader_ext1_visible_index, loader_ext2_visible_index;
    QueryPreloadersVisbleIndex(&pl_visible_index, &loader_ext1_visible_index, &loader_ext2_visible_index);

    //LOGD("pl_index[%d], loader_ext1_index[%d], loader_ext2_index[%d]\n", pl_index, loader_ext1_index, loader_ext2_index);
    //LOGD("pl_visible_index[%d], loader_ext1_visible_index[%d], loader_ext2_visible_index[%d]\n", pl_visible_index, loader_ext1_visible_index, loader_ext2_visible_index);

    if(pl_index != -1)
        main_window_->main_controller()->EnableROM(pl_index, enable);
    if(pl_visible_index != -1)
        ui_->tableWidget->item(pl_visible_index, ColumnEnable)->setCheckState(enable ? Qt::Checked : Qt::Unchecked);

    if(loader_ext1_index != -1)
        main_window_->main_controller()->EnableROM(loader_ext1_index, enable);
    if(loader_ext1_visible_index != -1)
        ui_->tableWidget->item(loader_ext1_visible_index, ColumnEnable)->setCheckState(enable ? Qt::Checked : Qt::Unchecked);

    if(loader_ext2_index != -1)
        main_window_->main_controller()->EnableROM(loader_ext2_index, enable);
    if(loader_ext2_visible_index != -1)
        ui_->tableWidget->item(loader_ext2_visible_index, ColumnEnable)->setCheckState(enable ? Qt::Checked : Qt::Unchecked);
}

void DownloadWidget::RomEnabledChanged(int row)
{
    int item_index = main_window_->main_controller()->QueryROMIndex(row);

    QTableWidgetItem *p_enable_item = ui_->tableWidget->item(row, ColumnEnable);
    Q_ASSERT(NULL != p_enable_item);
    if(p_enable_item->checkState() == Qt::Checked && !ui_->tableWidget->item(row, ColumnLocation)->text().isEmpty())
    {
        main_window_->main_controller()->EnableROM(item_index, true);

        if(row == QueryROMVisbleIndex("super"))
        {
            int vbmeta_system_idx = main_window_->main_controller()->QueryROMIndex("vbmeta_system");
            int vbmeta_vendor_idx = main_window_->main_controller()->QueryROMIndex("vbmeta_vendor");
            int vbmeta_system_visble_idx = QueryROMVisbleIndex("vbmeta_system");
            int vbmeta_vendor_visble_idx = QueryROMVisbleIndex("vbmeta_vendor");
            main_window_->main_controller()->EnableROM(vbmeta_system_idx, true);
            main_window_->main_controller()->EnableROM(vbmeta_vendor_idx, true);
            ui_->tableWidget->item(vbmeta_system_visble_idx, ColumnEnable)->setCheckState(Qt::Checked);
            ui_->tableWidget->item(vbmeta_vendor_visble_idx, ColumnEnable)->setCheckState(Qt::Checked);
        }
    }
    else
    {
        if(row == QueryROMVisbleIndex("vbmeta_system") || row == QueryROMVisbleIndex("vbmeta_vendor"))
        {
            int super_idx = QueryROMVisbleIndex("super");
            bool super_enable = ui_->tableWidget->item(super_idx, ColumnEnable)->checkState() == Qt::Checked;
            if(super_enable)
            {
                ui_->tableWidget->item(row, ColumnEnable)->setCheckState(Qt::Checked);
                flashtool_message_box(main_window_,
                                      main_window_->getConnectButton(),
                                      INFORMATION_MSGBOX,
                                      "Smart Phone Flash Tool",
                                      "Must disable 'super' first!",
                                      "OK");
                return;
            }
        }
        main_window_->main_controller()->EnableROM(item_index, false);
    }

    PreloaderChanged(p_enable_item);

    UpdateScene();

    if(scene_ == DOWNLOAD_ONLY)
        UpdateCustomSceneSelectItems();

    UpdateHeadviewCheckState();
}

void DownloadWidget::PreloaderChanged(QTableWidgetItem *item)
{
    //process preloader, loader_ext1, loader_ext2, need enable/disable at same time
    //otherwise will boot fail
    std::string part_name = ui_->tableWidget->item(item->row(), ColumnName)->text().toLocal8Bit().constData();
    if (IsPreloaderPartition(part_name))
    {
        char scatter_version[10];
        main_window_->main_controller()->GetScatterVersion(scatter_version);
        LOGD("scatter version: %s", scatter_version);
        if (std::string(scatter_version) == "V1.1.6")
        {
            Enable_preloaders_partition(item->checkState() == Qt::Checked);
        }
    }
    //end of process preloader/ext1/ext2
}

void DownloadWidget::RefreshTableWidget(const QList<UIImageStruct> &oRegionList)
{
    IniItem item("option.ini", "Download", "CheckLoadIntegrity");
    bool b_check_load_integrity = true;
    if (item.hasKey())
    {
        b_check_load_integrity = item.GetBooleanValue();
    }

    QTableWidgetItem * tableItem;
    int row = 0;
    bool has_uncheck_item = false;
    for(QList<UIImageStruct>::const_iterator it = oRegionList.begin();
        it != oRegionList.end(); ++it)
    {
        SetRomAddress(row, ColumnBeginAddr, it->sBeginAddr);
        SetRomAddress(row, ColumnEndAddr, it->sEndAddr);

        tableItem = ui_->tableWidget->item(row, columnRegion);
        if(tableItem == NULL){
            tableItem = new QTableWidgetItem();
            ui_->tableWidget->setItem(row, columnRegion, tableItem);
        }

        tableItem->setText(it->sRegion);

        tableItem = ui_->tableWidget->item(row, ColumnLocation);
        if (tableItem == NULL) {
            tableItem = new QTableWidgetItem();
            ui_->tableWidget->setItem(row, ColumnLocation,tableItem);
        }
        tableItem->setText(it->sLocation);

        tableItem = ui_->tableWidget->item(row, ColumnName);
        if (tableItem == NULL) {
            tableItem = new QTableWidgetItem();
            ui_->tableWidget->setItem(row, ColumnName, tableItem);
        }
        tableItem->setText(it->sName);

        tableItem = ui_->tableWidget->item(row, ColumnEnable);
        if (tableItem == NULL) {
            tableItem = new QTableWidgetItem();
            ui_->tableWidget->setItem(row, ColumnEnable, tableItem);
        }

        bool imageExist = !ui_->tableWidget->item(row, ColumnLocation)->text().isEmpty();
        if(b_check_load_integrity && (scene_ == FORMAT_ALL_DOWNLOAD || scene_ == FIRMWARE_UPGRADE) && !imageExist)
        {
            QString part_name = ui_->tableWidget->item(row, ColumnName)->text();
            QString err_msg = "Partition ["+part_name+"] No image file Exist!";
            flashtool_message_box(main_window_,
                                  main_window_->getConnectButton(),
                                  CRITICAL_MSGBOX,
                                  "Smart Phone Flash Tool",
                                  err_msg,
                                  "OK");
            UpdateScene();
            return;
        }

        int item_index = main_window_->main_controller()->QueryROMIndex(tableItem->row());
        if (it->checkState == Qt::Checked && imageExist)
        {
            tableItem->setCheckState(Qt::Checked);
            tableItem->setFlags(tableItem->flags()| Qt::ItemIsUserCheckable);
            tableItem->setToolTip("");
            main_window_->main_controller()->EnableROM(item_index, true);
        }
        else
        {
            tableItem->setCheckState(Qt::Unchecked);
            has_uncheck_item = true;
            if(ui_->tableWidget->item(row, ColumnLocation)->text().isEmpty())
            {
                tableItem->setFlags(tableItem->flags()&~Qt::ItemIsUserCheckable);
                tableItem->setToolTip("Please select valid rom file first");
            }
            main_window_->main_controller()->EnableROM(item_index, false);
        }
        PreloaderChanged(tableItem);

        row++;
    }
    if(!has_uncheck_item)
        header_->SetChecked(true);
    else
        header_->SetChecked(false);

    updateTableWidgetFlagsByConfig();
}

QList<UIImageStruct> DownloadWidget::FilterVisibleImages(const std::list<ImageInfo> &image_list, const ImageStatusMap &image_status_map) const
{
    QList<UIImageStruct> oUIRegionList;
    for (std::list<ImageInfo>::const_iterator it = image_list.begin(); it != image_list.end(); ++it)
    {
        if(!it->visible)
        {
            //skip invisible items
            continue;
        }
        UIImageStruct oUiRegion;
        oUiRegion.sBeginAddr = QString("0x%1").arg(it->begin_addr, 16, 16, QChar('0'));
        oUiRegion.sEndAddr = QString("0x%1").arg(it->end_addr, 16, 16, QChar('0'));
        oUiRegion.sRegion = EnumToRomString(storage_, it->region);
        oUiRegion.sName = it->name.c_str();
        ImageStatusMap::const_iterator cit = image_status_map.find(oUiRegion.sName);
        if (cit != image_status_map.end())
        {
            oUiRegion.sLocation = cit.value().second;
            oUiRegion.checkState = cit.value().first;
        }
        else
        {
            oUiRegion.sLocation = QDir::toNativeSeparators(QString::fromLocal8Bit(it->location.c_str()));
            oUiRegion.checkState = it->enabled ? Qt::Checked : Qt::Unchecked;
        }

        oUIRegionList.append(oUiRegion);
    }
    return oUIRegionList;
}

void DownloadWidget::restoreImageRegionInfo(std::list<ImageInfo> &image_list) const
{
    typedef std::list<ImageInfo>::iterator ImageIter;
    int nIndex = 0;
    for (ImageIter iter = image_list.begin(); iter != image_list.end(); ++iter)
    {
        if (!iter->visible)
        {
            continue;
        }
        std::string sCurUIName = ui_->tableWidget->item(nIndex, ColumnName)->text().toStdString();
        Q_ASSERT(sCurUIName == iter->name);
        QString sRegion = ui_->tableWidget->item(nIndex, columnRegion)->text();
        iter->region = Utils::getRegionPartId(sRegion);
        Q_ASSERT(mCurLocationRow != -1);
        if (mCurLocationRow != nIndex)
        {
            iter->begin_addr = ui_->tableWidget->item(nIndex, ColumnBeginAddr)->text().toULongLong(NULL, 16);
            iter->end_addr = ui_->tableWidget->item(nIndex, ColumnEndAddr)->text().toULongLong(NULL, 16);
        }
        ++nIndex;
    }
}

void DownloadWidget::updateDownloadSceneByConfig()
{
    QString sFormat_all_download = LoadQString(LANGUAGE_TAG, IDS_STRING_SCENE_FMTALLDL);
    QString sFireware_upgrade = LoadQString(LANGUAGE_TAG, IDS_STRING_SCENE_FIRMWAREUPGRADE);
    QString sDownload_only = LoadQString(LANGUAGE_TAG, IDS_STRING_SCENE_DOWNLOADONLY);
    QString sWipe_data = LoadQString(LANGUAGE_TAG, IDS_STRING_SCENE_WIPE_DATA);

    IniItem item("option.ini", "DownloadScene", "");
    if (item.hasSection()) {
        item.SetItemName("Format_All_Download");
        if (item.hasKey() && item.GetBooleanValue()) {
            ui_->comboBox_Scene->addItem(sFormat_all_download, FORMAT_ALL_DOWNLOAD);
        }
        item.SetItemName("Firmware_Upgrade");
        if (item.hasKey() && item.GetBooleanValue()) {
            ui_->comboBox_Scene->addItem(sFireware_upgrade, FIRMWARE_UPGRADE);
        }
        item.SetItemName("Download_Only");
        if (item.hasKey() && item.GetBooleanValue()) {
            ui_->comboBox_Scene->addItem(sDownload_only, DOWNLOAD_ONLY);
        }
        ui_->comboBox_Scene->addItem(sWipe_data, WIPE_DATA);

        changeDownloadSceneToDefault();
    }
    else {
        ui_->comboBox_Scene->addItem(sFormat_all_download, FORMAT_ALL_DOWNLOAD);
        ui_->comboBox_Scene->addItem(sFireware_upgrade, FIRMWARE_UPGRADE);
        ui_->comboBox_Scene->addItem(sDownload_only, DOWNLOAD_ONLY);
        ui_->comboBox_Scene->addItem(sWipe_data, WIPE_DATA);
        ui_->comboBox_Scene->setCurrentIndex(DOWNLOAD_ONLY);
        scene_ = DOWNLOAD_ONLY;
    }
}

void DownloadWidget::changeDownloadSceneToDefault()
{
    // linear switch to DOWNLOAD_ONLY, FIRMWARE_UPGEADE, FOEMAT_ALL_DOWNLOAD and WIPE_DATA if they exists
    // when do scatter change, item checked change or location change.
    if(ui_->comboBox_Scene->findData(DOWNLOAD_ONLY) != -1)
    {
        if (scene_ != DOWNLOAD_ONLY) {
            ui_->comboBox_Scene->setCurrentIndex(ui_->comboBox_Scene->findData(DOWNLOAD_ONLY));
            scene_ = DOWNLOAD_ONLY;
        }
    }
    else if(ui_->comboBox_Scene->findData(FIRMWARE_UPGRADE) != -1)
    {
        if (scene_ != FIRMWARE_UPGRADE) {
            ui_->comboBox_Scene->setCurrentIndex(ui_->comboBox_Scene->findData(FIRMWARE_UPGRADE));
            scene_ = FIRMWARE_UPGRADE;
        }
    }
    else if(ui_->comboBox_Scene->findData(FORMAT_ALL_DOWNLOAD) != -1)
    {
        if (scene_ != FORMAT_ALL_DOWNLOAD) {
            ui_->comboBox_Scene->setCurrentIndex(ui_->comboBox_Scene->findData(FORMAT_ALL_DOWNLOAD));
            scene_ = FORMAT_ALL_DOWNLOAD;
        }
    }
    else if(ui_->comboBox_Scene->findData(WIPE_DATA) != -1)
    {
        if (scene_ != WIPE_DATA) {
            ui_->comboBox_Scene->setCurrentIndex(ui_->comboBox_Scene->findData(WIPE_DATA));
            scene_ = WIPE_DATA;
        }
    }
    assert(scene_ != UNKOWN_DOWNLOAD_SCENE);
}

void DownloadWidget::updateTableWidgetFlagsByConfig()
{
    if (checkStateEnabledByConfig()) {
        for(int row = 0; row < ui_->tableWidget->rowCount(); row++)
        {
            QTableWidgetItem *item = ui_->tableWidget->item(row, ColumnEnable);
            assert(NULL != item);
            item->setFlags(item->flags()|Qt::ItemIsUserCheckable);
        }
    }
    else {
        for(int row = 0; row < ui_->tableWidget->rowCount(); row++)
        {
            QTableWidgetItem *item = ui_->tableWidget->item(row, ColumnEnable);
            assert(NULL != item);
            item->setFlags(item->flags()&~Qt::ItemIsUserCheckable);
        }
    }
}

bool DownloadWidget::checkStateEnabledByConfig() const
{
    bool bCheckStateEnabled = true;
    IniItem item("option.ini", "DownloadScene", "Download_Only");
    if (item.hasSection() && !item.GetBooleanValue()) { // table widget disable when Download_Only set false.
        bCheckStateEnabled = false;
    }
    return bCheckStateEnabled;
}

void DownloadWidget::on_checkbox_set_boot_mode_to_meta_clicked()
{
    if(ui_->checkbox_set_boot_mode_to_meta->isChecked())
    {
        EnableBootMode_ComType(true);
        EnableBootMode_ComId(true);
    }
    else
    {
        EnableBootMode_ComType(false);
        EnableBootMode_ComId(false);
    }
}


void DownloadWidget::on_comboBox_authFilePath_activated(int index)
{
    if(index > 0 && index < authFile_historyList_.count())
    {
        LoadAuthFile(authFile_historyList_[index]);
    }
}

void DownloadWidget::on_comboBox_certFilePath_activated(int index)
{
    if(index > 0 && index < certFile_historyList_.count())
    {
        LoadCertFile(certFile_historyList_[index]);
    }
}

bool DownloadWidget::GetCertDLToStorage()
{
    return ui_->radioButton_cert_dl_to_storage->isChecked();
}

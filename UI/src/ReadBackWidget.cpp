#include "ReadBackWidget.h"
#include "ui_ReadBackWidget.h"
#include "MainWindow.h"
#include "MainController.h"
#include "ICallback.h"
#include "CheckHeader.h"
#include "../../Public/AppTypes.h"
#include "../../Logger/Log.h"
#include "ReadBackAddressDialog.h"
#include "../../Setting/PlatformSetting.h"
#include "../../Utility/constString.h"
#include "../../Utility/FileUtils.h"
#include "../../Utility/Utils.h"
#include "../../Utility/constString.h"
#include "../../Utility/IniItem.h"
#include "../../ConsoleMode/SchemaValidator.h"

#include <QCheckBox>
#include <QtGlobal>
#include <QFileDialog>
#include <QDir>
#include <stdio.h>
#include <set>
#include "../../Host/Inc/RuntimeMemory.h"

static const unsigned int kDefaultAddress = 0;
static const unsigned int kDefaultLength = 4096;
static const QString kDefaultReadFlag = "PageSpare";
static const QString kEMMCReadFlag = "N/A";
static const QString kDefaultAddrStr = "0x0000000000000000";
static const QString kDefaultLenStr = "0x0000000000001000";
static const QString kEmmcDefRegionStr="EMMC_USER";
static const QString kUfsDefRegionStr="UFS_LU2";
static const QString kUfsEmmcDefRegionStr = "USER";
static const QString kDefaultFilePrefix = "ROM_";

ReadBackWidget::ReadBackWidget(QTabWidget *parent, MainWindow *window) :
    TabWidgetBase(3, tr("&Readback"), parent),
    ui_(new Ui::ReadBackWidget),
    main_window_(window),
    addr_dialog_(new ReadBackAddressDialog(this)),
    m_header_(new CheckHeader(Qt::Horizontal, this)),
    storage_(HW_STORAGE_EMMC),
    platform_changed_(false),
    readback_xml_loaded(false)
{
    main_window_->main_controller()->GetPlatformSetting()->addObserver(this);
    main_window_->scatter_observer()->addObserver(this);

    connect(this, SIGNAL(signal_platform_changed(HW_StorageType_E &)), addr_dialog_, SLOT(slot_platform_changed(HW_StorageType_E &)));

    ui_->setupUi(this);

    setStyleSheet("QHeaderView::section {background-color:rgba(170,170,255, 50%);}");

    read_flag_map_["PageSpare"] = NUTL_READ_PAGE_SPARE;
    read_flag_map_["PageOnly"] = NUTL_READ_PAGE_ONLY;
    read_flag_map_["SpareOnly"] = NUTL_READ_SPARE_ONLY;
    read_flag_map_["PageWithECC"] = NUTL_READ_PAGE_WITH_ECC;
    read_flag_map_["VerifyAfterProgram"] = NUTL_VERIFY_AFTER_PROGRAM;

    ui_->tableWidget->setColumnHidden(ColumnRegion, true);
    ui_->tableWidget->setHorizontalHeader(m_header_);
    QObject::connect(m_header_,SIGNAL(sectionClicked(int)),
                     this, SLOT(slot_OnHeaderView_click(int)));
    ui_->tableWidget->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    ShowRAMSelectBtn(false);
}

ReadBackWidget::~ReadBackWidget()
{
    if(ui_)
    {
    delete ui_;
        ui_ = NULL;
    }
}

void ReadBackWidget::LockOnUI()
{
    ui_->toolButton_add->setEnabled(false);
    ui_->toolButton_remove->setEnabled(false);
    ui_->toolButton_readBack->setEnabled(false);
    ui_->toolButton_stop->setEnabled(true);
    ui_->m_sram_rb_button->setEnabled(false);
    ui_->m_dram_rb_button->setEnabled(false);
    ui_->tableWidget->setEnabled(false);
}

void ReadBackWidget::DoFinished()
{
    ui_->toolButton_add->setEnabled(true);
    ui_->toolButton_remove->setEnabled(true);
    ui_->toolButton_readBack->setEnabled(true);
    ui_->toolButton_stop->setEnabled(false);
    ui_->m_sram_rb_button->setEnabled(true);
    ui_->m_dram_rb_button->setEnabled(true);
    ui_->tableWidget->setEnabled(true);

    main_window_->main_controller()->ClearAllRBItem();
}

void ReadBackWidget::UpdateUI()
{
    if(platform_changed_)
    {
       platform_changed_ = false;

       QString addr_str;
       U64 addr = 0;
       unsigned int count = ui_->tableWidget->rowCount();

       for(unsigned int i=0; i<count; i++)
       {
           addr = ui_->tableWidget->item(i,ColumnAddr)->text().toULongLong(NULL,16);
           addr_str = Utils::ULLToHex(addr, 20);
           
           if(storage_ == HW_STORAGE_NAND)
           {
              ui_->tableWidget->item(i,ColumnReadFlag)->setText(kDefaultReadFlag);
           }
           else
           {
              ui_->tableWidget->item(i,ColumnReadFlag)->setText(kEMMCReadFlag);
           }
           ui_->tableWidget->item(i,ColumnAddr)->setText(addr_str);
       }
    }
    else
    {
        ui_->retranslateUi(this);

      addr_dialog_->UpdateUI();
   }
}

void ReadBackWidget::UpdateReadBackItemsByScatter(bool scatter_ver2)
{
   LOG("start to show read back items by scatter file.");
   std::list<ImageInfo> image_info_list;
   QTableWidgetItem *item;
   QString addr_str;
   QString len_str;
   QString region_str;

   unsigned int row = 0;

    while(ui_->tableWidget->rowCount()>0)
    {
         ui_->tableWidget->removeRow(0);
    }

   main_window_->main_controller()->GetImageInfoList(image_info_list);
   std::list<ImageInfo>::iterator it = image_info_list.begin();

   for(;it!=image_info_list.end();++it)
   {
      if((it->begin_addr&0xFFFF0000) == 0xFFFF0000)
      {
         break;
      }
      row = ui_->tableWidget->rowCount();
      ui_->tableWidget->insertRow(row); 

      //set name
      LOG("item[%d].name:0x%s", row , it->name.c_str());
      item = new QTableWidgetItem(QString::fromLocal8Bit(it->name.c_str()));
      item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
      ui_->tableWidget->setItem(row, ColumnName, item);
      //set read flag  
      storage_ = main_window_->main_controller()->GetPlatformSetting()->getFlashToolStorageConfig().GetStorageType();
      if(storage_ == HW_STORAGE_NAND)
      {
         item = new QTableWidgetItem(kDefaultReadFlag);
      }
      else
      {
         item = new QTableWidgetItem(kEMMCReadFlag);
      }
      ui_->tableWidget->setItem(row, ColumnReadFlag, item);
      //set address
      addr_str.sprintf("0x%016llx", it->begin_addr);
      LOG("item[%d].address:0x%llx", row , addr_str.data());
      item = new QTableWidgetItem(addr_str);
      item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
      ui_->tableWidget->setItem(row, ColumnAddr, item);

      //set partition length
      len_str.sprintf("0x%016llx", it->partition_size);
      LOG("item[%d].length:%s",row, len_str.data());
      item = new QTableWidgetItem(len_str);
      item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
      ui_->tableWidget->setItem(row, ColumnLength, item);

      //set region
      region_str = EnumToRomString(storage_, it->region, scatter_ver2);
      LOG("item[%d].region:%s", row, region_str.data());
      item = new QTableWidgetItem(region_str);
      item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
      ui_->tableWidget->setItem(row, ColumnRegion, item);

      //set file path
      QString path = FileUtils::GetAppDirectory().c_str();
      path += QDir::separator() + kDefaultFilePrefix + QString::number(row);
      item = new QTableWidgetItem(QDir::toNativeSeparators(path));
      ui_->tableWidget->setItem(row, ColumnFile, item);

      item = ui_->tableWidget->item(row, ColumnEnable);
      if (item == NULL) {
          item = new QTableWidgetItem();
          ui_->tableWidget->setItem(row, ColumnEnable, item);
      }
      item->setCheckState(Qt::Unchecked);
      item->setFlags(item->flags()| Qt::ItemIsUserCheckable);
   }
   m_header_->SetChecked(false);
}

void ReadBackWidget::LoadReadBackFromXMLFile(const std::string &file_name)
{
    if (ValidReadBackXMLFormat(file_name))
    {
        //load readback setting from xml file
        QString fileName = QDir::toNativeSeparators(QString::fromStdString(file_name));
        ConsoleMode::Config config;
        config.LoadFile(fileName.toStdString());

        //validate before load
        if (!ValidateBeforeLoadXML(config))
        {
            return;
        }
        //load readback setting items to ui
        DoLoadReadBackSetting(config);
    }
}

void ReadBackWidget::AppendOneReadBackRow(const ReadbackItem &readbackitem)
{
    QTableWidgetItem *item;
    int row = ui_->tableWidget->rowCount();
    ui_->tableWidget->insertRow(row);

    QString read_flag_str = GetReadFlagStr(readbackitem.read_flag());
    item = new QTableWidgetItem(read_flag_str);
    ui_->tableWidget->setItem(row, ColumnReadFlag, item);


    QString sAddr;
    sAddr.sprintf("0x%016llx", readbackitem.addr());
    item = new QTableWidgetItem(sAddr);
    item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    ui_->tableWidget->setItem(row, ColumnAddr, item);

    QString sLen;
    sLen.sprintf("0x%016llx", readbackitem.len());
    item = new QTableWidgetItem(sLen);
    item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    ui_->tableWidget->setItem(row, ColumnLength, item);

    HW_StorageType_E en_storagetype = main_window_->main_controller()->GetPlatformSetting()->getFlashToolStorageConfig().GetStorageType();
    U32 nRegionID = adaptUfsEmmcCommonRegionID(readbackitem.region(), en_storagetype);
    QString RegionStr = Utils::getRegionStr(nRegionID, en_storagetype);
    item = new QTableWidgetItem(RegionStr);
    item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    ui_->tableWidget->setItem(row, ColumnRegion, item);

    QString path = QString::fromStdString(readbackitem.path());
    item = new QTableWidgetItem(QDir::toNativeSeparators(path));
    ui_->tableWidget->setItem(row, ColumnFile, item);

    item = ui_->tableWidget->item(row, ColumnEnable);
    if (item == NULL) {
        item = new QTableWidgetItem();
        ui_->tableWidget->setItem(row, ColumnEnable, item);
    }
    item->setCheckState(readbackitem.enable() ? Qt::Checked : Qt::Unchecked);
    item->setFlags(item->flags()| Qt::ItemIsUserCheckable);
}

void ReadBackWidget::DoLoadReadBackSetting(ConsoleMode::Config &config)
{
    //ui add readback setting item from readback setting loaded from xml file
    std::list<QSharedPointer<ISetting> > cmdList = config.pclGetCommandSetting()->aclGetCmdSettingList();
    std::list<QSharedPointer<ISetting> >::iterator it;
    for (it = cmdList.begin(); it != cmdList.end(); ++it)
    {
        QSharedPointer<APCore::ReadbackSetting> readbackSetting = it->dynamicCast<APCore::ReadbackSetting>();
        if (NULL != readbackSetting)
        {
            const std::list<ReadbackItem> readbackitems = readbackSetting->GetReadbackItems();
            for (std::list<ReadbackItem>::const_iterator cit = readbackitems.begin(); cit != readbackitems.end(); ++cit)
            {
                AppendOneReadBackRow(*cit);
            }
            UpdateHeadViewCheckState();
        }
    }
}

bool ReadBackWidget::ValidateBeforeLoadXML(ConsoleMode::Config &config) const
{
    if (ValidateDAScatterFileLoaded(false))
    {
        return ValidatePlatformMatched(config);
    }
    return false;
}

bool ReadBackWidget::ValidateDAScatterFileLoaded(bool showPrompt) const
{
    if(!main_window_->IsDALoad(showPrompt))
    {
        return false;
    }

    if(!main_window_->IsScatterFileLoad(showPrompt))
    {
        return false;
    }
    return true;
}

bool ReadBackWidget::ValidatePlatformMatched(ConsoleMode::Config &config) const
{
    //validate xml file platform info to platform info from scatter file
    std::string chipName_xml = config.pclGetGeneralSetting()->pclGetGeneralArg()->chip_name;
    HW_StorageType_E en_storage_type_xml = config.pclGetGeneralSetting()->pclGetGeneralArg()->storage_type;

    std::string chipName = main_window_->main_controller()->GetPlatformSetting()->getFlashToolStorageConfig().GetPlatform();
    HW_StorageType_E en_storage_type = main_window_->main_controller()->GetPlatformSetting()->getFlashToolStorageConfig().GetStorageType();

    return (chipName_xml == chipName) && (en_storage_type_xml == en_storage_type);
}

bool ReadBackWidget::ValidReadBackXMLFormat(const std::string &file_name) const
{
    if (FileUtils::IsFileExist(file_name))
    {
        ConsoleMode::SchemaValidator schemaValidator(Utils::GetTextCodec()->toUnicode(ABS_PATH_C("readback_ui_bak.xsd")));
        try
        {
            schemaValidator.Validate(QString::fromStdString(file_name));
            return true;
        }
        catch (...)
        {
            LOGI("Invalid readback setting xml file.");
            return false;
        }
    }
    return false;
}

bool ReadBackWidget::IsLoadedByScatter() const
{
    IniItem item("option.ini", "ReadBack", "ShowByScatter");
    return item.GetBooleanValue();
}

U32 ReadBackWidget::adaptUfsEmmcCommonRegionID(U32 nRegionID, HW_StorageType_E en_storage_type) const
{
    U32 nAdaptRegionID = nRegionID;
    //adapt UfsEmmcCommon region id and ufs or emmc region id by storage type, adapt for auto load readback items when run flashtool
    U32 UfsEmmcCommonID[] = {BOOT1_PART, BOOT2_PART, USER_PART};
    std::set<U32> UfsEmmcCommonSet(UfsEmmcCommonID, UfsEmmcCommonID + 3);
    bool bScatter_ver2_by_region = UfsEmmcCommonSet.count(nRegionID) != 0;
    bool bCur_scatter_ver2 = main_window_->IsScatterVer2();
    if (bScatter_ver2_by_region && !bCur_scatter_ver2)
    {
        switch (nRegionID)
        {
        case BOOT1_PART:
            nAdaptRegionID = en_storage_type == HW_STORAGE_UFS ? UFS_PART_LU0 : EMMC_BOOT1;
            break;
        case BOOT2_PART:
            nAdaptRegionID = en_storage_type == HW_STORAGE_UFS ? UFS_PART_LU1 : EMMC_BOOT2;
            break;
        case USER_PART:
            nAdaptRegionID = en_storage_type == HW_STORAGE_UFS ? UFS_PART_LU2 : EMMC_RPMB;
            break;
        }
    }
    else if (!bScatter_ver2_by_region && bCur_scatter_ver2)
    {
        switch (nRegionID)
        {
        case 1: //means UFS_PART_LU0 for UFS or EMMC_BOOT1 for EMMC
            nAdaptRegionID = BOOT1_PART;
            break;
        case 2: //means UFS_PART_LU1 for UFS or EMMC_BOOT2 for EMMC
            nAdaptRegionID = BOOT2_PART;
            break;
        case 3: //means UFS_PART_LU2 for UFS or EMMC_RPMB for EMMC
            nAdaptRegionID = USER_PART;
            break;
        }
    }
    return nAdaptRegionID;
}

bool ReadBackWidget::HasUnCheckedRBItem() const
{
    bool hasUnChecked = ui_->tableWidget->rowCount() == 0;
    for (int i = 0; i < ui_->tableWidget->rowCount(); ++i)
    {
        QTableWidgetItem *pItem = ui_->tableWidget->item(i, ColumnEnable);
        Q_ASSERT(NULL != pItem);
        if (pItem->checkState() == Qt::Unchecked)
        {
            hasUnChecked = true;
            break;
        }
    }
    return hasUnChecked;
}

void ReadBackWidget::SetAllRBItemCheckState(Qt::CheckState checkState)
{
    for (int i = 0; i < ui_->tableWidget->rowCount(); ++i)
    {
        QTableWidgetItem *pItem = ui_->tableWidget->item(i, ColumnEnable);
        Q_ASSERT(NULL != pItem);
        if (pItem->checkState() != checkState)
        {
            pItem->setCheckState(checkState);
        }
    }
}

void ReadBackWidget::UpdateHeadViewCheckState()
{
    m_header_->SetChecked(HasUnCheckedRBItem() ? false : true);
}

bool ReadBackWidget::IsSRAMReadback()
{
    return ui_->m_sram_rb_button->isChecked();
}

void ReadBackWidget::ShowRAMSelectBtn(bool show)
{
    ui_->m_sram_rb_button->setVisible(show);
    ui_->m_dram_rb_button->setVisible(show);
}

void ReadBackWidget::SetTabLabel(QTabWidget *tab_widget, int index)
{
    QString label = LoadQString(LANGUAGE_TAG, IDS_STRING_READ_BACK);

    tab_widget->setTabText(index, label);
}

void ReadBackWidget::SetShortCut(int cmd, const QString &shortcut)
{
    switch(cmd)
    {
    case RB_ADD:
        ui_->toolButton_add->setShortcut(shortcut);
        ui_->toolButton_add->setToolTip(shortcut);
        break;

    case RB_REMVOE:
        ui_->toolButton_remove->setShortcut(shortcut);
        ui_->toolButton_remove->setToolTip(shortcut);
        break;

    case RB_START:
        ui_->toolButton_readBack->setShortcut(shortcut);
        ui_->toolButton_readBack->setToolTip(shortcut);
        break;

    case RB_STOP:
        ui_->toolButton_stop->setShortcut(shortcut);
        ui_->toolButton_stop->setToolTip(shortcut);
        break;

    }
}

void ReadBackWidget::on_toolButton_readBack_clicked()
{
    if(ValidateBeforeReadback())
    {
        main_window_->main_controller()->SetPlatformSetting();

        main_window_->main_controller()->SetConnSetting(main_window_->CreateConnSetting());
        main_window_->main_controller()->QueueAJob(main_window_->CreateReadbackSetting());

        if(main_window_->isAutoPollingEnable())
            main_window_->main_controller()->QueueAJob(main_window_->CreateWatchDogSetting());

        main_window_->main_controller()->StartExecuting(
                    new SimpleCallback<MainWindow>(main_window_,&MainWindow::DoFinished));
        main_window_->LockOnUI();
        main_window_->GetOkDialog()->setWindowTitle(LoadQString(main_window_->GetLanguageTag(), IDS_STRING_READ_BACK_OK));
    }
}

void ReadBackWidget::on_toolButton_stop_clicked()
{
    main_window_->main_controller()->StopByUser();
}

void ReadBackWidget::on_toolButton_add_clicked()
{
    if (!ValidateDAScatterFileLoaded(true))
    {
        return;
    }

    storage_ = main_window_->main_controller()->GetPlatformSetting()->getFlashToolStorageConfig().GetStorageType();

    QString read_flag_str = (storage_ == HW_STORAGE_NAND) ? kDefaultReadFlag : kEMMCReadFlag;
    NUTL_ReadFlag_E read_flag = ParseReadFlag(read_flag_str);

    U64 addr = kDefaultAddrStr.toULongLong(NULL,16);

    U64 len  = kDefaultLenStr.toULongLong(NULL,16);

    int row = ui_->tableWidget->rowCount();
    QString path = FileUtils::GetAppDirectory().c_str();
    path += QDir::separator() + kDefaultFilePrefix + QString::number(row);
    path = QDir::toNativeSeparators(path);

    bool scatter_ver2 = main_window_->IsScatterVer2();
    QString defaultRegionStr;
    if (scatter_ver2)
    {
        defaultRegionStr = kUfsEmmcDefRegionStr;
    }
    else if(HW_STORAGE_EMMC ==  storage_)
    {
        defaultRegionStr = kEmmcDefRegionStr;
    }
    else if (HW_STORAGE_UFS ==  storage_)
    {
        defaultRegionStr = kUfsDefRegionStr;
    }
    U32 region = Utils::getRegionPartId(defaultRegionStr);

    NUTL_AddrTypeFlag_E addr_flag =
            main_window_->IsPhysicalFmtandReadback() ? NUTL_ADDR_PHYSICAL : NUTL_ADDR_LOGICAL;

    ReadbackItem item(row, true, addr, len, path.toStdString(), read_flag, region, addr_flag);
    AppendOneReadBackRow(item);
    UpdateHeadViewCheckState();
}

void ReadBackWidget::on_toolButton_remove_clicked()
{
    int row = ui_->tableWidget->currentRow();
    if(row >=0)
    {
         ui_->tableWidget->removeRow(row);
    }
    UpdateHeadViewCheckState();
}

ReadbackItem ReadBackWidget::GetRBItem(int row)
{
    QTableWidgetItem *pItem = ui_->tableWidget->item(row, ColumnEnable);
    Q_ASSERT(NULL != pItem);
    bool enable = pItem->checkState() == Qt::Checked;

    NUTL_ReadFlag_E read_flag = ParseReadFlag(
                ui_->tableWidget->item(row, ColumnReadFlag)->text());

    U64 addr =
            ui_->tableWidget->item(row, ColumnAddr)->text().toULongLong(NULL,16);

    U64 len  =
            ui_->tableWidget->item(row, ColumnLength)->text().toULongLong(NULL,16);

    QString file = ui_->tableWidget->item(row, ColumnFile)->text();

    U32 region = Utils::getRegionPartId(
                ui_->tableWidget->item(row, ColumnRegion)->text());

    NUTL_AddrTypeFlag_E addr_flag =
            main_window_->IsPhysicalFmtandReadback() ? NUTL_ADDR_PHYSICAL : NUTL_ADDR_LOGICAL;

    ReadbackItem item(row,enable,addr,len,string(file.toLocal8Bit()),read_flag, region, addr_flag);

    return item;
}

void ReadBackWidget::AppendRBItem(int row)
{
    ReadbackItem item = GetRBItem(row);

    main_window_->main_controller()->AppendRBItem(item);
}

void ReadBackWidget::SetReadbackListItem(
    QSharedPointer<APCore::ReadbackSetting> &readback_setting)
{
    std::list<ReadbackItem> items;

    main_window_->main_controller()->ClearAllRBItem();

    for(int i = 0; i < ui_->tableWidget->rowCount(); i++)
    {
        ReadbackItem rbitem = GetRBItem(i);

        items.push_back(rbitem);
    }

    readback_setting->set_readbackItems(items);
}

void ReadBackWidget::DeleteRBItem(int row)
{
    main_window_->main_controller()->RemoveRBItem(row);
}

NUTL_ReadFlag_E ReadBackWidget::ParseReadFlag(const QString &flag_str)
{
    NUTL_ReadFlag_E ret = NUTL_READ_PAGE_ONLY;

    if(flag_str == "N/A")
    {
        return ret;
    }

    std::map<QString,NUTL_ReadFlag_E>::const_iterator it = read_flag_map_.find(flag_str);
    if(it != read_flag_map_.end())
    {
        ret = it->second;
    }
    else
    {
        LOG("Unknown read flag: %s",flag_str.constData()->toAscii());
        ret = NUTL_READ_FLAG_END;
        Q_ASSERT(0 && "unknown read flag");
    }

    return ret;
}

QString ReadBackWidget::GetReadFlagStr(NUTL_ReadFlag_E flag)
{
    for(std::map<QString,NUTL_ReadFlag_E>::const_iterator it = read_flag_map_.begin();
        it != read_flag_map_.end(); ++it)
    {
        if(it->second == flag)
        {
            return it->first;
        }
    }
    Q_ASSERT(0 && "unknown read flag");

    return "";
}

void ReadBackWidget::on_tableWidget_cellDoubleClicked(int row, int column)
{
    Q_UNUSED(column);

    QString file_name = ui_->tableWidget->item(row,ColumnFile)->text();

    file_name = QFileDialog::getSaveFileName(this,tr("Save File"), file_name);
    if(!file_name.isEmpty())
    {
        file_name = QDir::toNativeSeparators(file_name);

        ui_->tableWidget->item(row,ColumnFile)->setText(file_name);

        NUTL_ReadFlag_E read_flag = ParseReadFlag(
                    ui_->tableWidget->item(row,ColumnReadFlag)->text());

        U32 region_id = Utils::getRegionPartId(
                    ui_->tableWidget->item(row, ColumnRegion)->text());

        //pop out the address dialog
        addr_dialog_->SetCurrentState(read_flag,
                                     ui_->tableWidget->item(row,ColumnAddr)->text(),
                                     ui_->tableWidget->item(row,ColumnLength)->text(),
                                      region_id);
        int cnf = addr_dialog_->exec();
        if(cnf == QDialog::Accepted)
        {
            //modify...
            NUTL_ReadFlag_E read_flag = addr_dialog_->read_flag();
            U64 addr = addr_dialog_->addr();
            U64 len  = addr_dialog_->length();
            U32 region_id = addr_dialog_->region_id();

            bool isAlign = ValidateAddrAlignment(addr, len);
            if(isAlign)
            {
                QString addr_str;
                if(storage_ == HW_STORAGE_NAND)
                {
                    QString read_flag_str = GetReadFlagStr(read_flag);
                    ui_->tableWidget->item(row,ColumnReadFlag)->setText(read_flag_str);
                }

                addr_str.sprintf("0x%016llx", addr);
     
                ui_->tableWidget->item(row,ColumnAddr)->setText(addr_str);

                QString len_str;
                len_str.sprintf("0x%016llx",len);
                ui_->tableWidget->item(row,ColumnLength)->setText(len_str);

                ui_->tableWidget->item(row, ColumnRegion)->setText(addr_dialog_->region());

                QTableWidgetItem *pItem = ui_->tableWidget->item(row, ColumnEnable);
                Q_ASSERT(NULL != pItem);
                bool enable = pItem->checkState() == Qt::Checked;

                QString file = ui_->tableWidget->item(row,ColumnFile)->text();

                ReadbackItem item(row, enable, addr,len,string(file.toLocal8Bit()),read_flag, region_id);

                LOG("updating Readback Item(%d)", item.index());
            }
            else
            {
                main_window_->ShowHelpContents(NULL, IDS_STRING_ERROR_ALIGN, "E_APP_ADDR_ALIGN_RB.htm");
            }
        }
    }
}

void ReadBackWidget::slot_OnHeaderView_click(int index)
{
    if(index == ColumnEnable)
    {
        bool checked = m_header_->GetChecked();

        SetAllRBItemCheckState(checked ? Qt::Checked : Qt::Unchecked);
    }
}

void ReadBackWidget::on_tableWidget_cellClicked(int row, int column)
{
    Q_UNUSED(row);
    if (column == ColumnEnable)
    {
        UpdateHeadViewCheckState();
    }
}

bool ReadBackWidget::ValidateAddrAlignment(U64 addr, U64 len)
{
    ReadbackRule readback_rule;
    unsigned int align_number(0);
    HW_StorageType_E storage_type = main_window_->main_controller()->GetPlatformSetting()->getFlashToolStorageConfig().GetStorageType();

    readback_rule.QueryReadBackRule(storage_type);
    align_number = readback_rule.align_number();

    if((addr%align_number)!= 0 )
    {
        return false;
    }

    if((len%align_number)!=0  || len == 0)
    {
        return false;
    }

    return true;
}

bool ReadBackWidget::ValidateBeforeReadback() const
{
    if (!ValidateDAScatterFileLoaded(true))
    {
        return false;
    }

    bool has_item = false;

    unsigned int count = ui_->tableWidget->rowCount();
    for(unsigned int i=0; i<count; i++)
    {
        QTableWidgetItem *pItem = ui_->tableWidget->item(i, ColumnEnable);
        Q_ASSERT(NULL != pItem);
        if(pItem->checkState() == Qt::Checked)
        {
            has_item = true;
            break;
        }
    }
    if(!has_item)
    {
        main_window_->ShowHelpContents(const_cast<ReadBackWidget *const>(this), IDS_STRING_WARNING_ADDRB, "E_APP_ADD_RB.htm");
    }
    return has_item;
}

void ReadBackWidget::onPlatformChanged()
{
     platform_changed_ = true;

     storage_ = main_window_->main_controller()->GetPlatformSetting()->getFlashToolStorageConfig().GetStorageType();
/*
     if(storage_ == HW_STORAGE_EMMC)
     {
         ui_->tableWidget->setColumnHidden(ColumnReadFlag, true);
     }
     else
     {
         ui_->tableWidget->setColumnHidden(ColumnReadFlag, false);
     }
*/
     this->UpdateUI();

     addr_dialog_->SetStorageType(storage_);

     emit signal_platform_changed(storage_);
}

void ReadBackWidget::OnScatterChanged(bool showRegion, bool scatter_ver2)
{
	//add for customer support requirement:show readback items by partitions in scatter file
    if(this->IsLoadedByScatter())
	{
	   ui_->tableWidget->setColumnHidden(ColumnName, false);
       UpdateReadBackItemsByScatter(scatter_ver2);
	}
	///////////////////////////////////
	else
    {
        ui_->tableWidget->setColumnHidden(ColumnName, true);
        ui_->tableWidget->setColumnHidden(ColumnRegion, !showRegion);
        int row_count = ui_->tableWidget->rowCount();

        if(showRegion)
        {
            QString defaultRegionStr;
            if (scatter_ver2)
            {
                defaultRegionStr = kUfsEmmcDefRegionStr;
            }
            else if(HW_STORAGE_EMMC ==  storage_)
            {
                defaultRegionStr = kEmmcDefRegionStr;
            }
            else if (HW_STORAGE_UFS ==  storage_)
            {
                defaultRegionStr = kUfsDefRegionStr;
            }
            for (int i = 0; i < row_count; ++i )
            {
                ui_->tableWidget->item(i,ColumnRegion)->setText(defaultRegionStr);
            }
        }
	}
    addr_dialog_->OnScatterChanged(showRegion, scatter_ver2);
}

void ReadBackWidget::ShowReadFlagColumn(bool show)
{
    ui_->tableWidget->setColumnHidden(ColumnReadFlag, !show);
}

int ReadBackWidget::GetRBCount() const
{
    return ui_->tableWidget->rowCount();
}

void ReadBackWidget::AutoLoadReadBackSetting()
{
    /* priority:
     * 1. auto load by scatter;
     * 2. auto load by readback_ui_bak.xml file.
     */
    if (!readback_xml_loaded && !IsLoadedByScatter())
    {
        LoadReadBackFromXMLFile(ABS_PATH("readback_ui_bak.xml"));
    }
    // only try once to load items from readback_ui_bak.xml when first time to this page.
    // prevent ShowByScatter option changed manually when tool running.
    readback_xml_loaded = true;
}

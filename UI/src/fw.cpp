#include "fw.h"
#include "ui_fw.h"
#include "../../Utility/FileUtils.h"
#include "MainController.h"
#include "ICallback.h"
#include <QFile>
#include <QFileDialog>

FW::FW(QTabWidget *parent, MainWindow *window) :
    TabWidgetBase(11, tr("Firmware Udpate"), parent),
    main_window_(window),
    ui(new Ui::FW),
    file_length_(0)
{
    ui->setupUi(this);
}

FW::~FW()
{
	if(ui)
    {
        delete ui;
        ui = NULL;
    }
}

void FW::on_toolButton_openFwFile_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(
                this,
                tr("Open UFS Firmware Upgrade File"),
                FileUtils::GetAppDirectory().c_str(),
                tr("All Files (*.*)"));

    if(!file_name.isEmpty())
    {
        QFileInfo fileInfo(file_name);
        file_length_ = fileInfo.size();

        ui->lineEdit_fwfile->setText(file_name);
    }
    else
    {
        file_length_ = 0;
        ui->lineEdit_fwfile->setText("");
    }
}

void FW::LockOnUI()
{
    ui->lineEdit_fwfile->setEnabled(false);
    ui->pushButton_Start->setEnabled(false);
    ui->pushButton_Stop->setEnabled(true);
    ui->toolButton_openFwFile->setEnabled(false);
}

void FW::DoFinished()
{
    ui->lineEdit_fwfile->setEnabled(true);
    ui->pushButton_Start->setEnabled(true);
    ui->pushButton_Stop->setEnabled(false);
    ui->toolButton_openFwFile->setEnabled(true);
}

void FW::UpdateUI()
{
    ui->retranslateUi(this);
}

void FW::SetTabLabel(QTabWidget *tab_widget, int index)
{
    QString label = "Firmware Upgrade";

    tab_widget->setTabText(index, label);
}

void FW::SetShortCut(int cmd, const QString &shortcut)
{
    Q_UNUSED(cmd);
    Q_UNUSED(shortcut);
}


void FW::on_pushButton_Stop_clicked()
{
    main_window_->main_controller()->StopByUser();
}

void FW::on_pushButton_Start_clicked()
{
    if(ui->lineEdit_fwfile->text().isEmpty())
    {
        flashtool_message_box(this, NULL, CRITICAL_MSGBOX, "Firmware Update", "Please select firmware file first", "OK");
        return;
    }
    main_window_->main_controller()->SetPlatformSetting();
    main_window_->main_controller()->SetConnSetting(main_window_->CreateConnSetting());
    main_window_->main_controller()->QueueAJob(main_window_->CreateFWSetting());
    main_window_->main_controller()->StartExecuting(
                new SimpleCallback<MainWindow>(main_window_,&MainWindow::DoFinished));
    main_window_->LockOnUI();
    main_window_->GetOkDialog()->setWindowTitle("Firmware upgrade");
}

QSharedPointer<APCore::FWSetting> FW::CreateFWSetting()
{
    QSharedPointer<APCore::FWSetting> setting(new APCore::FWSetting());

    setting->set_fw_file(ui->lineEdit_fwfile->text().toStdString());

    return setting;
}

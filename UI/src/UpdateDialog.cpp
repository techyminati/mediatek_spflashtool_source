#include "UpdateDialog.h"
#include "ui_UpdateDialog.h"

#include <QThreadPool>
#include "../../UI/src/AsyncUpdater.h"
#include "../../UI/src/MainWindow.h"
#include "../../Host/Inc/RuntimeMemory.h"
#include "DownloadWidget.h"

UpdateDialog::UpdateDialog(MainWindow *parent, AsyncUpdater *updater) :
    QDialog(parent),
    async_updater(updater),
    ui(new Ui::UpdateDialog),
    mainWindow(parent)
{
    ui->setupUi(this);

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    //TODO: read release notes into text edit
    ui->textBrowser_note->setText(async_updater->GetReleaseNotes());

    UpdateUI();
}

UpdateDialog::~UpdateDialog()
{
    if(ui)
    {
        delete ui;
        ui = NULL;
    }
}

void UpdateDialog::on_pushButton_remind_clicked()
{
    //TODO: When to re update

    close();
}

#define CURRENT_PROCESS_NAME "flash_tool.exe"
void UpdateDialog::on_pushButton_install_clicked()
{
#ifdef _WIN32
    if (IsMoreThanOneProcessByName(CURRENT_PROCESS_NAME))
    {
        flashtool_message_box(this, 0, INFORMATION_MSGBOX,
                              LoadQString(mainWindow->GetLanguageTag(), IDS_STRING_UPDATE_TITLE),
                              LoadQString(mainWindow->GetLanguageTag(), IDS_STRING_UPDATE_MULTI_PROCESS),
                              LoadQString(mainWindow->GetLanguageTag(), IDS_STRING_OK));
    }
    else
    {
        mainWindow->downloadWidget()->UserCancelLoadScatter();
        QThreadPool::globalInstance()->waitForDone();
        async_updater->AsyncUpdateTool();
    }
#endif
}

void UpdateDialog::UpdateUI()
{
    this->setWindowTitle(LoadQString(mainWindow->GetLanguageTag(), IDS_STRING_UPDATE));
    ui->label->setText(LoadQString(mainWindow->GetLanguageTag(), IDS_STRING_NEW_DETECT));
    ui->pushButton_remind->setText(LoadQString(mainWindow->GetLanguageTag(), IDS_STRING_REMIND));
    ui->pushButton_install->setText(LoadQString(mainWindow->GetLanguageTag(), IDS_STRING_INSTALL_UPDATE));
}

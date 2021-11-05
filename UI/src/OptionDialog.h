#ifndef OPTIONDIALOG_H
#define OPTIONDIALOG_H

#include <QDialog>
#include <QString>
#include <QSharedPointer>
#include <QTreeWidgetItem>
#include <QTranslator>
#include <vector>
#include <map>
#include <string>

#include "../../Setting/ConnSetting.h"
#include "../../Setting/PlatformSetting.h"
#include "../../Setting/ConnBromUSBSetting.h"
#include "../../Setting/LogInfoSetting.h"

namespace Ui {
class OptionDialog;
}

class MainWindow;
class LogCleanThread;
class OptionDialog : public QDialog, public APCore::IPlatformOb
{
    Q_OBJECT

public:
    explicit OptionDialog(QWidget *parent, MainWindow *mainWindow);
    ~OptionDialog();

    virtual void onPlatformChanged();

#ifdef _WIN32
    virtual bool winEvent(MSG *message, long *result);
#endif

    QSharedPointer<APCore::ConnSetting> CreateConnSetting();
    QSharedPointer<APCore::LogInfoSetting> CreateLogInfoSetting();

    QString GetDescription();

    QString GetUSBSpeed();

    void InitUARTInfo();

    void InitSettings();

    void SaveSettings();

    void LoadGeneralSettings();

    void SaveGeneralSettings();

    void LoadConnSettings();

    void SaveConnSettings();

    void ShowPhysicalFormatItem(bool show);

    void ShowCheckRPMBItem(bool show);

    bool isRPMBChecked();

    void SetPlatformSetting(const QSharedPointer<APCore::PlatformSetting> &platform_setting);

    void SetPowerSetting(bool autoDetect);

    void UpdateUI();

    void SetLabelText(int index);

    QString GetKeyboardModify(int row);

    void SetShortcut(int row);

    int GetChecksumLevel();

    bool LibDAMatchChecked();

protected:
    virtual void showEvent(QShowEvent *event);

private:
    APCore::USBPower GetPowerControl();
    void ReInitItems();
    void InitThemeItems();
    void ShowCustomThemeSetting(bool show);
    void AdjustTime();
    void InitShortKeyTable();

    void InitKeyMapInfo();
    QString GetBindFromVector(const int index);

    void ResetShortKey(int row);
    void SetCurrentKey(int row);
    void EnableConnControl(int index);
    void EnableControl(bool enable);
    void startCleanThread();
    qint64 GetHoursFromDateEnum() const;
    bool CheckStorageLifeCycle() const;
    void InitUpdateOptions();

private:
    MainWindow *main_window_;
    Ui::OptionDialog *ui;
    QTranslator app_translator;
    QSharedPointer<APCore::PlatformSetting> platform_setting;

    std::vector<std::string> bind_vector;
    std::map<std::string, std::string> default_bind_map;

    LogCleanThread *m_workerThread;

private slots:

    void onToggleUSB_UART(bool toggled);

    void on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

    void on_comboBox_Language_activated(int index);

    void on_comboBoxTheme_currentIndexChanged(int index);

    void on_checkBox_ShowWelcome_clicked();

    void on_checkBox_physicalFormat_clicked();

    void on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    void on_checkBox_RestoreStartup_clicked();

    void on_checkBox_clicked();

    void on_checkBox_enableTracing_clicked();

    void on_checkBox_usbchksum_toggled(bool checked);

    void on_checkBox_emmcchksum_toggled(bool checked);

    void slot_SetUARTBaudrateIndex(unsigned int);

    void on_checkBox_rpmb_clicked();

    void on_btn_logPath_clicked();

    void on_checkBox_StorageLifeCycle_clicked();

    void slot_get_uart_com_ports(const std::list<std::string> &uart_com_ports);

signals:
    void signal_enable_DAChksum(int Chksum_type);
    void signal_language_change();

};

#endif // OPTIONDIALOG_H

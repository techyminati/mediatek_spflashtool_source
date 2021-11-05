#ifndef MEMORYTESTWIDGET_H
#define MEMORYTESTWIDGET_H

#include "TabWidgetBase.h"
#include "../../Setting/MemoryTestSetting.h"
#include "../../Setting/PlatformSetting.h"
#include "../../Setting/DRAMRepairSetting.h"
#include "ScatterObserver.h"

#include <QColor>
#include <QSharedPointer>

namespace Ui
{
class MemoryTestWidget;
}
class MainWindow;

class MemoryTestWidget : public TabWidgetBase, public APCore::IPlatformOb, public IScatterObj
{
    Q_OBJECT
public:
    explicit MemoryTestWidget(QTabWidget *parent, MainWindow *window);
    ~MemoryTestWidget();

    DECLARE_TABWIDGET_VFUNCS()

    virtual void onPlatformChanged();
    virtual void OnScatterChanged(bool showRegion, bool scatter_ver2);

    QSharedPointer<APCore::MemoryTestSetting> CreateMemtestSetting();
    QSharedPointer<APCore::DRAMRepairSetting> CreateDRAMRepairSetting();

    bool isSelectNothing();
    void ShowEMMCTest(bool isShow);
    void ShowNANDTest(bool isShow);
    void ShowRAMTest(bool isShow);
    void ShowDRAMFlipTest(bool isShow);
    void ShowDRAMRepairBtn(bool isShow);
    void clearUIText();
    void OnDRAMRepairFinished();

private:
    void HideFlashTestItems();
    void selectAllTestItems(bool select);

    MainWindow *main_window_;
    Ui::MemoryTestWidget *ui_;
    QStringList mEmmcRegionList;
    QStringList mUfsEmmcRegionList;
    int m_dram_repair_status_code;


signals:
    void signal_dram_repair_finished();

public slots:

private slots:
    void on_toolButton_start_clicked();
    void on_SELECT_ALL_clicked();
    void on_toolButton_repair_clicked();
    void slot_dram_repair_finished();

    void slot_MemoryTestCallback(const QString &msg, QColor color);
    void slot_MemoryTestRepairCallback(int op_status_code);
    void on_toolButton_stop_clicked();
    void on_radioButton_dramtest_manual_clicked(bool checked);
    void on_radioButton_dramtest_auto_clicked(bool checked);
};

#endif // MEMORYTESTWIDGET_H

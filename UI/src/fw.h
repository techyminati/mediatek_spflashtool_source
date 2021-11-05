#ifndef FW_H
#define FW_H

#include <QWidget>
#include "TabWidgetBase.h"
#include "MainWindow.h"

namespace Ui {
class FW;
}

class MainWindow;

class FW : public TabWidgetBase
{
    Q_OBJECT
    
public:
    explicit FW(QTabWidget *parent,  MainWindow *window);
    DECLARE_TABWIDGET_VFUNCS()
    QSharedPointer<APCore::FWSetting> CreateFWSetting();
    ~FW();
    
private slots:
    void on_toolButton_openFwFile_clicked();

    void on_pushButton_Stop_clicked();

    void on_pushButton_Start_clicked();

private:
    MainWindow *main_window_;
    Ui::FW *ui;
    uint file_length_;
};

#endif // FW_H

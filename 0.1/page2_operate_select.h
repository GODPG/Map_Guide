#ifndef PAGE2_OPERATE_SELECT_H
#define PAGE2_OPERATE_SELECT_H

#include <QWidget>

namespace Ui {
class page2_operate_select;
}

class page2_operate_select : public QWidget
{
    Q_OBJECT

public:
    explicit page2_operate_select(QWidget *parent = nullptr);
    ~page2_operate_select();

signals:
    // 返回登录界面
    void backToLogin();

    // 进入地图导入/标注界面
    void openMapImport();

    // 进入地图选择界面
    void openMapSelect();

    // 触发数据导出
    void exportDataRequested();

private slots:
    void onExitClicked();
    void onExportClicked();
    void onImportClicked();
    void onSelectClicked();

private:
    Ui::page2_operate_select *ui;
};

#endif // PAGE2_OPERATE_SELECT_H

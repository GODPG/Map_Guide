#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 前向声明，避免头文件互相 include 过多
class page1_login;
class page2_operate_select;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 登录成功后切换到功能选择页面
    void handleLoginSuccess(const QString &username);

    // 从功能选择页退出/返回登录
    void handleLogoutRequested();

    // 从功能选择页进入“地图导入 / 标注”页面
    void handleOpenMapImport();

    // 从功能选择页进入“地图选择”页面
    void handleOpenMapSelect();

    // 从功能选择页触发导出（暂时占位）
    void handleExportDataRequested();

private:
    Ui::MainWindow *ui;
    QString m_currentUser;   // 保存当前登录用户名

    // 统一管理各个页面索引，避免魔法数字
    enum PageIndex {
        PageLogin = 0,
        PageOperateSelect = 1,
        PageMapLabGuide = 2,
        PageMapSelect = 3
    };

    void setupPages();                // 初始化页面指针与信号连接
    void showPage(PageIndex index);   // 切换页面并做一些状态更新
};

#endif // MAINWINDOW_H

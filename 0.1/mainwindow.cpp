#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "page1_login.h"
#include "page2_operate_select.h"

// 如果你已经创建了 page2_1_map_lab_guide 类，可以取消注释这一行
#include "page2_1_map_lab_guide.h"
#include "page2_2_map_select.h"

#include <QMessageBox>
#include <QStatusBar>
#include <QStackedWidget>

namespace {

// 通用：从 stackedWidget 的某个 index 获取页面；如果没有则创建并插入
template<typename T>
T* ensurePage(QStackedWidget *stack, int index, QWidget *parent)
{
    if (!stack) return nullptr;

    if (index >= 0 && index < stack->count()) {
        if (auto *p = qobject_cast<T*>(stack->widget(index)))
            return p;
    }

    auto *p = new T(parent);
    stack->insertWidget(index, p);
    return p;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupPages();
    showPage(PageLogin);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupPages()
{
    auto *stack = ui->stackedWidget;
    if (!stack) return;

    // 0/1/2/3 四个页面：确保存在并拿到指针
    auto *loginPage     = ensurePage<page1_login>(stack, PageLogin, this);
    auto *operatePage   = ensurePage<page2_operate_select>(stack, PageOperateSelect, this);
    auto *labGuidePage  = ensurePage<page2_1_map_lab_guide>(stack, PageMapLabGuide, this);
    auto *mapSelectPage = ensurePage<page2_2_map_select>(stack, PageMapSelect, this);

    // -------- 登录页 -> MainWindow --------
    connect(loginPage, &page1_login::loginSuccess,
            this, &MainWindow::handleLoginSuccess);

    // -------- 功能选择页 -> MainWindow --------
    connect(operatePage, &page2_operate_select::backToLogin,
            this, &MainWindow::handleLogoutRequested);

    connect(operatePage, &page2_operate_select::openMapImport,
            this, &MainWindow::handleOpenMapImport);

    connect(operatePage, &page2_operate_select::openMapSelect,
            this, &MainWindow::handleOpenMapSelect);

    connect(operatePage, &page2_operate_select::exportDataRequested,
            this, &MainWindow::handleExportDataRequested);

    // -------- 标注页 -> MainWindow --------
    connect(labGuidePage, &page2_1_map_lab_guide::backToMenu,
            this, [this](){ showPage(PageOperateSelect); });

    // （可选）标注页数据变更
    // connect(labGuidePage, &page2_1_map_lab_guide::mapModified,
    //         this, [](){ /* 以后做自动保存 */ });

    // -------- 地图选择页 -> MainWindow --------
    connect(mapSelectPage, &page2_2_map_select::backToMenu,
            this, [this](){ showPage(PageOperateSelect); });

    connect(mapSelectPage, &page2_2_map_select::requestOpenProject,
            this, [this, labGuidePage](const QString &path, bool isNew){
                if (!labGuidePage->loadProject(path, isNew)) {
                    QMessageBox::warning(this, tr("加载失败"),
                                         tr("无法加载：%1").arg(path));
                    return;
                }
                showPage(PageMapLabGuide);
            });



}


void MainWindow::showPage(MainWindow::PageIndex index)
{
    auto *stack = ui->stackedWidget;
    if (!stack) return;

    if (index < 0 || index >= stack->count()) return;

    stack->setCurrentIndex(index);

    // 状态栏提示集中管理
    switch (index) {
    case PageLogin:
        statusBar()->showMessage(tr("请登录"));
        break;
    case PageOperateSelect:
        statusBar()->showMessage(tr("当前用户：%1").arg(m_currentUser));
        break;
    case PageMapLabGuide:
        statusBar()->showMessage(tr("地图导入与标注"));
        break;
    case PageMapSelect:
        statusBar()->showMessage(tr("地图选择与导航"));
        break;
    }
}

// ---------------- slots ----------------

void MainWindow::handleLoginSuccess(const QString &username)
{
    m_currentUser = username;
    showPage(PageOperateSelect);
}

void MainWindow::handleLogoutRequested()
{
    m_currentUser.clear();
    showPage(PageLogin);
}

void MainWindow::handleOpenMapImport()
{
    showPage(PageMapLabGuide);
}

void MainWindow::handleOpenMapSelect()
{
    showPage(PageMapSelect);
}

void MainWindow::handleExportDataRequested()
{
    QMessageBox::information(this, tr("提示"), tr("导出功能尚未实现。"));
}

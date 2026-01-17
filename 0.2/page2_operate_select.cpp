#include "page2_operate_select.h"
#include "ui_page2_operate_select.h"

#include <QMessageBox>

#include <QFileDialog>  // 用于弹出保存框
#include <QFile>        // 用于创建文件
#include <QTextStream>  // 用于写入文本
#include <QDateTime>    // 用于生成文件名的时间戳
#include <QDebug>
page2_operate_select::page2_operate_select(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::page2_operate_select)
{
    ui->setupUi(this);

    // 连接按钮到槽函数
    connect(ui->page2_exit_pb,   &QPushButton::clicked,
            this, &page2_operate_select::onExitClicked);

    connect(ui->page2_export_pb, &QPushButton::clicked,
            this, &page2_operate_select::onExportClicked);

    connect(ui->page2_import_pb, &QPushButton::clicked,
            this, &page2_operate_select::onImportClicked);

    connect(ui->page2_select_pb, &QPushButton::clicked,
            this, &page2_operate_select::onSelectClicked);
}

page2_operate_select::~page2_operate_select()
{
    delete ui;
}

void page2_operate_select::onExitClicked()
{
    // 返回登录页
    emit backToLogin();
}

// void page2_operate_select::onExportClicked()
// {
//     // 发信号，由 MainWindow 决定如何处理
//     emit exportDataRequested();

//     // 也可以在这里给个简单提示
//     // QMessageBox::information(this, tr("提示"), tr("导出功能尚未实现"));
// }

void page2_operate_select::onExportClicked()
{
    // 不在这里做导出，而是切换到地图页去做
    emit requestExportMap();

}

void page2_operate_select::onImportClicked()
{
    // 进入地图导入/标注界面（Index 2）
    emit openMapImport();
}

void page2_operate_select::onSelectClicked()
{
    // 进入地图选择界面（Index 3）
    emit openMapSelect();
}

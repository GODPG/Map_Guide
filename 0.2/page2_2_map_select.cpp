#include "page2_2_map_select.h"
//#include "ui_map_select.h"
// page2_2_map_select.cpp
#include "ui_page2_2_map_select.h" //  C++ 类名
#include "map_card_widget.h"

#include <QListWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDateTime>

page2_2_map_select::page2_2_map_select(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::map_select)
{
    ui->setupUi(this);
    initUi();
    reloadProjectList();
}

page2_2_map_select::~page2_2_map_select()
{
    delete ui;
}

void page2_2_map_select::initUi()
{
    // 给按钮改个更像样的文字（你 ui 里还是 PushButton）
    ui->pbBack->setText(tr("返回"));
    ui->pbImport->setText(tr("导入新地图"));
    ui->pbRefresh->setText(tr("刷新"));

    // listWidget 的视觉与交互设置
    ui->listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidget->setSpacing(8);

    connect(ui->pbBack,    &QPushButton::clicked, this, &page2_2_map_select::onBackClicked);
    connect(ui->pbImport,  &QPushButton::clicked, this, &page2_2_map_select::onImportClicked);
    connect(ui->pbRefresh, &QPushButton::clicked, this, &page2_2_map_select::onRefreshClicked);

    connect(ui->listWidget, &QListWidget::itemDoubleClicked,
            this, &page2_2_map_select::onItemDoubleClicked);
}

QString page2_2_map_select::mapGuideRootDir() const
{
    // 默认把 Map_guide 放在程序目录旁边
    // 例：MyApp.exe 同级的 Map_guide/
    const QString appDir = QCoreApplication::applicationDirPath();
    return appDir + "/Map_guide";
}

int page2_2_map_select::parseSavedMapId(const QString &folderName) const
{
    // 期望格式：Saved_Map#123
    const QString prefix = "Saved_Map#";
    if (!folderName.startsWith(prefix)) return -1;

    bool ok = false;
    int id = folderName.mid(prefix.size()).toInt(&ok);
    return ok ? id : -1;
}

QString page2_2_map_select::findPreviewImage(const QString &projectRoot) const
{
    // 优先 map_view.png，其次 map_image.png
    const QString p1 = projectRoot + "/map_view.png";
    if (QFileInfo::exists(p1)) return p1;

    const QString p2 = projectRoot + "/map_image.png";
    if (QFileInfo::exists(p2)) return p2;

    // 你也可能存成 jpg
    const QString p3 = projectRoot + "/map_image.jpg";
    if (QFileInfo::exists(p3)) return p3;

    return QString();
}

void page2_2_map_select::reloadProjectList()
{
    ui->listWidget->clear();

    const QString rootPath = mapGuideRootDir();
    QDir root(rootPath);

    if (!root.exists()) {
        // 没有目录就创建，避免用户第一次打开是空
        root.mkpath(".");
    }

    // 只列出子目录
    const QFileInfoList dirs = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    // 为了简单：按名字排序（Saved_Map#1, Saved_Map#2 ... 如果想数值排序再加一段排序代码）
    for (const QFileInfo &fi : dirs) {
        const QString folderName = fi.fileName();
        const int mapId = parseSavedMapId(folderName);
        if (mapId < 0) continue; // 非项目目录跳过

        const QString projectRoot = fi.absoluteFilePath();

        // 简单合法性检查：有没有 guide_data/graph.json
        const QString graphJson = projectRoot + "/guide_data/graph.json";
        const bool okProject = QFileInfo::exists(graphJson);

        // 卡片显示内容
        const QString title = folderName;

        QString sub;
        sub += okProject ? tr("状态：可加载\n") : tr("状态：存档损坏/缺少 graph.json\n");

        // 显示最后修改时间（用目录的 lastModified）
        const QDateTime t = fi.lastModified();
        sub += tr("更新时间：%1\n").arg(t.toString("yyyy-MM-dd HH:mm"));

        // 存项目路径（显示短一点）
        sub += tr("路径：%1").arg(projectRoot);

        // 创建 QListWidgetItem
        auto *item = new QListWidgetItem(ui->listWidget);
        item->setSizeHint(QSize(0, 92));
        item->setData(Qt::UserRole, projectRoot);

        // 若损坏项目：禁用双击（用户看得到但无法加载）
        if (!okProject) {
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        }

        // 创建卡片 widget
        auto *card = new MapCardWidget;
        const QString preview = findPreviewImage(projectRoot);
        card->setInfo(title, sub, preview); // preview 为空会走默认 Icon/空白

        ui->listWidget->addItem(item);
        ui->listWidget->setItemWidget(item, card);
    }

    // 如果没找到任何项目，给个提示项（可选）
    if (ui->listWidget->count() == 0) {
        auto *item = new QListWidgetItem(ui->listWidget);
        item->setSizeHint(QSize(0, 72));
        item->setFlags(Qt::NoItemFlags);

        auto *card = new MapCardWidget;
        card->setInfo(tr("暂无地图存档"),
                      tr("点击“导入新地图”创建项目。\n"
                         "存档目录：%1").arg(rootPath),
                      QString());

        ui->listWidget->addItem(item);
        ui->listWidget->setItemWidget(item, card);
    }
}

void page2_2_map_select::onBackClicked()
{
    emit backToMenu();
}

void page2_2_map_select::onImportClicked()
{
    const QString file = QFileDialog::getOpenFileName(
        this,
        tr("选择地图图片"),
        QString(),
        tr("Images (*.png *.jpg *.jpeg *.bmp)")
        );

    if (file.isEmpty()) return;

    // 发送给 MainWindow：新建项目
    emit requestOpenProject(file, true);
}

void page2_2_map_select::onRefreshClicked()
{
    reloadProjectList();
}

void page2_2_map_select::onItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;

    // 若 item 不可用（损坏项目），直接忽略
    if (!(item->flags() & Qt::ItemIsEnabled)) return;

    const QString projectRoot = item->data(Qt::UserRole).toString();
    if (projectRoot.isEmpty()) return;

    // 发送给 MainWindow：加载旧项目
    emit requestOpenProject(projectRoot, false);
}

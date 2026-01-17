// streetviewdialog.cpp
#include "streetviewdialog.h"

// 必须包含 UI 自动生成的头文件
#include "ui_streetviewdialog.h"

// 图像处理和调试所需的 Qt 类
#include <QPixmap>
#include <QDebug>
#include <QLabel> // 虽然在 .h 里没有显式使用，但 .cpp 里需要用到 QLabel 对应的组件

/**
 * 构造函数
 * 初始化 UI，接收并存储图片路径，并显示第一张图片。
 */
StreetViewDialog::StreetViewDialog(const QVector<QString> &paths, QWidget *parent)
    // 1. 初始化基类
    : QDialog(parent),
    // 2. 实例化 UI 对象 (QDialog 标准做法)
    ui(new Ui::StreetViewDialog),
    // 3. 存储传入的图片路径
    m_allPaths(paths)
{
    ui->setupUi(this);
    this->setWindowTitle("路景查看");

    // 连接按钮的信号到槽函数
    connect(ui->pbPrevious, &QPushButton::clicked, this, &StreetViewDialog::on_pbPrevious_clicked);
    connect(ui->pbNext, &QPushButton::clicked, this, &StreetViewDialog::on_pbNext_clicked);

    // 初始检查和加载
    if (m_allPaths.isEmpty()) {
        ui->labelImage->setText("该节点没有存储路景图片。");
        ui->labelProgress->setText("");
    } else {
        m_currentIndex = 0; // 确保从第一张开始
        updateImageAndProgress();
    }
}

/**
 * 析构函数
 * 释放 UI 内存。
 */
StreetViewDialog::~StreetViewDialog()
{
    delete ui;
}

/**
 * 核心功能：加载当前索引的图片并更新进度显示。
 */
void StreetViewDialog::updateImageAndProgress()
{
    if (m_allPaths.isEmpty()) {
        // 如果路径列表为空，直接返回，避免越界
        ui->labelImage->setText("该节点没有路景图片。");
        return;
    }

    // 1. 加载图片
    QString currentPath = m_allPaths.at(m_currentIndex);
    QPixmap pixmap;

    // 尝试从绝对路径加载图片
    if (pixmap.load(currentPath)) {
        // 假设您的 UI 中图片显示的 QLabel 命名为 labelImage
        // 并且该 QLabel 的 scaledContents 属性已设置为 true
        ui->labelImage->setPixmap(pixmap);
        ui->labelImage->setAlignment(Qt::AlignCenter); // 居中显示
    } else {
        ui->labelImage->setText(QString("无法加载图片 [%1/%2]:\n%3")
                                    .arg(m_currentIndex + 1)
                                    .arg(m_allPaths.size())
                                    .arg(currentPath));
        qDebug() << "Error: Failed to load image at path:" << currentPath;
    }

    // 2. 更新进度显示 (例如: 1 / 5)
    QString progress = QString("%1 / %2").arg(m_currentIndex + 1).arg(m_allPaths.size());
    // 假设您的 UI 中进度显示的 QLabel 命名为 labelProgress
    ui->labelProgress->setText(progress);

    // 3. 更新导航按钮状态
    toggleNavigationButtons();
}

/**
 * 根据当前索引，启用或禁用“上一张”和“下一张”按钮。
 */
void StreetViewDialog::toggleNavigationButtons()
{
    if (m_allPaths.size() <= 1) {
        // 只有一张图片或没有图片时，都禁用按钮
        ui->pbPrevious->setEnabled(false);
        ui->pbNext->setEnabled(false);
        return;
    }

    // 假设 UI 中按钮命名为 pbPrevious 和 pbNext

    // 如果是第一张图片 (索引 0)，禁用“上一张”
    ui->pbPrevious->setEnabled(m_currentIndex > 0);

    // 如果是最后一张图片，禁用“下一张”
    ui->pbNext->setEnabled(m_currentIndex < m_allPaths.size() - 1);
}

/**
 * 槽函数：点击“下一张”按钮
 */
void StreetViewDialog::on_pbNext_clicked()
{
    if (m_currentIndex < m_allPaths.size() - 1) {
        m_currentIndex++;
        updateImageAndProgress();
    }
}

/**
 * 槽函数：点击“上一张”按钮
 */
void StreetViewDialog::on_pbPrevious_clicked()
{
    if (m_currentIndex > 0) {
        m_currentIndex--;
        updateImageAndProgress();
    }
}

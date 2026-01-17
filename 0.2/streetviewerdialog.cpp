#include "streetviewerdialog.h"

#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QDesktopServices>
#include <QUrl>

StreetViewerDialog::StreetViewerDialog(const QStringList &imagePaths, int startIndex, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("路景查看");
    setModal(true);
    setMinimumSize(640, 420);

    buildUi();
    setImages(imagePaths, startIndex);
}

void StreetViewerDialog::buildUi()
{
    // 图片显示区
    m_imageLabel = new QLabel(this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setText("无图片");
    m_imageLabel->setStyleSheet("QLabel { background: #111; color: #DDD; }");
    m_imageLabel->setMinimumHeight(240);

    // 信息栏：第N/总数 + 文件名
    m_infoLabel = new QLabel(this);
    m_infoLabel->setAlignment(Qt::AlignLeft);
    m_infoLabel->setText("0/0");

    // 控制按钮
    m_prevBtn = new QPushButton("上一张", this);
    m_nextBtn = new QPushButton("下一张", this);
    m_closeBtn = new QPushButton("关闭", this);

    m_openBtn = new QToolButton(this);
    m_openBtn->setText("外部打开");

    connect(m_prevBtn, &QPushButton::clicked, this, &StreetViewerDialog::onPrev);
    connect(m_nextBtn, &QPushButton::clicked, this, &StreetViewerDialog::onNext);
    connect(m_closeBtn, &QPushButton::clicked, this, &StreetViewerDialog::reject);
    connect(m_openBtn, &QToolButton::clicked, this, &StreetViewerDialog::onOpenExternal);

    auto *btnRow = new QHBoxLayout();
    btnRow->addWidget(m_infoLabel, 1);
    btnRow->addWidget(m_openBtn);
    btnRow->addSpacing(12);
    btnRow->addWidget(m_prevBtn);
    btnRow->addWidget(m_nextBtn);
    btnRow->addWidget(m_closeBtn);

    auto *root = new QVBoxLayout(this);
    root->addWidget(m_imageLabel, 1);
    root->addLayout(btnRow);

    setLayout(root);
}

void StreetViewerDialog::setImages(const QStringList &imagePaths, int startIndex)
{
    m_paths = imagePaths;

    if (m_paths.isEmpty()) {
        m_index = 0;
        m_originalPixmap = QPixmap();
        m_imageLabel->setPixmap(QPixmap());
        m_imageLabel->setText("该节点没有路景图片");
        updateButtons();
        return;
    }

    if (startIndex < 0) startIndex = 0;
    if (startIndex >= m_paths.size()) startIndex = m_paths.size() - 1;
    m_index = startIndex;

    loadCurrent();
}

void StreetViewerDialog::loadCurrent()
{
    if (m_paths.isEmpty()) {
        m_imageLabel->setPixmap(QPixmap());
        m_imageLabel->setText("该节点没有路景图片");
        updateButtons();
        return;
    }

    const QString path = m_paths[m_index];
    QFileInfo fi(path);

    if (!fi.exists()) {
        m_originalPixmap = QPixmap();
        m_imageLabel->setPixmap(QPixmap());
        m_imageLabel->setText(QString("图片不存在：\n%1").arg(path));
        updateButtons();
        return;
    }

    QPixmap pix(path);
    if (pix.isNull()) {
        m_originalPixmap = QPixmap();
        m_imageLabel->setPixmap(QPixmap());
        m_imageLabel->setText(QString("无法读取图片：\n%1").arg(path));
        updateButtons();
        return;
    }

    m_originalPixmap = pix;
    updateScaledPixmap();
    updateButtons();
}

void StreetViewerDialog::updateScaledPixmap()
{
    if (m_originalPixmap.isNull()) return;

    // 以 label 可用区域为基准等比缩放
    QSize target = m_imageLabel->size();
    if (target.width() < 10 || target.height() < 10) return;

    QPixmap scaled = m_originalPixmap.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_imageLabel->setPixmap(scaled);
    m_imageLabel->setText(QString()); // 清掉文字
}

void StreetViewerDialog::updateButtons()
{
    const int n = m_paths.size();

    // 标题/信息
    if (n <= 0) {
        m_infoLabel->setText("0/0");
        setWindowTitle("路景查看");
    } else {
        QFileInfo fi(m_paths[m_index]);
        m_infoLabel->setText(QString("%1 / %2    %3")
                                 .arg(m_index + 1)
                                 .arg(n)
                                 .arg(fi.fileName()));
        setWindowTitle(QString("路景查看 (%1/%2)").arg(m_index + 1).arg(n));
    }

    // 按钮可用性
    const bool has = (n > 0);
    m_prevBtn->setEnabled(has && n > 1);
    m_nextBtn->setEnabled(has && n > 1);
    m_openBtn->setEnabled(has);
}

void StreetViewerDialog::onPrev()
{
    if (m_paths.size() <= 1) return;
    m_index = (m_index - 1 + m_paths.size()) % m_paths.size();
    loadCurrent();
}

void StreetViewerDialog::onNext()
{
    if (m_paths.size() <= 1) return;
    m_index = (m_index + 1) % m_paths.size();
    loadCurrent();
}

void StreetViewerDialog::onOpenExternal()
{
    if (m_paths.isEmpty()) return;
    const QString path = m_paths[m_index];
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void StreetViewerDialog::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_A:
        onPrev();
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        onNext();
        break;
    case Qt::Key_Escape:
        reject();
        break;
    default:
        QDialog::keyPressEvent(event);
        break;
    }
}

void StreetViewerDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    updateScaledPixmap();
}

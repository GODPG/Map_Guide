#include "map_card_widget.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QCoreApplication>
#include <QStyle>

MapCardWidget::MapCardWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("MapCardWidget");
    setAttribute(Qt::WA_Hover, true);

    buildUi();
    applyStyle();

    setThumbnail(QString());
}

void MapCardWidget::setInfo(const QString &title,
                            const QString &subtitle,
                            const QString &thumbnailPath)
{
    setTitle(title);
    setSubtitle(subtitle);
    setThumbnail(thumbnailPath);
}

void MapCardWidget::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void MapCardWidget::setSubtitle(const QString &subtitle)
{
    m_subLabel->setText(subtitle);
}

void MapCardWidget::setDefaultIconDir(const QString &dirPath)
{
    m_iconDirOverride = dirPath;
    setThumbnail(QString());
}

void MapCardWidget::setDefaultIconFileName(const QString &fileName)
{
    m_defaultIconFile = fileName;
    setThumbnail(QString());
}

void MapCardWidget::setBlankIconFileName(const QString &fileName)
{
    m_blankIconFile = fileName;
    setThumbnail(QString());
}

void MapCardWidget::setThumbnail(const QString &thumbnailPath)
{
    QPixmap pix;

    if (!thumbnailPath.isEmpty()) {
        pix = loadPixmapOrBlank(thumbnailPath);
    } else {
        // “初始为空”时，优先 blank；你也可以改成优先 default
        const QString blankPath = resolveBlankIconPath();
        if (!blankPath.isEmpty()) {
            pix = loadPixmapOrBlank(blankPath);
        } else {
            const QString defPath = resolveDefaultIconPath();
            if (!defPath.isEmpty()) pix = loadPixmapOrBlank(defPath);
        }
    }

    if (pix.isNull()) {
        // 最终兜底：透明空白
        QPixmap empty(m_thumbSize, m_thumbSize);
        empty.fill(Qt::transparent);
        pix = empty;
    }

    // 等比缩放并居中显示
    QPixmap scaled = pix.scaled(m_thumbSize, m_thumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_thumbLabel->setPixmap(scaled);
}

void MapCardWidget::setSelected(bool on)
{
    if (m_selected == on) return;
    m_selected = on;

    // 直接用属性驱动样式（更干净）
    setProperty("selected", m_selected);
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void MapCardWidget::buildUi()
{
    // 缩略图
    m_thumbLabel = new QLabel(this);
    m_thumbLabel->setFixedSize(m_thumbSize, m_thumbSize);
    m_thumbLabel->setAlignment(Qt::AlignCenter);
    m_thumbLabel->setObjectName("thumbLabel");

    // 标题
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("titleLabel");
    m_titleLabel->setText("Untitled");

    // 副标题（多行）
    m_subLabel = new QLabel(this);
    m_subLabel->setObjectName("subLabel");
    m_subLabel->setText("");
    m_subLabel->setWordWrap(true);

    auto *right = new QVBoxLayout();
    right->setContentsMargins(0, 0, 0, 0);
    right->setSpacing(4);
    right->addWidget(m_titleLabel);
    right->addWidget(m_subLabel, 1);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(10);
    root->addWidget(m_thumbLabel);
    root->addLayout(right, 1);

    setLayout(root);

    setMinimumHeight(115);
}

void MapCardWidget::applyStyle()
{
    setStyleSheet(R"(
        /* ----------------------- 1. 主卡片容器样式 ----------------------- */
        QWidget#MapCardWidget {
            /* 基础外观：纯白卡片，带浅灰边框 */
            border: 1px solid #E2E8F0;
            border-radius: 12px;
            background-color: #FFFFFF;
        }

        /* 鼠标悬停 (Hover) 状态：边框变蓝，背景微蓝 */
        QWidget#MapCardWidget:hover {
            border: 1px solid #4A90E2;
            background-color: #F7FAFC;
        }

        /* 选中 (Selected) 状态：粗蓝边框，高亮背景 */
        QWidget#MapCardWidget[selected="true"] {
            border: 2px solid #4A90E2;
            background-color: #EBF8FF;
            padding: -1px; /* 补偿边框加粗造成的位移 */
        }

        /* ----------------------- 2. 文本样式 (关键修改：由白改黑) ----------------------- */

        /* 标题 (Title): 深灰色 (#2D3748), 加粗, 14px */
        QLabel#titleLabel {
            font-size: 14px;
            font-weight: bold;
            font-family: "Microsoft YaHei", sans-serif;
            color: #2D3748;   /* <--- 这里改成了深色，以前是 #FFFFFF */
            qproperty-text-overrun: elideRight;
        }

        /* 副标题 (Subtitle): 浅灰色 (#718096), 12px */
        QLabel#subLabel {
            font-size: 12px;
            font-family: "Microsoft YaHei", sans-serif;
            color: #718096;   /* <--- 这里改成了灰色，以前是 #FFFFFF */
            qproperty-text-overrun: elideRight;
        }

        /* ----------------------- 3. 缩略图样式 ----------------------- */
        QLabel#thumbLabel {
            border: 1px solid #EDF2F7;
            border-radius: 8px;
            background-color: #F7FAFC; /* 缩略图背景也调亮一点 */
            margin: 2px;
        }
    )");
}

QPixmap MapCardWidget::loadPixmapOrBlank(const QString &path) const
{
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile()) return QPixmap();

    QPixmap pix(path);
    return pix;
}

QString MapCardWidget::defaultIconDir() const
{
    if (!m_iconDirOverride.isEmpty())
        return m_iconDirOverride;

    const QString appDir = QCoreApplication::applicationDirPath();
    return appDir + "/Map_guide/Icon";
}

QString MapCardWidget::resolveDefaultIconPath() const
{
    const QString dir = defaultIconDir();
    const QString p = dir + "/" + m_defaultIconFile;
    QFileInfo fi(p);
    return (fi.exists() && fi.isFile()) ? fi.absoluteFilePath() : QString();
}

QString MapCardWidget::resolveBlankIconPath() const
{
    const QString dir = defaultIconDir();
    const QString p = dir + "/" + m_blankIconFile;
    QFileInfo fi(p);
    return (fi.exists() && fi.isFile()) ? fi.absoluteFilePath() : QString();
}

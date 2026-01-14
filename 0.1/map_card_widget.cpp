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

    // 初始为空：显示 blank（或透明空白）
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
    // 重新刷新缩略图（用默认 icon）
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
    // 优先用传入路径；为空则用默认 icon；默认 icon 不存在则用 blank；blank 也不存在则透明空白
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

    // 给卡片一个较舒服的高度（你也可以在 QListWidgetItem 的 sizeHint 控制）
    setMinimumHeight(115);
}

void MapCardWidget::applyStyle()
{

    setStyleSheet(R"(
        /* ----------------------- 1. 主卡片容器样式 ----------------------- */
        QWidget#MapCardWidget {
            /* 基础外观 */
            border: 1px solid rgba(0, 0, 0, 0.08); /* 极细且柔和的边框 */
            border-radius: 8px;                  /* 略大的圆角 */
            background-color: #FFFFFF;            /* 纯白色背景 */

            /* 柔和的阴影：增加悬浮感 */
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05);
        }

        /* 鼠标悬停 (Hover) 状态 */
        QWidget#MapCardWidget:hover {
            border: 1px solid #4CAF50; /* 使用柔和的绿色边框 */
            background-color: #F8FFF8; /* 略微发绿的背景，增强反馈 */
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); /* 阴影加深，模拟“抬升” */
        }

        /* 选中 (Selected) 状态 */
        QWidget#MapCardWidget[selected="true"] {
            border: 2px solid #1E90FF; /* 突出显示：使用 Qt 蓝边框，加粗 */
            background-color: #F0F8FF; /* 略微发蓝的背景 */
            box-shadow: 0 4px 10px rgba(0, 0, 0, 0.15); /* 突出阴影 */
            padding: -1px; /* 补偿边框增厚带来的布局偏移 */
        }

        /* ----------------------- 2. 文本样式 (重点修改部分) ----------------------- */

        /* 标题 (Title): 纯白, 加粗, 稍微变小 (13px), 保证不超框 */
        QLabel#titleLabel {
            font-size: 13px;          /* 稍微变小 */
            font-weight: 700;         /* 加粗 */
            color: #FFFFFF;           /* 纯白色 */
            /* 文本过长时省略（防止超框）*/
            qproperty-text-overrun: elideRight;
        }

        /* 副标题 (Subtitle/Info): 纯白, 稍微变小 (11px) */
        QLabel#subLabel {
            font-size: 10px;
            font-weight: 500;
            color: #FFFFFF;           /* 纯白色 */
            /* 文本过长时省略或换行（如果 space 允许）*/
            qproperty-text-overrun: elideRight;
        }

        /* ----------------------- 3. 缩略图样式 ----------------------- */

        QLabel#thumbLabel {
            border: none;             /* 移除边框，使用卡片本身的边框 */
            border-radius: 8px;       /* 保持缩略图圆角 */
            background-color: #EEEEEE;/* 浅灰色占位符背景 */
            /* 稍微内缩，避免紧贴卡片边缘 */
            margin: 1px;
        }
    )");
}

QPixmap MapCardWidget::loadPixmapOrBlank(const QString &path) const
{
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile()) return QPixmap();

    QPixmap pix(path);
    return pix; // 可能是 null（读取失败），上层会兜底
}

QString MapCardWidget::defaultIconDir() const
{
    if (!m_iconDirOverride.isEmpty())
        return m_iconDirOverride;

    // 默认：<appDir>/Map_guide/Icon
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

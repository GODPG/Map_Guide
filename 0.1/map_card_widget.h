#ifndef MAP_CARD_WIDGET_H
#define MAP_CARD_WIDGET_H

#include <QWidget>
#include <QString>
#include <QPixmap>

class QLabel;

class MapCardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MapCardWidget(QWidget *parent = nullptr);

    // 一次性设置
    void setInfo(const QString &title,
                 const QString &subtitle,
                 const QString &thumbnailPath = QString());

    // 分开设置
    void setTitle(const QString &title);
    void setSubtitle(const QString &subtitle);

    // 设置缩略图（支持传入具体图片路径；空字符串则用默认 Icon）
    void setThumbnail(const QString &thumbnailPath);

    // 列表选中时，你可以调用它改变外观（可选）
    void setSelected(bool on);

    // 可选：设置默认 Icon 目录（默认：<appDir>/Map_guide/Icon）
    void setDefaultIconDir(const QString &dirPath);

    // 可选：设置默认 Icon 文件名（默认 default.png / blank.png）
    void setDefaultIconFileName(const QString &fileName);
    void setBlankIconFileName(const QString &fileName);

private:
    void buildUi();
    void applyStyle();

    QPixmap loadPixmapOrBlank(const QString &path) const;
    QString defaultIconDir() const;
    QString resolveDefaultIconPath() const;
    QString resolveBlankIconPath() const;

private:
    QLabel *m_thumbLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_subLabel   = nullptr;

    bool m_selected = false;

    QString m_iconDirOverride;          // 可选覆盖
    QString m_defaultIconFile = "default.png";
    QString m_blankIconFile   = "blank.png";

    int m_thumbSize = 93;               // 缩略图区域边长
};

#endif // MAP_CARD_WIDGET_H

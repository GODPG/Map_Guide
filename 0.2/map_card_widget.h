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


    void setInfo(const QString &title,
                 const QString &subtitle,
                 const QString &thumbnailPath = QString());


    void setTitle(const QString &title);
    void setSubtitle(const QString &subtitle);


    void setThumbnail(const QString &thumbnailPath);


    void setSelected(bool on);


    void setDefaultIconDir(const QString &dirPath);


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

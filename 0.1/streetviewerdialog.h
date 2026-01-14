#ifndef STREET_VIEWER_DIALOG_H
#define STREET_VIEWER_DIALOG_H

#include <QDialog>
#include <QStringList>
#include <QPixmap>

class QLabel;
class QPushButton;
class QToolButton;

class StreetViewerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit StreetViewerDialog(const QStringList &imagePaths,
                                int startIndex = 0,
                                QWidget *parent = nullptr);

    void setImages(const QStringList &imagePaths, int startIndex = 0);

    int currentIndex() const { return m_index; }
    int imageCount()   const { return m_paths.size(); }

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onPrev();
    void onNext();
    void onOpenExternal();

private:
    void buildUi();
    void loadCurrent();       // 加载当前图片（或显示提示）
    void updateButtons();     // 按钮状态、标题、计数文字
    void updateScaledPixmap();// 根据当前窗口大小缩放显示

private:
    QStringList m_paths;
    int m_index = 0;

    // 缓存当前原始图片，缩放时用
    QPixmap m_originalPixmap;

    // UI
    QLabel *m_imageLabel = nullptr;
    QLabel *m_infoLabel  = nullptr;

    QPushButton *m_prevBtn = nullptr;
    QPushButton *m_nextBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QToolButton *m_openBtn = nullptr;
};

#endif // STREET_VIEWER_DIALOG_H

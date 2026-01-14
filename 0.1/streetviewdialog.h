// streetviewdialog.h
#ifndef STREETVIEWDIALOG_H
#define STREETVIEWDIALOG_H

#include <QDialog>
#include <QVector>
#include <QString>

namespace Ui {
class StreetViewDialog;
}

class StreetViewDialog : public QDialog
{
    Q_OBJECT

public:
    // 构造函数：接收图片路径列表
    explicit StreetViewDialog(const QVector<QString> &paths, QWidget *parent = nullptr);
    ~StreetViewDialog();

private slots:
    void on_pbPrevious_clicked();
    void on_pbNext_clicked();

private:
    Ui::StreetViewDialog *ui;

    QVector<QString> m_allPaths; // 所有图片的绝对路径
    int m_currentIndex = 0;      // 当前显示的图片索引

    void updateImageAndProgress(); // 加载图片并更新进度条
    void toggleNavigationButtons(); // 根据图片数量和索引启用/禁用按钮
};

#endif // STREETVIEWDIALOG_H

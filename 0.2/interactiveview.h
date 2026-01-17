#ifndef INTERACTIVEVIEW_H
#define INTERACTIVEVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>

class InteractiveView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit InteractiveView(QWidget *parent = nullptr);

protected:
    // 滚轮缩放
    void wheelEvent(QWheelEvent *event) override;

    // 鼠标交互
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    // 【新增】键盘交互：监听空格键状态
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    bool m_isPanning = false;      // 是否正在拖动
    bool m_isSpacePressed = false; // 空格键是否被按住
    QPoint m_lastPanPos;           // 上一次鼠标位置
};

#endif // INTERACTIVEVIEW_H

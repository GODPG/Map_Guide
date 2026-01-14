#ifndef INTERACTIVEVIEW_H
#define INTERACTIVEVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>
#include <QMouseEvent>

class InteractiveView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit InteractiveView(QWidget *parent = nullptr);

protected:
    // 滚轮缩放
    void wheelEvent(QWheelEvent *event) override;

    // 鼠标交互：中键漫游，左/右键透传
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_isPanning = false; // 是否正在漫游
    QPoint m_lastPanPos;      // 上一次鼠标位置
};

#endif // INTERACTIVEVIEW_H

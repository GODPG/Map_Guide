#include "interactiveview.h"
#include <QScrollBar>

InteractiveView::InteractiveView(QWidget *parent) : QGraphicsView(parent)
{
    // 1. 设置缩放中心：缩放时以鼠标当前位置为锚点，而不是左上角
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    // 2. 设置调整大小锚点：保持视图中心
    setResizeAnchor(QGraphicsView::AnchorViewCenter);

    // 3. 开启抗锯齿，让线条在缩放后依然平滑
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform); // 图片缩放平滑

    // 4. (可选) 隐藏滚动条，像地图软件一样全屏漫游
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

// --- 滚轮缩放逻辑 ---
void InteractiveView::wheelEvent(QWheelEvent *event)
{
    // 获取滚轮滚动的角度
    int angle = event->angleDelta().y();
    if (angle == 0) return;

    // 缩放系数：每次滚动放大/缩小 15%
    qreal scaleFactor = 1.15;

    if (angle > 0) {
        // 向上滚：放大
        scale(scaleFactor, scaleFactor);
    } else {
        // 向下滚：缩小
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
}

// --- 鼠标按下事件 ---
void InteractiveView::mousePressEvent(QMouseEvent *event)
{
    // 只有按下“中键”才触发漫游
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor); // 光标变为“抓手”
        event->accept(); // 吃掉事件，不传给 Scene
    } else {
        // 【关键】左键/右键必须传给基类，否则您的添加节点/连线功能会失效！
        QGraphicsView::mousePressEvent(event);
    }
}

// --- 鼠标移动事件 ---
void InteractiveView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        // 计算鼠标移动的距离差
        QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();

        // 反向调整滚动条，实现“拖拽地图”的效果
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());

        event->accept();
    } else {
        // 【关键】非漫游状态传给基类，保证您的“长按连线”功能正常
        QGraphicsView::mouseMoveEvent(event);
    }
}

// --- 鼠标释放事件 ---
void InteractiveView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor); // 恢复光标
        event->accept();
    } else {
        // 【关键】传给基类
        QGraphicsView::mouseReleaseEvent(event);
    }
}

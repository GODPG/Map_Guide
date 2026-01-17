#include "interactiveview.h"
#include <QScrollBar>
#include <QApplication>

InteractiveView::InteractiveView(QWidget *parent) : QGraphicsView(parent)
{
    // 1. 设置缩放锚点
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);

    // 2. 开启抗锯齿
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);

    // 3. 隐藏滚动条
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 4. 关闭 Qt 自带拖拽，完全接管
    setDragMode(QGraphicsView::NoDrag);

    // 5. 开启鼠标追踪
    setMouseTracking(true);

    // 6. 设置强焦点策略，否则接收不到键盘事件(空格键)
    setFocusPolicy(Qt::StrongFocus);
}


void InteractiveView::wheelEvent(QWheelEvent *event)
{
    bool isCtrlPressed = (event->modifiers() & Qt::ControlModifier);

    if (isCtrlPressed) {
        int angle = event->angleDelta().y();
        if (angle == 0) return;

        qreal scaleFactor = 1.15;
        if (angle > 0) {
            scale(scaleFactor, scaleFactor);
        } else {
            scale(1.0 / scaleFactor, 1.0 / scaleFactor);
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

// --- 【新增】键盘按下事件：按下空格变“抓手” ---
void InteractiveView::keyPressEvent(QKeyEvent *event)
{
    // 如果还没在拖动，且按下了空格 -> 进入准备拖动状态
    if (!event->isAutoRepeat() && event->key() == Qt::Key_Space) {
        m_isSpacePressed = true;
        // 视觉反馈：光标变手型
        setCursor(Qt::OpenHandCursor);
        event->accept(); // 吃掉事件，不让空格触发按钮点击等其他行为
    } else {
        QGraphicsView::keyPressEvent(event);
    }
}

// --- 【新增】键盘松开事件：松开空格恢复光标 ---
void InteractiveView::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat() && event->key() == Qt::Key_Space) {
        m_isSpacePressed = false;

        // 如果此刻没有在按鼠标拖动，就恢复普通光标
        if (!m_isPanning) {
            setCursor(Qt::ArrowCursor);
        }
        event->accept();
    } else {
        QGraphicsView::keyReleaseEvent(event);
    }
}

// --- 鼠标按下 ---
void InteractiveView::mousePressEvent(QMouseEvent *event)
{
    // 触发拖动的条件：
    // 1. 中键按下
    // 2. 或者：左键按下 且 空格键正被按住 (m_isSpacePressed)
    bool isMiddleClick = (event->button() == Qt::MiddleButton);
    bool isSpaceDrag   = (event->button() == Qt::LeftButton && m_isSpacePressed);

    if (isMiddleClick || isSpaceDrag) {
        m_isPanning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor); // 变为“握紧的手”
        event->accept();
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

// --- 鼠标移动 ---
void InteractiveView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();

        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());

        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

// --- 鼠标释放 ---
void InteractiveView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        m_isPanning = false;

        // 恢复光标逻辑：
        // 如果空格还按着，恢复成“张开的手”(准备下一次拖动)
        // 否则恢复成“箭头”
        if (m_isSpacePressed) {
            setCursor(Qt::OpenHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }

        event->accept();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

#include "page2_1_map_lab_guide.h"
#include "ui_page2_1_map_lab_guide.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QPen>
#include <QBrush>

#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>
#include <QDateTime>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <queue>
#include <limits>
#include <cmath>

#include <QGraphicsSceneMouseEvent>
#include <QTransform>
#include <algorithm>

#include <QPainterPath>
#include <QDebug>

#include "streetviewerdialog.h"

class MapScene : public QGraphicsScene
{
public:
    using QGraphicsScene::QGraphicsScene;

    std::function<void(const QPointF&)> onLeftPress;
    std::function<void(const QPointF&)> onLeftMove;
    std::function<void(const QPointF&)> onLeftRelease;
    std::function<void(const QPointF&)> onRightClick;

    // hover move：不要求按住左键
    std::function<void(const QPointF&)> onHoverMove;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            if (onLeftPress) onLeftPress(event->scenePos());
        } else if (event->button() == Qt::RightButton) {
            if (onRightClick) onRightClick(event->scenePos());
        }
        QGraphicsScene::mousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        // hover 更新
        if (onHoverMove) onHoverMove(event->scenePos());

        // leftMove 仍保留（画边拖动）
        if (onLeftMove) onLeftMove(event->scenePos());

        QGraphicsScene::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            if (onLeftRelease) onLeftRelease(event->scenePos());
        }
        QGraphicsScene::mouseReleaseEvent(event);
    }
};


// -------------------- Helpers: node/edge item data keys --------------------
static constexpr int kItemTypeKey = 0;   // 0=node, 1=edge
static constexpr int kItemIdKey   = 1;   // nodeId/edgeId
static constexpr int kTypeNode    = 0;
static constexpr int kTypeEdge    = 1;

// -------------------- Ctor/Dtor --------------------
page2_1_map_lab_guide::page2_1_map_lab_guide(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::page2_1_map_lab_guide)
{
    ui->setupUi(this);

    ensureGraphicsView();
    initScene();
    bindButtonsByObjectName();
    updateUiStateHint();
}

page2_1_map_lab_guide::~page2_1_map_lab_guide()
{
    delete ui;
}

// -------------------- UI setup --------------------
void page2_1_map_lab_guide::ensureGraphicsView()
{
    auto *gv = this->findChild<QGraphicsView*>("graphicsView");
    if (!gv) {
        // 程序创建一个 GraphicsView（简单兜底）
        gv = new QGraphicsView(this);
        gv->setObjectName("graphicsView");
        gv->setMinimumSize(800, 600);

        auto *layout = this->layout();
        if (layout) {
            layout->addWidget(gv);
        } else {
            // 没有布局就粗暴放满
            gv->setGeometry(this->rect());
            gv->raise();
        }
    }

    // 通用视图设置
    gv->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    gv->setDragMode(QGraphicsView::NoDrag); // 不允许拖拽背景，我们要自己处理点击
    gv->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    gv->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
}

void page2_1_map_lab_guide::initScene()
{
    if (!ui->graphicsView) return;

    // 【核心关键】开启鼠标追踪 (Mouse Tracking)
    ui->graphicsView->setMouseTracking(true);

    // 创建自定义 Scene
    auto *scene = new MapScene(this);
    m_scene = scene;

    ui->graphicsView->setScene(m_scene);

    // 预览折线 item
    m_previewPathItem = new QGraphicsPathItem();
    QPen previewPen(Qt::blue);
    previewPen.setStyle(Qt::DashLine);
    previewPen.setWidth(2);
    m_previewPathItem->setPen(previewPen);
    m_previewPathItem->setZValue(50);
    m_previewPathItem->setVisible(false);
    m_scene->addItem(m_previewPathItem);

    // 绑定 scene 事件回调到本页面
    scene->onLeftPress    = [this](const QPointF &p){ handleSceneLeftPress(p); };
    scene->onLeftMove     = [this](const QPointF &p){ handleSceneLeftMove(p); };
    scene->onLeftRelease  = [this](const QPointF &p){ handleSceneLeftRelease(p); };
    scene->onRightClick   = [this](const QPointF &p){ handleSceneRightClick(p); };

    // Hover 回调
    scene->onHoverMove    = [this](const QPointF &p){ updateHoverAt(p); };
}


void page2_1_map_lab_guide::bindButtonsByObjectName()
{
    auto bind = [this](const char* objName, auto slotFunc){
        if (auto *btn = this->findChild<QPushButton*>(objName)) {
            connect(btn, &QPushButton::clicked, this, slotFunc);
        }
    };

    bind("pbBack",       &page2_1_map_lab_guide::onBack);
    bind("pbLoadMap",    &page2_1_map_lab_guide::onLoadMap);
    bind("pbStartLabel", &page2_1_map_lab_guide::onStartLabel);
    bind("pbEndLabel",   &page2_1_map_lab_guide::onEndLabel);
    bind("pbStartGuide", &page2_1_map_lab_guide::onStartGuide);
    bind("pbEndGuide",   &page2_1_map_lab_guide::onEndGuide);
    bind("pbUndo",       &page2_1_map_lab_guide::onUndo);

    // 【新增修改 1】绑定保存截图按钮
    // 注意：这里的名字必须和 UI 文件里的 objectName "btn_save_image" 一致
    bind("btn_save_image", &page2_1_map_lab_guide::onSaveImageClicked);
}

void page2_1_map_lab_guide::updateUiStateHint()
{
    switch (m_state) {
    case AppState::IDLE:
        this->setWindowTitle("Map Lab/Guide - IDLE");
        break;
    case AppState::LABELING:
        this->setWindowTitle("Map Lab/Guide - LABELING");
        break;
    case AppState::GUIDING:
        this->setWindowTitle("Map Lab/Guide - GUIDING");
        break;
    }
}

// -------------------- Public APIs --------------------
bool page2_1_map_lab_guide::loadMapImage(const QString &imagePath)
{
    if (imagePath.isEmpty()) return false;

    QFileInfo fi(imagePath);
    if (!fi.exists()) {
        QMessageBox::warning(this, "加载失败", "图片文件不存在。");
        return false;
    }

    QPixmap pix(imagePath);
    if (pix.isNull()) {
        QMessageBox::warning(this, "加载失败", "无法读取图片（格式不支持或文件损坏）。");
        return false;
    }

    m_mapImageAbsPath = fi.absoluteFilePath();

    if (!m_scene) return false;

    // 清除旧背景
    if (m_bgItem) {
        m_scene->removeItem(m_bgItem);
        delete m_bgItem;
        m_bgItem = nullptr;
    }

    m_bgItem = new QGraphicsPixmapItem(pix);
    m_bgItem->setZValue(-1000);
    m_scene->addItem(m_bgItem);

    // 设置 scene 范围
    m_scene->setSceneRect(m_bgItem->boundingRect());

    // 视图适配
    if (auto *gv = this->findChild<QGraphicsView*>("graphicsView")) {
        gv->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }

    emit mapModified();
    return true;
}

void page2_1_map_lab_guide::clearAll()
{
    m_nodes.clear();
    m_edges.clear();
    m_adj.clear();

    m_isDrawingEdge = false;
    m_edgeFromNodeId = -1;
    m_currentPolyline.clear();

    m_startNodeId = -1;
    m_endNodeId = -1;

    clearHighlight();

    m_undoStack.clear();

    if (m_scene) {
        QList<QGraphicsItem*> items = m_scene->items();
        for (auto *it : items) {
            if (it == m_bgItem || it == m_previewPathItem) continue;
            m_scene->removeItem(it);
            delete it;
        }
    }

    emit mapModified();
}

// -------------------- Slots (buttons) --------------------
void page2_1_map_lab_guide::onBack()
{
    emit backToMenu();
}

void page2_1_map_lab_guide::onLoadMap()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("选择地图图片"),
        QString(),
        tr("Images (*.png *.jpg *.jpeg *.bmp);;All Files (*)")
        );
    if (path.isEmpty()) return;

    if (QFileInfo(path).suffix().compare("pdf", Qt::CaseInsensitive) == 0) {
        QMessageBox::information(this, tr("提示"), tr("PDF 暂不直接支持，请先转换为 png/jpg/bmp。"));
        return;
    }

    if (!loadNewImageOnly(path)) {
        QMessageBox::warning(this, tr("加载失败"), tr("无法加载图片：%1").arg(path));
        return;
    }

    m_state = AppState::IDLE;
    clearHighlight();
    updateUiStateHint();
}


void page2_1_map_lab_guide::onStartLabel()
{
    const bool hasBg = (m_bgItem && !m_bgItem->pixmap().isNull());
    if (!hasBg) {
        QMessageBox::information(this, "提示", "请先导入地图图片。");
        return;
    }

    m_state = AppState::LABELING;
    m_startNodeId = -1;
    m_endNodeId = -1;
    m_isDrawingEdge = false;
    m_edgeFromNodeId = -1;
    m_currentPolyline.clear();
    clearHighlight();

    updateUiStateHint();
}


void page2_1_map_lab_guide::onEndLabel()
{
    if (m_state != AppState::LABELING) {
        QMessageBox::information(this, "提示", "当前不在标注模式。");
        return;
    }
    buildAdjacency();
    m_state = AppState::IDLE;
    updateUiStateHint();

    if (!saveCurrentProject()) {
        QMessageBox::warning(this, "保存失败", "标注结束后自动保存失败，请检查目录权限。");
    }
}

void page2_1_map_lab_guide::onStartGuide()
{
    if (m_nodes.size() < 2 || m_edges.empty()) {
        QMessageBox::information(this, "提示", "请先完成标注（至少2个节点与1条路径）。");
        return;
    }
    buildAdjacency();
    clearHighlight();

    m_state = AppState::GUIDING;
    m_startNodeId = -1;
    m_endNodeId = -1;

    updateUiStateHint();
    QMessageBox::information(this, "导引模式", "请依次点击起点节点与终点节点以计算最短路径。");
}

void page2_1_map_lab_guide::onEndGuide()
{
    if (m_state != AppState::GUIDING) {
        QMessageBox::information(this, "提示", "当前不在导引模式。");
        return;
    }

    if (!saveCurrentProject()) {
        QMessageBox::warning(this, "保存失败", "导引结束保存失败，请检查目录权限。");
    }

    m_state = AppState::IDLE;
    m_startNodeId = -1;
    m_endNodeId = -1;
    updateUiStateHint();
}

// 实现保存截图的功能
void page2_1_map_lab_guide::onSaveImageClicked()
{
    // 1. 检查是否有地图内容
    if (!ui->graphicsView->scene()) {
        QMessageBox::warning(this, "提示", "当前没有地图内容，无法保存。");
        return;
    }

    // 弹出文件保存框
    QString defaultName = "Map_Route_" + QDateTime::currentDateTime().toString("HHmm") + ".png";
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "保存地图截图",
        defaultName,
        "Images (*.png *.jpg)"
        );

    if (fileName.isEmpty()) return;

    // 抓取当前绘图区
    QPixmap pixmap = ui->graphicsView->grab();

    // 4. 保存
    if (pixmap.save(fileName)) {
        QMessageBox::information(this, "成功", "图片已保存至：\n" + fileName);
    } else {
        QMessageBox::critical(this, "失败", "图片保存失败，请检查权限。");
    }
}

void page2_1_map_lab_guide::handleSceneLeftPress(const QPointF &scenePos)
{
    if (!m_bgItem) return;

    if (m_state == AppState::LABELING) {
        // 1) 若点到节点附近：准备画边
        int nid = findNearestNodeId(scenePos);
        if (nid >= 0) {
            m_isDrawingEdge = true;
            m_edgeFromNodeId = nid;
            m_currentPolyline.clear();
            m_currentPolyline.push_back(m_nodes[nid].pos);
            m_currentPolyline.push_back(scenePos);

            m_previewPathItem->setVisible(true);
            updatePreviewPath();
            return;
        }

        // 2) 点空白：新增节点
        addNodeAt(scenePos);
        emit mapModified();
        return;
    }

    if (m_state == AppState::GUIDING) {
        int nid = findNearestNodeId(scenePos);
        if (nid < 0) return;

        if (m_startNodeId < 0) {
            m_startNodeId = nid;
            QMessageBox::information(this, "导引", QString("已选择起点：Node %1").arg(nid));
        } else if (m_endNodeId < 0) {
            // 先暂存终点
            m_endNodeId = nid;

            // 【新增】Dijkstra 前先判断连通性（多子图场景）
            buildAdjacency(); // 防御：确保 m_adj 最新（你 startGuide 里也有，这里再调用也不贵）

            if (!isConnectedBfs(m_startNodeId, m_endNodeId)) {
                QMessageBox::warning(this, "不可到达",
                                     "起点与终点不在同一个连通子图中（不同孤岛），无法导航。\n"
                                     "请重新选择终点。");
                m_endNodeId = -1;   // 关键：让用户重新点终点
                clearHighlight();   // 可选：清理旧路径显示
                return;
            }

            // 连通才算最短路
            computeAndShowShortestPath();

        } else {
            // 第三次点击：重选
            m_startNodeId = nid;
            m_endNodeId = -1;
            clearHighlight();
            QMessageBox::information(this, "导引", QString("重新选择起点：Node %1").arg(nid));
        }
    }
}

// void page2_1_map_lab_guide::handleSceneLeftPress(const QPointF &scenePos)
// {
//     if (!m_bgItem) return;

//     if (m_state == AppState::LABELING) {
//         int nid = findNearestNodeId(scenePos);

//         if (m_isDrawingEdge) {
//             // 连线逻辑
//             if (nid >= 0 && nid != m_edgeFromNodeId) {
//                 if (findEdgeIdBetween(m_edgeFromNodeId, nid) >= 0) {
//                     QMessageBox::information(this, "提示", "这两点之间已经存在路径。");
//                 } else {
//                     std::vector<QPointF> linePoints;
//                     linePoints.push_back(m_nodes[m_edgeFromNodeId].pos);
//                     linePoints.push_back(m_nodes[nid].pos);
//                     addEdge(m_edgeFromNodeId, nid, linePoints);
//                 }
//             }
//             m_isDrawingEdge = false;
//             m_edgeFromNodeId = -1;
//             if (m_previewPathItem) {
//                 m_previewPathItem->setVisible(false);
//                 m_previewPathItem->setPath(QPainterPath());
//             }
//             emit mapModified();
//             return;
//         }

//         if (nid >= 0) {
//             // 选中起点
//             m_isDrawingEdge = true;
//             m_edgeFromNodeId = nid;
//             if (m_previewPathItem) {
//                 QPainterPath path;
//                 path.moveTo(m_nodes[nid].pos);
//                 path.lineTo(scenePos);
//                 m_previewPathItem->setPath(path);
//                 m_previewPathItem->setVisible(true);
//             }
//         } else {
//             // 新增节点
//             addNodeAt(scenePos);
//             emit mapModified();
//         }
//         return;
//     }

//     //导引模式
//     if (m_state == AppState::GUIDING) {
//         int nid = findNearestNodeId(scenePos);
//         if (nid < 0) return;

//         if (m_startNodeId < 0) {
//             m_startNodeId = nid;
//             QMessageBox::information(this, "导引", QString("已选择起点：Node %1").arg(nid));
//         } else if (m_endNodeId < 0) {
//             m_endNodeId = nid;
//             buildAdjacency();
//             if (!isConnectedBfs(m_startNodeId, m_endNodeId)) {
//                 QMessageBox::warning(this, "不可到达", "起点与终点不连通，请重选终点。");
//                 m_endNodeId = -1;
//                 clearHighlight();
//                 return;
//             }
//             computeAndShowShortestPath();
//         } else {
//             m_startNodeId = nid;
//             m_endNodeId = -1;
//             clearHighlight();
//             QMessageBox::information(this, "导引", QString("重新选择起点：Node %1").arg(nid));
//         }
//     }
// }

void page2_1_map_lab_guide::handleSceneLeftMove(const QPointF &scenePos)
{
    if (m_state != AppState::LABELING) return;
    if (!m_isDrawingEdge) return;

    // 画折线：移动超过一定距离才采样，避免点太密
    const double minStep = 3.0;
    if (!m_currentPolyline.empty()) {
        QPointF last = m_currentPolyline.back();
        double dx = scenePos.x() - last.x();
        double dy = scenePos.y() - last.y();
        if (std::sqrt(dx*dx + dy*dy) < minStep) {
            // 只更新末尾点，让预览线跟随鼠标
            if (m_currentPolyline.size() >= 2) {
                m_currentPolyline.back() = scenePos;
            }
            updatePreviewPath();
            return;
        }
    }

    // 追加新点
    m_currentPolyline.push_back(scenePos);
    updatePreviewPath();
}

// void page2_1_map_lab_guide::handleSceneLeftMove(const QPointF &scenePos)
// {
//     if (m_state != AppState::LABELING) return;
//     if (m_isDrawingEdge && m_edgeFromNodeId != -1) {
//         if (m_previewPathItem) {
//             QPainterPath path;
//             path.moveTo(m_nodes[m_edgeFromNodeId].pos);
//             path.lineTo(scenePos);
//             m_previewPathItem->setPath(path);
//         }
//     }
// }

void page2_1_map_lab_guide::handleSceneLeftRelease(const QPointF &scenePos)
{
    if (m_state != AppState::LABELING) return;
    if (!m_isDrawingEdge) return;

    m_isDrawingEdge = false;
    m_previewPathItem->setVisible(false);

    int toId = findNearestNodeId(scenePos);
    if (toId < 0) {
        // 未吸附到终点，取消
        m_edgeFromNodeId = -1;
        m_currentPolyline.clear();
        return;
    }
    if (toId == m_edgeFromNodeId) {
        // 起终相同，取消
        m_edgeFromNodeId = -1;
        m_currentPolyline.clear();
        return;
    }

    // 折线末端吸附到终点真实节点位置
    if (!m_currentPolyline.empty()) {
        m_currentPolyline.back() = m_nodes[toId].pos;
    } else {
        m_currentPolyline.push_back(m_nodes[m_edgeFromNodeId].pos);
        m_currentPolyline.push_back(m_nodes[toId].pos);
    }

    // 禁止重复边（无向）
    if (findEdgeIdBetween(m_edgeFromNodeId, toId) >= 0) {
        QMessageBox::information(this, "提示", "该两节点之间已存在路径，已忽略重复添加。");
        m_edgeFromNodeId = -1;
        m_currentPolyline.clear();
        return;
    }

    addEdge(m_edgeFromNodeId, toId, m_currentPolyline);
    m_edgeFromNodeId = -1;
    m_currentPolyline.clear();

    emit mapModified();
}

// void page2_1_map_lab_guide::handleSceneLeftRelease(const QPointF &scenePos)
// {
//     Q_UNUSED(scenePos);
// }

void page2_1_map_lab_guide::handleSceneRightClick(const QPointF &scenePos)
{
    if (!m_scene) return;

    QGraphicsItem *clicked = m_scene->itemAt(scenePos, QTransform());
    int type = -1, id = -1;
    if (clicked) {
        type = clicked->data(kItemTypeKey).toInt();
        id = clicked->data(kItemIdKey).toInt();
    }

    QMenu menu(this);

    if (type == kTypeNode) {
        QAction *actAddStreet = menu.addAction("导入街景图...");
        QAction *actDelNode   = menu.addAction("删除该节点");
        QAction *actViewStreet = menu.addAction("查看路景图");

        QAction *act = menu.exec(QCursor::pos());
        if (!act) return;

        if (act == actAddStreet) addStreetImageToNode(id);
        if (act == actDelNode)   deleteNode(id);

        if (act == actViewStreet) {
            QStringList imgs;
            for (const auto &p : m_nodes[id].imagePathsAbs) imgs << p;
            StreetViewerDialog dlg(imgs, 0, this);
            dlg.exec();
        }

    } else if (type == kTypeEdge) {
        QAction *actDelEdge = menu.addAction("删除该路径");
        QAction *act = menu.exec(QCursor::pos());
        if (!act) return;
        if (act == actDelEdge) deleteEdge(id);

    } else {
        QAction *actLoad = menu.addAction("导入地图...");
        QAction *act = menu.exec(QCursor::pos());
        if (!act) return;
        if (act == actLoad) onLoadMap();
    }
}

// -------------------- Model ops --------------------
int page2_1_map_lab_guide::addNodeAt(const QPointF &scenePos, const QString &name)
{
    int id = static_cast<int>(m_nodes.size());
    Node n;
    n.id = id;
    n.name = name.isEmpty() ? QString("Node %1").arg(id) : name;
    n.pos = scenePos;
    m_nodes.push_back(n);


    const double r = 10.0;
    QPen pen(Qt::white, 3);
    QBrush brush(QColor("#FF5252"));

    m_nodePenNormal = pen;
    m_nodeBrushNormal = brush;

    auto *circle = m_scene->addEllipse(scenePos.x() - r, scenePos.y() - r, 2 * r, 2 * r,
                                       pen, brush);

    const int NODE_Z_VALUE = 100;
    circle->setZValue(NODE_Z_VALUE);
    circle->setData(kItemTypeKey, kTypeNode);
    circle->setData(kItemIdKey, id);
    circle->setFlag(QGraphicsItem::ItemIsSelectable, true);

    m_nodeItemById[id] = circle;

    auto *label = m_scene->addText(QString::number(id));
    QFont font("Microsoft YaHei", 10, QFont::Bold);
    label->setFont(font);
    label->setDefaultTextColor(Qt::darkBlue);

    label->setParentItem(circle);
    const double offset = r + 2;
    label->setPos(offset, -offset);
    label->setZValue(NODE_Z_VALUE + 1);

    label->setData(kItemTypeKey, kTypeNode);
    label->setData(kItemIdKey, id);

    if (m_state == AppState::LABELING) {
        m_undoStack.push_back({CmdAddNode, id});
    }

    emit mapModified();
    return id;
}


int page2_1_map_lab_guide::addEdge(int fromId, int toId, const std::vector<QPointF> &polyline)
{
    int id = static_cast<int>(m_edges.size());
    Edge e;
    e.id = id;
    e.from = fromId;
    e.to = toId;
    e.polyline = polyline;
    e.length = calcLength(polyline) * m_scale;
    m_edges.push_back(e);

    QPainterPath path;
    if (!polyline.empty()) {
        path.moveTo(polyline.front());
        for (size_t i = 1; i < polyline.size(); ++i) {
            path.lineTo(polyline[i]);
        }
    }

    QPen edgePenNormal(QColor("#AAB8C2"), 3);
    edgePenNormal.setCapStyle(Qt::RoundCap);

    auto *item = m_scene->addPath(path, edgePenNormal);
    item->setZValue(10);
    item->setData(kItemTypeKey, kTypeEdge);
    item->setData(kItemIdKey, id);

    m_edgeItemById[id] = item;

    if (m_state == AppState::LABELING) {
        m_undoStack.push_back({CmdAddEdge, id});
    }

    return id;
}

void page2_1_map_lab_guide::deleteNode(int nodeId)
{
    if (nodeId < 0 || nodeId >= static_cast<int>(m_nodes.size())) return;
    if (m_nodes[nodeId].id == -1) return;

    if (m_hoverNodeId == nodeId) {
        m_hoverNodeId = -1;
    }

    for (int i = 0; i < static_cast<int>(m_edges.size()); ++i) {
        if (m_edges[i].id != -1) {
            if (m_edges[i].from == nodeId || m_edges[i].to == nodeId) {
                if (m_hoverEdgeId == m_edges[i].id) m_hoverEdgeId = -1;
                deleteEdge(m_edges[i].id);
            }
        }
    }

    auto it = m_nodeItemById.find(nodeId);
    if (it != m_nodeItemById.end() && it.value() != nullptr) {
        QGraphicsEllipseItem *item = it.value();
        if (item->scene()) item->scene()->removeItem(item);
        delete item;
    }
    m_nodeItemById.remove(nodeId);

    m_nodes[nodeId].id = -1;
    m_nodes[nodeId].name = "[DELETED]";
    m_nodes[nodeId].pos = QPointF(-1e9, -1e9);
    m_nodes[nodeId].imagePathsAbs.clear();

    if (m_isDrawingEdge && m_edgeFromNodeId == nodeId) {
        m_isDrawingEdge = false;
        m_edgeFromNodeId = -1;
        if (m_previewPathItem) m_previewPathItem->setPath(QPainterPath());
    }
    if (m_startNodeId == nodeId) m_startNodeId = -1;
    if (m_endNodeId == nodeId)   m_endNodeId = -1;

    buildAdjacency();
    emit mapModified();
}

void page2_1_map_lab_guide::deleteEdge(int edgeId)
{
    if (edgeId < 0 || edgeId >= static_cast<int>(m_edges.size()))
        return;

    if (m_hoverEdgeId == edgeId) {
        m_hoverEdgeId = -1;
    }

    auto newEnd = std::remove(m_highlightEdgeIds.begin(), m_highlightEdgeIds.end(), edgeId);
    m_highlightEdgeIds.erase(newEnd, m_highlightEdgeIds.end());

    auto it = m_edgeItemById.find(edgeId);
    if (it != m_edgeItemById.end() && it.value() != nullptr) {
        QGraphicsPathItem *item = it.value();
        if (item->scene()) item->scene()->removeItem(item);
        delete item;
    }
    m_edgeItemById.remove(edgeId);

    m_edges[edgeId].from = -1;
    m_edges[edgeId].to   = -1;
    m_edges[edgeId].polyline.clear();
    m_edges[edgeId].length = 0.0;

    buildAdjacency();
    emit mapModified();
}

int page2_1_map_lab_guide::findNearestNodeId(const QPointF &scenePos, double maxDistPx) const
{
    int bestId = -1;
    double bestD = maxDistPx;

    for (const auto &n : m_nodes) {
        if (n.id < 0) continue;
        if (n.pos.x() < -1e8) continue;
        double dx = scenePos.x() - n.pos.x();
        double dy = scenePos.y() - n.pos.y();
        double d = std::sqrt(dx*dx + dy*dy);
        if (d < bestD) {
            bestD = d;
            bestId = n.id;
        }
    }
    return bestId;
}

int page2_1_map_lab_guide::findEdgeIdBetween(int a, int b) const
{
    for (const auto &e : m_edges) {
        if (e.from < 0 || e.to < 0) continue;
        if ((e.from == a && e.to == b) || (e.from == b && e.to == a)) return e.id;
    }
    return -1;
}

double page2_1_map_lab_guide::calcLength(const std::vector<QPointF> &polyline)
{
    double len = 0.0;
    for (size_t i = 1; i < polyline.size(); ++i) {
        double dx = polyline[i].x() - polyline[i-1].x();
        double dy = polyline[i].y() - polyline[i-1].y();
        len += std::sqrt(dx*dx + dy*dy);
    }
    return len;
}

// -------------------- Graphics ops --------------------
void page2_1_map_lab_guide::clearHighlight()
{
    for (int eid : m_highlightEdgeIds) {
        if (eid == m_hoverEdgeId) continue;

        auto it = m_edgeItemById.find(eid);
        if (it != m_edgeItemById.end() && it.value() != nullptr) {
            it.value()->setPen(m_edgePenNormal);
        }
    }
    m_highlightEdgeIds.clear();
}

void page2_1_map_lab_guide::highlightEdges(const std::vector<int> &edgeIds)
{
    clearHighlight();
    m_highlightEdgeIds = edgeIds;

    for (int eid : m_highlightEdgeIds) {
        if (eid == m_hoverEdgeId) continue;

        auto it = m_edgeItemById.find(eid);
        if (it != m_edgeItemById.end() && it.value() != nullptr) {
            it.value()->setPen(m_edgePenHighlight);
        }
    }
}

void page2_1_map_lab_guide::updatePreviewPath()
{
    if (!m_previewPathItem) return;
    if (m_currentPolyline.size() < 2) return;

    QPainterPath path;
    path.moveTo(m_currentPolyline.front());
    for (size_t i=1;i<m_currentPolyline.size();++i) path.lineTo(m_currentPolyline[i]);
    m_previewPathItem->setPath(path);
}


void page2_1_map_lab_guide::addStreetImageToNode(int nodeId)
{
    if (nodeId < 0 || nodeId >= (int)m_nodes.size()) return;

    QString img = QFileDialog::getOpenFileName(
        this,
        "选择街景图片",
        QString(),
        "Images (*.png *.jpg *.jpeg *.bmp);;All Files (*)"
        );
    if (img.isEmpty()) return;

    QFileInfo fi(img);
    if (!fi.exists()) return;

    m_nodes[nodeId].imagePathsAbs.push_back(fi.absoluteFilePath());
    emit mapModified();

    QMessageBox::information(this, "已添加", QString("已为 Node %1 添加街景图：\n%2").arg(nodeId).arg(fi.fileName()));
}

//------------------------------BFS-------------------

bool page2_1_map_lab_guide::isConnectedBfs(int startId, int endId)
{
    if (startId < 0 || endId < 0) return false;
    if (startId >= (int)m_adj.size() || endId >= (int)m_adj.size()) return false;
    if (startId == endId) return true;

    std::vector<char> vis(m_adj.size(), 0);
    std::queue<int> q;

    vis[startId] = 1;
    q.push(startId);

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        for (const auto &ae : m_adj[u]) {
            int v = ae.to;
            if (v < 0 || v >= (int)m_adj.size()) continue;
            if (vis[v]) continue;

            if (v == endId) return true;

            vis[v] = 1;
            q.push(v);
        }
    }
    return false;
}

// -------------------- Dijkstra --------------------
void page2_1_map_lab_guide::buildAdjacency()
{
    int n = (int)m_nodes.size();
    m_adj.assign(n, {});

    for (const auto &e : m_edges) {
        if (e.from < 0 || e.to < 0) continue;
        if (e.from >= n || e.to >= n) continue;

        if (m_nodes[e.from].pos.x() < -1e8) continue;
        if (m_nodes[e.to].pos.x() < -1e8) continue;

        m_adj[e.from].push_back({e.to, e.length, e.id});
        m_adj[e.to].push_back({e.from, e.length, e.id});
    }
}

bool page2_1_map_lab_guide::dijkstra(int startId, int endId,
                                     std::vector<int> &outNodePath, double &outDist)
{
    outNodePath.clear();
    outDist = std::numeric_limits<double>::infinity();

    if (startId < 0 || endId < 0) return false;
    if (startId >= (int)m_adj.size() || endId >= (int)m_adj.size()) return false;

    const int n = (int)m_adj.size();
    std::vector<double> dist(n, std::numeric_limits<double>::infinity());
    std::vector<int> prev(n, -1);

    using P = std::pair<double,int>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;

    dist[startId] = 0.0;
    pq.push({0.0, startId});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[u]) continue;
        if (u == endId) break;

        for (const auto &ae : m_adj[u]) {
            int v = ae.to;
            double nd = d + ae.w;
            if (nd < dist[v]) {
                dist[v] = nd;
                prev[v] = u;
                pq.push({nd, v});
            }
        }
    }

    if (!std::isfinite(dist[endId])) return false;

    for (int cur = endId; cur != -1; cur = prev[cur]) outNodePath.push_back(cur);
    std::reverse(outNodePath.begin(), outNodePath.end());

    outDist = dist[endId];
    return true;
}

// 【新增修改 3】重写该函数，添加自动缩放聚焦功能
void page2_1_map_lab_guide::computeAndShowShortestPath()
{
    if (m_startNodeId < 0 || m_endNodeId < 0) return;

    // 1. 确保邻接表是最新的
    buildAdjacency();

    // 2. 运行 Dijkstra 算法
    std::vector<int> nodePath;
    double dist = 0.0;
    if (!dijkstra(m_startNodeId, m_endNodeId, nodePath, dist)) {
        QMessageBox::warning(this, "不可到达", "起点与终点之间不存在可达路径。");
        clearHighlight();
        return;
    }

    // 3. 高亮路径
    std::vector<int> edgeIds;
    // --- 创建一个多边形容器，用于计算路径的包围盒 ---
    QPolygonF pathPolygon;

    for (size_t i = 0; i < nodePath.size(); ++i) {
        // 收集边 ID
        if (i > 0) {
            int a = nodePath[i-1];
            int b = nodePath[i];
            int eid = findEdgeIdBetween(a, b);
            if (eid >= 0) edgeIds.push_back(eid);
        }

        // 收集点坐标，用于聚焦
        int nid = nodePath[i];
        if (nid >= 0 && nid < (int)m_nodes.size()) {
            pathPolygon << m_nodes[nid].pos;
        }
    }

    highlightEdges(edgeIds);
    QMessageBox::information(this, "最短路径",
                             QString("最短路径节点数：%1\n总距离：%2")
                                 .arg((int)nodePath.size())
                                 .arg(dist));

    // 4. 自动聚焦 (Zoom to fit)
    if (!pathPolygon.isEmpty()) {
        QRectF rect = pathPolygon.boundingRect();

        // 防止路径太短导致缩放过大，设置最小显示范围 (200x200)
        double minSize = 200.0;
        if (rect.width() < minSize) {
            double center = rect.center().x();
            rect.setLeft(center - minSize/2);
            rect.setRight(center + minSize/2);
        }
        if (rect.height() < minSize) {
            double center = rect.center().y();
            rect.setTop(center - minSize/2);
            rect.setBottom(center + minSize/2);
        }

        // 添加留白 (Padding)
        double padding = 50.0;
        rect.adjust(-padding, -padding, padding, padding);

        // 执行缩放
        if (ui->graphicsView) {
            ui->graphicsView->fitInView(rect, Qt::KeepAspectRatio);
        }
    }

    emit mapModified();
}

// -------------------- Save project --------------------
QString page2_1_map_lab_guide::projectRootDir() const
{
    return QDir::cleanPath(QDir::currentPath() + "/Map_guide");
}

int page2_1_map_lab_guide::allocateNextMapId(const QString &rootDirAbs) const
{
    QDir root(rootDirAbs);
    if (!root.exists()) return 1;

    int maxId = 0;
    const auto entries = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    QRegularExpression re(R"(Saved_Map#(\d+))");

    for (const QString &name : entries) {
        auto m = re.match(name);
        if (m.hasMatch()) {
            bool ok = false;
            int id = m.captured(1).toInt(&ok);
            if (ok) maxId = std::max(maxId, id);
        }
    }
    return maxId + 1;
}

bool page2_1_map_lab_guide::saveCurrentProject()
{
    if (m_mapImageAbsPath.isEmpty() || !m_bgItem) {
        return false;
    }

    const QString root = projectRootDir();
    QDir().mkpath(root);

    const int mapId = allocateNextMapId(root);
    const QString projDir = QDir::cleanPath(root + QString("/Saved_Map#%1").arg(mapId));
    const QString dataDir = QDir::cleanPath(projDir + "/guide_data");
    const QString streetDir = QDir::cleanPath(projDir + "/streetscape");

    QDir().mkpath(projDir);
    QDir().mkpath(dataDir);
    QDir().mkpath(streetDir);

    {
        const QString dst = QDir::cleanPath(projDir + "/map_image.png");
        if (QFile::exists(dst)) QFile::remove(dst);
        if (!QFile::copy(m_mapImageAbsPath, dst)) {
            QMessageBox::warning(this, "保存失败", "背景图复制失败。");
            return false;
        }
    }

    {
        auto *gv = this->findChild<QGraphicsView*>("graphicsView");
        if (gv) {
            QPixmap shot = gv->grab();
            shot.save(QDir::cleanPath(projDir + "/map_view.png"));
        }
    }

    QJsonObject rootObj;
    rootObj["map_id"] = mapId;
    rootObj["scale"] = m_scale;
    rootObj["map_image"] = "map_image.png";

    QJsonArray nodesArr;
    for (const auto &n : m_nodes) {
        if (n.id < 0) continue;
        if (n.pos.x() < -1e8) continue;

        QJsonObject no;
        no["id"] = n.id;
        no["name"] = n.name;
        no["x"] = n.pos.x();
        no["y"] = n.pos.y();

        QJsonArray imgRelArr;
        for (size_t k=0;k<n.imagePathsAbs.size();++k) {
            QFileInfo fi(n.imagePathsAbs[k]);
            if (!fi.exists()) continue;

            const QString ext = fi.suffix().isEmpty() ? "png" : fi.suffix();
            const QString rel = QString("streetscape/Node%1_%2.%3").arg(n.id).arg((int)k).arg(ext);
            const QString absDst = QDir::cleanPath(projDir + "/" + rel);

            if (QFile::exists(absDst)) QFile::remove(absDst);
            QFile::copy(fi.absoluteFilePath(), absDst);

            imgRelArr.append(rel);
        }
        no["streetscapes"] = imgRelArr;

        nodesArr.append(no);
    }
    rootObj["nodes"] = nodesArr;

    QJsonArray edgesArr;
    for (const auto &e : m_edges) {
        if (e.from < 0 || e.to < 0) continue;
        if (e.polyline.empty()) continue;

        QJsonObject eo;
        eo["id"] = e.id;
        eo["from"] = e.from;
        eo["to"] = e.to;
        eo["length"] = e.length;

        QJsonArray polyArr;
        for (const auto &p : e.polyline) {
            QJsonArray pt;
            pt.append(p.x());
            pt.append(p.y());
            polyArr.append(pt);
        }
        eo["polyline"] = polyArr;

        edgesArr.append(eo);
    }
    rootObj["edges"] = edgesArr;

    const QString jsonPath = QDir::cleanPath(dataDir + "/graph.json");
    QFile f(jsonPath);
    if (!f.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "保存失败", "graph.json 无法写入。");
        return false;
    }
    f.write(QJsonDocument(rootObj).toJson(QJsonDocument::Indented));
    f.close();

    return true;
}

// -------------------- Hover Logic --------------------
bool page2_1_map_lab_guide::isEdgeHighlighted(int edgeId) const
{
    return std::find(m_highlightEdgeIds.begin(), m_highlightEdgeIds.end(), edgeId)
           != m_highlightEdgeIds.end();
}

void page2_1_map_lab_guide::restoreEdgeStyle(int edgeId)
{
    auto it = m_edgeItemById.find(edgeId);
    if (it == m_edgeItemById.end() || it.value() == nullptr) return;

    it.value()->setPen(isEdgeHighlighted(edgeId) ? m_edgePenHighlight : m_edgePenNormal);
}

void page2_1_map_lab_guide::restoreNodeStyle(int nodeId)
{
    auto it = m_nodeItemById.find(nodeId);
    if (it == m_nodeItemById.end() || it.value() == nullptr) return;

    it.value()->setPen(m_nodePenNormal);
    it.value()->setBrush(m_nodeBrushNormal);
}

void page2_1_map_lab_guide::clearHover()
{
    if (m_hoverEdgeId != -1) {
        restoreEdgeStyle(m_hoverEdgeId);
        m_hoverEdgeId = -1;
    }
    if (m_hoverNodeId != -1) {
        restoreNodeStyle(m_hoverNodeId);
        m_hoverNodeId = -1;
    }
}

void page2_1_map_lab_guide::updateHoverAt(const QPointF &scenePos)
{
    if (m_isDrawingEdge) return;
    if (!m_scene) return;

    QGraphicsItem *hit = m_scene->itemAt(scenePos, QTransform());

    int newNode = -1;
    int newEdge = -1;

    if (hit) {
        const int type = hit->data(kItemTypeKey).toInt();
        const int id   = hit->data(kItemIdKey).toInt();
        if (type == kTypeNode) newNode = id;
        else if (type == kTypeEdge) newEdge = id;
    }

    if (newNode != -1) newEdge = -1;

    // --- edge hover ---
    if (newEdge != m_hoverEdgeId) {
        if (m_hoverEdgeId != -1) restoreEdgeStyle(m_hoverEdgeId);
        m_hoverEdgeId = newEdge;

        if (m_hoverEdgeId != -1) {
            auto it = m_edgeItemById.find(m_hoverEdgeId);
            if (it != m_edgeItemById.end() && it.value() != nullptr) {
                it.value()->setPen(m_edgePenHover); // 绿色
            }
        }
    }

    // --- node hover ---
    if (newNode != m_hoverNodeId) {
        if (m_hoverNodeId != -1) restoreNodeStyle(m_hoverNodeId);
        m_hoverNodeId = newNode;

        if (m_hoverNodeId != -1) {
            auto it = m_nodeItemById.find(m_hoverNodeId);
            if (it != m_nodeItemById.end() && it.value() != nullptr) {
                it.value()->setPen(m_nodePenHover);
                it.value()->setBrush(m_nodeBrushHover);
            }
        }
    }

    if (newNode == -1 && newEdge == -1) {
        clearHover();
    }
}

// -------------------- Loading / Parsing --------------------
bool page2_1_map_lab_guide::loadProject(const QString &path, bool isNewProject)
{
    if (isNewProject) {
        return loadNewImageOnly(path);
    } else {
        return loadExistingProjectDir(path);
    }
}

bool page2_1_map_lab_guide::loadNewImageOnly(const QString &imageAbsPath)
{
    if (!QFileInfo::exists(imageAbsPath)) return false;

    clearProjectVisualAndData();

    if (!setBackgroundImage(imageAbsPath)) {
        return false;
    }

    m_scale = 1.0;
    buildAdjacency();
    emit mapModified();
    return true;
}

bool page2_1_map_lab_guide::loadExistingProjectDir(const QString &projectRoot)
{
    QDir dir(projectRoot);
    if (!dir.exists()) return false;

    QString jsonPath = dir.filePath("guide_data/graph.json");
    if (!QFileInfo::exists(jsonPath)) {
        jsonPath = dir.filePath("graph.json");
    }
    if (!QFileInfo::exists(jsonPath)) return false;

    clearProjectVisualAndData();

    const bool ok = loadGraphJson(jsonPath, projectRoot);
    if (!ok) {
        return false;
    }
    return true;
}

void page2_1_map_lab_guide::clearProjectVisualAndData()
{
    m_undoStack.clear();

    m_highlightEdgeIds.clear();
    m_mapImageAbsPath.clear();

    m_isDrawingEdge = false;
    m_edgeFromNodeId = -1;
    m_startNodeId = -1;
    m_endNodeId   = -1;

    for (auto it = m_edgeItemById.begin(); it != m_edgeItemById.end(); ++it) {
        if (it.value()) {
            if (it.value()->scene()) it.value()->scene()->removeItem(it.value());
            delete it.value();
        }
    }
    m_edgeItemById.clear();

    for (auto it = m_nodeItemById.begin(); it != m_nodeItemById.end(); ++it) {
        if (it.value()) {
            if (it.value()->scene()) it.value()->scene()->removeItem(it.value());
            delete it.value();
        }
    }
    m_nodeItemById.clear();

    if (m_bgItem) {
        if (m_scene) m_scene->removeItem(m_bgItem);
        delete m_bgItem;
        m_bgItem = nullptr;
    }

    if (m_previewPathItem) {
        m_previewPathItem->setPath(QPainterPath());
        m_previewPathItem->setVisible(false);
    }

    m_nodes.clear();
    m_edges.clear();
}

bool page2_1_map_lab_guide::setBackgroundImage(const QString &imageAbsPath)
{
    if (!m_scene) return false;

    QPixmap pix(imageAbsPath);
    if (pix.isNull()) return false;

    if (!m_bgItem) {
        m_bgItem = m_scene->addPixmap(pix);
        m_bgItem->setZValue(0);
    } else {
        m_bgItem->setPixmap(pix);
    }

    m_mapImageAbsPath = QFileInfo(imageAbsPath).absoluteFilePath();
    return true;
}

static QPointF parsePoint2(const QJsonValue &v)
{
    const QJsonArray a = v.toArray();
    if (a.size() < 2) return QPointF();
    return QPointF(a[0].toDouble(), a[1].toDouble());
}

bool page2_1_map_lab_guide::loadGraphJson(const QString &jsonPath, const QString &projectRoot)
{
    if (!m_scene) return false;

    QFile f(jsonPath);
    if (!f.open(QIODevice::ReadOnly)) return false;

    const QByteArray data = f.readAll();
    f.close();

    QJsonParseError err {};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return false;

    const QJsonObject root = doc.object();

    m_scale = root.value("scale").toDouble(1.0);

    const QString mapImageRel = root.value("map_image").toString();
    if (mapImageRel.isEmpty()) {
        return false;
    }

    const QString mapAbs = QDir(projectRoot).filePath(mapImageRel);

    if (!setBackgroundImage(mapAbs)) {
        return false;
    }

    const QJsonArray nodesArr = root.value("nodes").toArray();

    int maxNodeId = -1;
    for (const auto &nv : nodesArr) {
        const QJsonObject no = nv.toObject();
        maxNodeId = std::max(maxNodeId, no.value("id").toInt(-1));
    }
    if (maxNodeId >= 0) {
        m_nodes.resize(static_cast<size_t>(maxNodeId + 1));
        for (int i = 0; i <= maxNodeId; ++i) {
            m_nodes[i].id = -1;
        }
    }

    for (const auto &nv : nodesArr) {
        const QJsonObject no = nv.toObject();
        const int id = no.value("id").toInt(-1);
        if (id < 0) continue;

        Node n;
        n.id = id;
        n.name = no.value("name").toString(QString("Node %1").arg(id));
        n.pos = QPointF(no.value("x").toDouble(), no.value("y").toDouble());

        const QJsonArray scArr = no.value("streetscapes").toArray();
        n.imagePathsAbs.clear();
        for (const auto &sv : scArr) {
            const QString rel = sv.toString();
            if (rel.isEmpty()) continue;
            n.imagePathsAbs.push_back(QDir(projectRoot).filePath(rel));
        }

        if (id >= 0 && id < (int)m_nodes.size()) {
            m_nodes[id] = n;
        }

        const double r = 10.0;
        const int NODE_Z_VALUE = 100;

        auto *circle = m_scene->addEllipse(n.pos.x()-r, n.pos.y()-r, 2*r, 2*r,
                                           QPen(Qt::white, 3), QBrush(QColor("#FF5252")));

        circle->setZValue(NODE_Z_VALUE);
        circle->setData(kItemTypeKey, kTypeNode);
        circle->setData(kItemIdKey, id);
        circle->setFlag(QGraphicsItem::ItemIsSelectable, true);

        m_nodeItemById[id] = circle;

        auto *label = m_scene->addText(QString::number(id));
        label->setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
        label->setDefaultTextColor(Qt::darkBlue);

        label->setParentItem(circle);
        label->setPos(n.pos.x() + 12, n.pos.y() - 12);

        label->setZValue(NODE_Z_VALUE + 1);
        label->setData(kItemTypeKey, kTypeNode);
        label->setData(kItemIdKey, id);
    }

    const QJsonArray edgesArr = root.value("edges").toArray();

    int maxEdgeId = -1;
    for (const auto &ev : edgesArr) {
        const QJsonObject eo = ev.toObject();
        maxEdgeId = std::max(maxEdgeId, eo.value("id").toInt(-1));
    }
    if (maxEdgeId >= 0) {
        m_edges.resize(static_cast<size_t>(maxEdgeId + 1));
        for (int i = 0; i <= maxEdgeId; ++i) {
            m_edges[i].id = -1;
            m_edges[i].from = -1;
            m_edges[i].to = -1;
        }
    }

    for (const auto &ev : edgesArr) {
        const QJsonObject eo = ev.toObject();

        const int id   = eo.value("id").toInt(-1);
        const int from = eo.value("from").toInt(-1);
        const int to   = eo.value("to").toInt(-1);
        if (id < 0 || from < 0 || to < 0) continue;

        Edge e;
        e.id = id;
        e.from = from;
        e.to = to;
        e.length = eo.value("length").toDouble(0.0);

        e.polyline.clear();
        const QJsonArray pl = eo.value("polyline").toArray();
        for (const auto &pv : pl) {
            e.polyline.push_back(parsePoint2(pv));
        }

        if (id >= 0 && id < (int)m_edges.size()) {
            m_edges[id] = e;
        }

        QPainterPath path;
        if (!e.polyline.empty()) {
            path.moveTo(e.polyline.front());
            for (size_t i = 1; i < e.polyline.size(); ++i) path.lineTo(e.polyline[i]);
        }

        auto *item = m_scene->addPath(path, QPen(Qt::darkGray, 2));
        item->setZValue(10);
        item->setData(kItemTypeKey, kTypeEdge);
        item->setData(kItemIdKey, id);

        m_edgeItemById[id] = item;
    }

    buildAdjacency();
    emit mapModified();
    return true;
}

void page2_1_map_lab_guide::onUndo()
{
    if (m_state != AppState::LABELING) {
        return;
    }
    if (m_undoStack.empty()) {
        return;
    }

    UndoCommand lastCmd = m_undoStack.back();
    m_undoStack.pop_back();

    if (lastCmd.type == CmdAddNode) {
        deleteNode(lastCmd.id);
    } else if (lastCmd.type == CmdAddEdge) {
        deleteEdge(lastCmd.id);
    }

    if (m_isDrawingEdge) {
        m_isDrawingEdge = false;
        m_edgeFromNodeId = -1;
        m_currentPolyline.clear();
        if (m_previewPathItem) {
            m_previewPathItem->setVisible(false);
            m_previewPathItem->setPath(QPainterPath());
        }
    }

    emit mapModified();
}


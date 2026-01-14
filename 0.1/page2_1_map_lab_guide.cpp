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
#include <algorithm>   // std::find / std::remove


#include <QPainterPath>
#include <QDebug>

#include "streetviewerdialog.h"


// -------------------- Scene subclass (for mouse events) --------------------
class MapScene : public QGraphicsScene
{
public:
    using QGraphicsScene::QGraphicsScene;

    std::function<void(const QPointF&)> onLeftPress;
    std::function<void(const QPointF&)> onLeftMove;
    std::function<void(const QPointF&)> onLeftRelease;
    std::function<void(const QPointF&)> onRightClick;

    // 【新增】hover move：不要求按住左键
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
        // 【新增】hover 更新
        if (onHoverMove) onHoverMove(event->scenePos());

        // 你原本的 leftMove 仍保留（画边拖动）
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
    gv->setDragMode(QGraphicsView::ScrollHandDrag);
    gv->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    gv->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
}

void page2_1_map_lab_guide::initScene()
{
    // 【修改建议 1】直接使用 ui->graphicsView
    // 既然使用了 .ui 文件，直接用 ui 指针比 findChild 更快、更安全。
    // 由于进行了“提升”，ui->graphicsView 的类型已经是 InteractiveView* 了。
    if (!ui->graphicsView) return;

    // 【核心关键】开启鼠标追踪 (Mouse Tracking)
    //InteractiveView 默认只处理按键事件。
    // 为了让您的 "onHoverMove" (绿色高亮) 在不按鼠标时也能触发，必须开启此项！
    ui->graphicsView->setMouseTracking(true);

    // 创建自定义 Scene
    // 注意：这里用 auto 也可以，但明确类型更好，假设您的 Scene 类叫 MapScene
    auto *scene = new MapScene(this);
    m_scene = scene;

    ui->graphicsView->setScene(m_scene);

    // --- 以下保持不变 ---

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
    scene->onHoverMove    = [this](const QPointF &p){ updateHoverAt(p); }; // 需实现 updateHoverAt
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

    if (m_scene) {
        // 保留背景和预览 item 的话可以不清空；这里重建更干净
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

    // PDF：这里只做提示，实际转换你可以后续加
    if (QFileInfo(path).suffix().compare("pdf", Qt::CaseInsensitive) == 0) {
        QMessageBox::information(this, tr("提示"), tr("PDF 暂不直接支持，请先转换为 png/jpg/bmp。"));
        return;
    }

    // 关键：导入新图 = 新项目，必须清空旧节点/旧边/旧高亮/旧邻接表
    if (!loadNewImageOnly(path)) {
        QMessageBox::warning(this, tr("加载失败"), tr("无法加载图片：%1").arg(path));
        return;
    }

    // 进入标注前的初始状态
    m_state = AppState::IDLE;
    clearHighlight();          // 可选：保险起见
    updateUiStateHint();
}


void page2_1_map_lab_guide::onStartLabel()
{

    // 以背景是否真实存在为准，避免状态分裂
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
    // 标注结束：构建邻接表，为导引准备
    buildAdjacency();
    m_state = AppState::IDLE;
    updateUiStateHint();

    // 自动保存（可选）：指导要求“标注完成后系统开始解析并保存”
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

    // 导引结束：建议保存“完整视图截图”等
    if (!saveCurrentProject()) {
        QMessageBox::warning(this, "保存失败", "导引结束保存失败，请检查目录权限。");
    }

    m_state = AppState::IDLE;
    m_startNodeId = -1;
    m_endNodeId = -1;
    updateUiStateHint();
}

// -------------------- Scene interaction --------------------
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
        // 【新增】查看路景动作
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
// page2_1_map_lab_guide.cpp

int page2_1_map_lab_guide::addNodeAt(const QPointF &scenePos, const QString &name)
{
    // --- 1. 数据层添加 ---
    int id = static_cast<int>(m_nodes.size());
    Node n;
    n.id = id;
    n.name = name.isEmpty() ? QString("Node %1").arg(id) : name;
    n.pos = scenePos;
    m_nodes.push_back(n);

    // --- 2. 图形层添加 (Item) - 美化和优化 ---

    // 定义常量和样式
    const double NODE_RADIUS = 6.0;
    const QPen NODE_PEN_NORMAL = QPen(QColor("#1565C0"), 1.5); // 深蓝边框
    const QBrush NODE_BRUSH_NORMAL = QBrush(QColor("#42A5F5")); // 浅蓝填充
    const int NODE_Z_VALUE = 100;

    const double r = NODE_RADIUS;

    // 创建圆点
    auto *circle = m_scene->addEllipse(scenePos.x() - r, scenePos.y() - r, 2 * r, 2 * r,
                                       NODE_PEN_NORMAL, NODE_BRUSH_NORMAL);

    circle->setZValue(NODE_Z_VALUE);
    circle->setData(kItemTypeKey, kTypeNode);
    circle->setData(kItemIdKey, id);
    circle->setFlag(QGraphicsItem::ItemIsSelectable, true);

    // 【关键修改1】存入映射表 (使用裸指针)
    m_nodeItemById[id] = circle;

    // --- 3. 添加文字标签 - 美化和优化 ---

    // 创建文字
    auto *label = m_scene->addText(QString::number(id));

    // 设置字体和颜色
    QFont font("Arial", 9, QFont::Bold); // 字体：Arial，9pt，粗体
    label->setFont(font);
    label->setDefaultTextColor(Qt::darkGreen); // 文本颜色设置为深绿色

    // 【关键修改2】设置父子关系
    label->setParentItem(circle);

    // 设置坐标：使用相对坐标
    // 偏移量：圆点半径 + 间距 (r + 1 像素)
    const double offset = r + 1;

    // 相对于父项 (circle) 的中心 (0, 0) 进行偏移
    // 设置标签在圆点右侧上方附近
    label->setPos(offset, -offset);

    // Z值比圆点更高，确保文字不被圆点遮挡
    label->setZValue(NODE_Z_VALUE + 1);

    // 设置数据
    label->setData(kItemTypeKey, kTypeNode);
    label->setData(kItemIdKey, id);

    // --- 4. 结束与返回 ---
    emit mapModified();
    return id;
}

int page2_1_map_lab_guide::addEdge(int fromId, int toId, const std::vector<QPointF> &polyline)
{
    // --- 1. 数据层添加 (保持不变) ---
    int id = static_cast<int>(m_edges.size());
    Edge e;
    e.id = id;
    e.from = fromId;
    e.to = toId;
    e.polyline = polyline;
    e.length = calcLength(polyline) * m_scale;
    m_edges.push_back(e);

    // --- 2. 路径构建 (保持不变) ---
    QPainterPath path;
    if (!polyline.empty()) {
        path.moveTo(polyline.front());
        for (size_t i = 1; i < polyline.size(); ++i) {
            path.lineTo(polyline[i]);
        }
    }

    // --- 3. 边样式美化 (方案二实现) ---
    // 创建一个非 const QPen 对象
    QPen edgePenNormal(QColor("#AAB8C2"), 3);

    // 现在可以修改它，设置圆帽样式
    edgePenNormal.setCapStyle(Qt::RoundCap);

    // --- 4. 图形层添加 (Item) ---
    // 注意：这里替换了旧的 addPath 调用，避免了 'item' 重定义错误
    auto *item = m_scene->addPath(path, edgePenNormal);

    item->setZValue(10);
    item->setData(kItemTypeKey, kTypeEdge);
    item->setData(kItemIdKey, id);

    // 保存映射：edgeId -> item* (保持不变)
    m_edgeItemById[id] = item;

    return id;
}

void page2_1_map_lab_guide::deleteNode(int nodeId)
{
    if (nodeId < 0 || nodeId >= static_cast<int>(m_nodes.size())) return;
    if (m_nodes[nodeId].id == -1) return;

    // 若该节点正在 hover，先清 hover（避免后续 restore 访问不存在）
    if (m_hoverNodeId == nodeId) {
        m_hoverNodeId = -1;
    }

    // 级联删除边（删除过程中也可能影响 hoverEdgeId，所以这里顺便清一下）
    for (int i = 0; i < static_cast<int>(m_edges.size()); ++i) {
        if (m_edges[i].id != -1) {
            if (m_edges[i].from == nodeId || m_edges[i].to == nodeId) {
                // 如果 hover 正在这条边上，先清掉
                if (m_hoverEdgeId == m_edges[i].id) m_hoverEdgeId = -1;
                deleteEdge(m_edges[i].id);
            }
        }
    }

    // 精准移除节点图元（label 是 circle 的子项，会自动删除）
    auto it = m_nodeItemById.find(nodeId);
    if (it != m_nodeItemById.end() && it.value() != nullptr) {
        QGraphicsEllipseItem *item = it.value();
        if (item->scene()) item->scene()->removeItem(item);
        delete item;
    }
    m_nodeItemById.remove(nodeId);

    // 数据层软删除
    m_nodes[nodeId].id = -1;
    m_nodes[nodeId].name = "[DELETED]";
    m_nodes[nodeId].pos = QPointF(-1e9, -1e9);
    m_nodes[nodeId].imagePathsAbs.clear();

    // 状态重置（避免画线/导引使用已删点）
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


#include <algorithm>  // std::remove

void page2_1_map_lab_guide::deleteEdge(int edgeId)
{
    if (edgeId < 0 || edgeId >= static_cast<int>(m_edges.size()))
        return;

    // 若该边正在 hover，先清 hover（避免后续 restore 访问不存在）
    if (m_hoverEdgeId == edgeId) {
        m_hoverEdgeId = -1;
    }

    // 若该边在高亮列表里，移除
    auto newEnd = std::remove(m_highlightEdgeIds.begin(), m_highlightEdgeIds.end(), edgeId);
    m_highlightEdgeIds.erase(newEnd, m_highlightEdgeIds.end());

    // 删除图元
    auto it = m_edgeItemById.find(edgeId);
    if (it != m_edgeItemById.end() && it.value() != nullptr) {
        QGraphicsPathItem *item = it.value();
        if (item->scene()) item->scene()->removeItem(item);
        delete item;
    }
    m_edgeItemById.remove(edgeId);

    // 数据层软删除
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
        if (n.pos.x() < -1e8) continue; // deleted
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
    // 把旧高亮恢复，但若当前 hover 正在绿着，别覆盖
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
    // 先清除旧高亮
    clearHighlight();

    // 记录新高亮
    m_highlightEdgeIds = edgeIds;

    // 设置红色（若被 hover，绿色优先，跳过即可）
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

// -------------------- Streetscape --------------------
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

        // 忽略 deleted node
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

    // 回溯路径
    for (int cur = endId; cur != -1; cur = prev[cur]) outNodePath.push_back(cur);
    std::reverse(outNodePath.begin(), outNodePath.end());

    outDist = dist[endId];
    return true;
}

void page2_1_map_lab_guide::computeAndShowShortestPath()
{
    if (m_startNodeId < 0 || m_endNodeId < 0) return;

    buildAdjacency();

    std::vector<int> nodePath;
    double dist = 0.0;
    if (!dijkstra(m_startNodeId, m_endNodeId, nodePath, dist)) {
        QMessageBox::warning(this, "不可到达", "起点与终点之间不存在可达路径。");
        clearHighlight();
        return;
    }

    // 把 nodePath 转成 edgeId 列表用于高亮
    std::vector<int> edgeIds;
    for (size_t i=1;i<nodePath.size();++i) {
        int a = nodePath[i-1];
        int b = nodePath[i];
        int eid = findEdgeIdBetween(a, b);
        if (eid >= 0) edgeIds.push_back(eid);
    }

    highlightEdges(edgeIds);
    QMessageBox::information(this, "最短路径",
                             QString("最短路径节点数：%1\n总距离：%2")
                                 .arg((int)nodePath.size())
                                 .arg(dist));

    emit mapModified();
}

// -------------------- Save project --------------------
QString page2_1_map_lab_guide::projectRootDir() const
{
    // 按指导风格：在程序当前工作目录创建 Map_guide
    // 你也可以改成 QStandardPaths::AppDataLocation 更规范
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
        // 没地图就不保存
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

    // 1) 保存背景图（copy到 map_image.png）
    {
        const QString dst = QDir::cleanPath(projDir + "/map_image.png");
        if (QFile::exists(dst)) QFile::remove(dst);
        if (!QFile::copy(m_mapImageAbsPath, dst)) {
            // 有些情况下 copy 失败（权限/路径），提示但不直接崩
            QMessageBox::warning(this, "保存失败", "背景图复制失败。");
            return false;
        }
    }

    // 2) 保存当前视图截图 map_view.png（含标注/高亮）
    {
        auto *gv = this->findChild<QGraphicsView*>("graphicsView");
        if (gv) {
            QPixmap shot = gv->grab();
            shot.save(QDir::cleanPath(projDir + "/map_view.png"));
        }
    }

    // 3) 保存 JSON：nodes + edges（街景写相对路径）
    QJsonObject rootObj;
    rootObj["map_id"] = mapId;
    rootObj["scale"] = m_scale;
    rootObj["map_image"] = "map_image.png"; // 相对路径

    QJsonArray nodesArr;
    for (const auto &n : m_nodes) {
        if (n.id < 0) continue;
        if (n.pos.x() < -1e8) continue; // deleted

        QJsonObject no;
        no["id"] = n.id;
        no["name"] = n.name;
        no["x"] = n.pos.x();
        no["y"] = n.pos.y();

        // streetscape：把绝对路径图片拷贝到 streetscape/Node{id}_{k}.ext
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

    // 写入文件 guide_data/graph.json
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




//悬停新增：
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
    // 画边拖拽时不做 hover（避免闪烁）
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

    // 同时只高亮一个：优先节点（更符合“删除节点不误删边”的体验）
    if (newNode != -1) newEdge = -1;

    // --- edge hover 切换 ---
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

    // --- node hover 切换 ---
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

    // 空白处：清 hover
    if (newNode == -1 && newEdge == -1) {
        clearHover();
    }
}

//新增，文档解析----------------------------------------------------
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

    // 必须确认背景加载成功，否则不要留下旧图/旧状态
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
        // 保持为空（已经 clear 过了），避免旧图残留
        return false;
    }
    return true;
}


void page2_1_map_lab_guide::clearProjectVisualAndData()
{
    // 1) 清空状态
    m_highlightEdgeIds.clear();
    m_mapImageAbsPath.clear();
    // 如果你做了 hover，建议也清：
    // clearHover();

    m_isDrawingEdge = false;
    m_edgeFromNodeId = -1;
    m_startNodeId = -1;
    m_endNodeId   = -1;

    // 2) 删除边图元
    for (auto it = m_edgeItemById.begin(); it != m_edgeItemById.end(); ++it) {
        if (it.value()) {
            if (it.value()->scene()) it.value()->scene()->removeItem(it.value());
            delete it.value();
        }
    }
    m_edgeItemById.clear();

    // 3) 删除节点图元（label 是 circle 的子项，会自动删）
    for (auto it = m_nodeItemById.begin(); it != m_nodeItemById.end(); ++it) {
        if (it.value()) {
            if (it.value()->scene()) it.value()->scene()->removeItem(it.value());
            delete it.value();
        }
    }
    m_nodeItemById.clear();

    // 关键：删除背景图（否则会残留旧图）
    if (m_bgItem) {
        if (m_scene) m_scene->removeItem(m_bgItem);
        delete m_bgItem;
        m_bgItem = nullptr;
    }

    // 4) 预览线清空（保留 item 但清空 path）
    if (m_previewPathItem) {
        m_previewPathItem->setPath(QPainterPath());
        m_previewPathItem->setVisible(false); // 可选：更干净
    }

    // 5) 清空数据层
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

    // 成功后同步状态：让 onStartLabel 不会误判
    m_mapImageAbsPath = QFileInfo(imageAbsPath).absoluteFilePath();
    return true;
}



static QPointF parsePoint2(const QJsonValue &v)
{
    // 期望格式：[x, y]
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

    // 1) 读取 scale / map_image
    m_scale = root.value("scale").toDouble(1.0);

    const QString mapImageRel = root.value("map_image").toString(); // e.g. "map_image.png"
    if (mapImageRel.isEmpty()) {
        // 存档没有背景图信息：直接判失败更合理（否则后续 onStartLabel 必然提示）
        return false;
    }

    const QString mapAbs = QDir(projectRoot).filePath(mapImageRel);

    // 关键：必须确保背景图加载成功，否则不要继续
    if (!setBackgroundImage(mapAbs)) {
        return false;
    }

    // 2) 读取 nodes：先找最大 id，resize vector 以保证 m_nodes[id] 可写
    const QJsonArray nodesArr = root.value("nodes").toArray();

    int maxNodeId = -1;
    for (const auto &nv : nodesArr) {
        const QJsonObject no = nv.toObject();
        maxNodeId = std::max(maxNodeId, no.value("id").toInt(-1));
    }
    if (maxNodeId >= 0) {
        m_nodes.resize(static_cast<size_t>(maxNodeId + 1));
        for (int i = 0; i <= maxNodeId; ++i) {
            m_nodes[i].id = -1; // 先标记无效，后面按 JSON 填充
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

        // streetscapes：JSON里通常是相对路径数组，运行时拼绝对路径
        // 你的示例里字段名就是 "streetscapes" :contentReference[oaicite:5]{index=5}
        const QJsonArray scArr = no.value("streetscapes").toArray();
        n.imagePathsAbs.clear();
        for (const auto &sv : scArr) {
            const QString rel = sv.toString();
            if (rel.isEmpty()) continue;
            n.imagePathsAbs.push_back(QDir(projectRoot).filePath(rel));
        }

        // 写入数据层（按 id 索引）
        if (id >= 0 && id < (int)m_nodes.size()) {
            m_nodes[id] = n;
        }

        // --- 绘制节点图元（复用你当前 addNodeAt 的“绘制方式”，但这里必须保留 id） ---
        const double r = 6.0;
        auto *circle = m_scene->addEllipse(n.pos.x()-r, n.pos.y()-r, 2*r, 2*r,
                                           QPen(Qt::black), QBrush(Qt::yellow));
        circle->setZValue(100);
        circle->setData(kItemTypeKey, kTypeNode);
        circle->setData(kItemIdKey, id);
        circle->setFlag(QGraphicsItem::ItemIsSelectable, true);

        m_nodeItemById[id] = circle;

        auto *label = m_scene->addText(QString::number(id));
        label->setParentItem(circle);
        label->setPos(n.pos.x() + 8, n.pos.y() - 10);
        label->setZValue(101);
        label->setData(kItemTypeKey, kTypeNode);
        label->setData(kItemIdKey, id);
    }

    // 3) 读取 edges：同样先 resize
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

        // polyline：[[x,y],[x,y],...] :contentReference[oaicite:6]{index=6}
        e.polyline.clear();
        const QJsonArray pl = eo.value("polyline").toArray();
        for (const auto &pv : pl) {
            e.polyline.push_back(parsePoint2(pv));
        }

        if (id >= 0 && id < (int)m_edges.size()) {
            m_edges[id] = e;
        }

        // --- 绘制边图元 ---
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

    // 4) 重建邻接表 + 通知
    buildAdjacency();
    emit mapModified();
    return true;
}

#ifndef PAGE2_1_MAP_LAB_GUIDE_H
#define PAGE2_1_MAP_LAB_GUIDE_H

#include <QWidget>
#include <QPointF>
#include <QString>
#include <vector>
#include <QGraphicsSceneMouseEvent>

//修正高亮引入
#include <QHash>
#include <QPointer>
#include <QPen>



class QGraphicsScene;
class QGraphicsPixmapItem;
class QGraphicsPathItem;
class QGraphicsEllipseItem;
class StreetViewDialog;

namespace Ui {
class page2_1_map_lab_guide;

}

class page2_1_map_lab_guide : public QWidget
{
    Q_OBJECT
public:
    explicit page2_1_map_lab_guide(QWidget *parent = nullptr);
    ~page2_1_map_lab_guide() override;

    enum class AppState {
        IDLE,       // 已加载/未加载地图，但未开始标注/导引
        LABELING,   // 标注节点与道路
        GUIDING     // 选择起终点并计算最短路
    };

    struct Node {
        int id = -1;
        QString name;                // 可选
        QPointF pos;                 // Scene 坐标
        std::vector<QString> imagePathsAbs; // 运行时绝对路径（街景图）
    };

    struct Edge {
        int id = -1;
        int from = -1;
        int to = -1;
        std::vector<QPointF> polyline; // 折线点（Scene 坐标）
        double length = 0.0;           // 像素长度（或 scale 后）
    };

signals:
    void backToMenu();  // 给 MainWindow：切回 page2_operate_select
    void mapModified(); // 数据改变（可用于自动保存）

public slots:
    // 供按钮连接
    void onBack();
    void onLoadMap();
    void onStartLabel();
    void onEndLabel();
    void onStartGuide();
    void onEndGuide();

public:
    // 公共接口：外部也可调用
    bool loadMapImage(const QString &imagePath);
    void clearAll(); // 清空当前工程（不删除硬盘数据）
    // MainWindow 统一调用：isNewProject=true 传图片路径；false 传项目目录
    bool loadProject(const QString &path, bool isNewProject);

private:
    Ui::page2_1_map_lab_guide *ui = nullptr;

    // ---------- Scene ----------
    QGraphicsScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_bgItem = nullptr;

    // 预览线（画折线时显示）
    QGraphicsPathItem *m_previewPathItem = nullptr;

    // ---------- Data ----------
    std::vector<Node> m_nodes;
    std::vector<Edge> m_edges;

    // adjacency for Dijkstra
    struct AdjEdge {
        int to;
        double w;
        int edgeId;
    };
    std::vector<std::vector<AdjEdge>> m_adj;

    // ---------- State ----------
    AppState m_state = AppState::IDLE;
    QString m_mapImageAbsPath;   // 当前底图绝对路径（导入时选的）
    double m_scale = 1.0;        // 可选：像素->真实距离缩放（先默认1）

    // 标注：画边过程
    bool m_isDrawingEdge = false;
    int m_edgeFromNodeId = -1;
    std::vector<QPointF> m_currentPolyline;

    // 导引：选点过程
    int m_startNodeId = -1;
    int m_endNodeId   = -1;

    // 记录当前高亮的 edgeId
    std::vector<int> m_highlightEdgeIds;

    //修正高亮引入：
    // ID -> 图元指针映射（只存“真实边”的 item，不存高亮覆盖层）
    //修正3QHash<int, QPointer<QGraphicsPathItem>> m_edgeItemById;
    QHash<int, QGraphicsPathItem*> m_edgeItemById;
    QHash<int, QGraphicsEllipseItem*> m_nodeItemById;
    // 可选：节点也可以类似映射（以后拖拽/改色用）
    // QHash<int, QPointer<QGraphicsEllipseItem>> m_nodeItemById;


    //新增如下（）悬停（）

    // hover 目标（同一时刻只允许一个：node 或 edge）
    int m_hoverNodeId = -1;
    int m_hoverEdgeId = -1;

    // 线条样式：普通/最短路/悬停
    QPen m_edgePenNormal    = QPen(Qt::darkGray, 2);
    QPen m_edgePenHighlight = QPen(Qt::red, 4);
    QPen m_edgePenHover     = QPen(Qt::green, 3);

    // 节点样式：普通/悬停
    QPen   m_nodePenNormal  = QPen(Qt::black, 1);
    QBrush m_nodeBrushNormal = QBrush(Qt::yellow);
    QPen   m_nodePenHover   = QPen(Qt::green, 2);
    QBrush m_nodeBrushHover  = QBrush(Qt::green);

    // hover 更新函数
    void updateHoverAt(const QPointF &scenePos);
    void clearHover();

    // 辅助：恢复某条边/节点为“正确”的颜色（边要考虑是否最短路红色）
    void restoreEdgeStyle(int edgeId);
    void restoreNodeStyle(int nodeId);

    bool isEdgeHighlighted(int edgeId) const;



private:
    // ---------- UI helpers ----------
    void ensureGraphicsView();      // 如果 .ui 没放 graphicsView，就程序创建一个
    void initScene();
    void bindButtonsByObjectName(); // 用 findChild 绑定按钮（弱依赖 .ui）
    void updateUiStateHint();       // 可选：根据状态更新窗口标题/状态提示

    // ---------- Interaction core ----------
    void handleSceneLeftPress(const QPointF &scenePos);
    void handleSceneLeftMove(const QPointF &scenePos);
    void handleSceneLeftRelease(const QPointF &scenePos);
    void handleSceneRightClick(const QPointF &scenePos);

    // ---------- Model ops ----------
    int addNodeAt(const QPointF &scenePos, const QString &name = QString());
    int addEdge(int fromId, int toId, const std::vector<QPointF> &polyline);
    void deleteNode(int nodeId);
    void deleteEdge(int edgeId);

    int findNearestNodeId(const QPointF &scenePos, double maxDistPx = 25.0) const;
    int findEdgeIdBetween(int a, int b) const;

    static double calcLength(const std::vector<QPointF> &polyline);

    // ---------- Graphics ops ----------
    void redrawAll();                 // 重新绘制节点与边（当删除/加载等）
    void clearHighlight();
    void highlightEdges(const std::vector<int> &edgeIds);
    void updatePreviewPath();         // 更新预览折线路径

    // ---------- Dijkstra ----------
    void buildAdjacency();
    bool dijkstra(int startId, int endId, std::vector<int> &outNodePath, double &outDist);
    void computeAndShowShortestPath();

    //新增联通性检查
    bool isConnectedBfs(int startId, int endId);


    // ---------- Streetscape images ----------
    void addStreetImageToNode(int nodeId);

    // ---------- Save/Load project (JSON) ----------
    bool saveCurrentProject();        // 保存到 Map_guide/Saved_Map#N
    int  allocateNextMapId(const QString &rootDirAbs) const;
    QString projectRootDir() const;   // Map_guide 目录（绝对路径）

    //新增
    void showStreetViewDialog(int nodeId); // 弹出对话框的逻辑

    //新增：存档解析
    bool loadNewImageOnly(const QString &imageAbsPath);
    bool loadExistingProjectDir(const QString &projectRoot);

    bool loadGraphJson(const QString &jsonPath, const QString &projectRoot);
    void clearProjectVisualAndData();     // 清空当前图元/数据（不重建UI）
    bool setBackgroundImage(const QString &imageAbsPath);

};

#endif // PAGE2_1_MAP_LAB_GUIDE_H

#ifndef PAGE2_2_MAP_SELECT_H
#define PAGE2_2_MAP_SELECT_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class map_select; }
QT_END_NAMESPACE

class QListWidgetItem;

class page2_2_map_select : public QWidget
{
    Q_OBJECT
public:
    explicit page2_2_map_select(QWidget *parent = nullptr);
    ~page2_2_map_select();

signals:
    // 统一入口：path + isNewProject
    void requestOpenProject(const QString &path, bool isNewProject);
    void backToMenu(); // 返回功能选择页

private slots:
    void onBackClicked();
    void onImportClicked();
    void onRefreshClicked();
    void onItemDoubleClicked(QListWidgetItem *item);

private:
    void initUi();
    void reloadProjectList();

    // 返回 Map_guide 根目录（默认：<appDir>/Map_guide）
    QString mapGuideRootDir() const;

    // 解析 Saved_Map#xxx -> id（失败返回 -1）
    int parseSavedMapId(const QString &folderName) const;

    // 找项目缩略图（优先 map_view.png，其次 map_image.png，找不到返回空串）
    QString findPreviewImage(const QString &projectRoot) const;

private:
    Ui::map_select *ui = nullptr;
};

#endif // PAGE2_2_MAP_SELECT_H

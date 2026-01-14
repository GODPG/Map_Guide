#ifndef PAGE1_LOGIN_H
#define PAGE1_LOGIN_H

#include <QWidget>

namespace Ui {
class page1_login;
}

class page1_login : public QWidget
{
    Q_OBJECT

public:
    explicit page1_login(QWidget *parent = nullptr);
    ~page1_login();

signals:
    // 登录成功信号，由 MainWindow 捕获并切换页面
    void loginSuccess(const QString &username);

    // 注册请求（后续可在 MainWindow 里打开注册对话框）
    void requestRegister();

private slots:
    void onLoginButtonClicked();
    void onRegisterButtonClicked();

private:
    Ui::page1_login *ui;

    // 简单的账号校验逻辑（当前用作示例）
    bool checkCredentials(const QString &username, const QString &password);
};

#endif // PAGE1_LOGIN_H

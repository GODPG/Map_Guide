#ifndef PAGE1_LOGIN_H
#define PAGE1_LOGIN_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>

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
    void loginSuccess(const QString &username);
    void requestRegister();

private slots:
    void onLoginButtonClicked();
    void onRegisterButtonClicked();
    void onSendCodeClicked();

    void updateCountdown();

private:
    Ui::page1_login *ui;
    QNetworkAccessManager *manager;

    QTimer *m_timer;
    int m_countdown;
};

#endif

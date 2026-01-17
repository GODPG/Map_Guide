#include "page1_login.h"
#include "ui_page1_login.h"

#include <QMessageBox>
#include <QDebug>

#include <QRegularExpressionValidator>
#include <QRegularExpression>


const QString SERVER_URL = "http://127.0.0.1:5000";
// const QString SERVER_URL = "http://172.27.187.51:5000";

page1_login::page1_login(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::page1_login)
{
    ui->setupUi(this);

    // --- 1. 基础 UI 设置 ---
    ui->password->setPlaceholderText("请输入验证码");
    ui->username->setPlaceholderText("请输入手机号");

    ui->password->setEchoMode(QLineEdit::Normal);

    // --- 2. 初始化网络管理器 ---
    manager = new QNetworkAccessManager(this);

    // --- 3. 按钮信号连接 ---
    connect(ui->page1_login_push_Botton, &QPushButton::clicked,
            this, &page1_login::onLoginButtonClicked);

    connect(ui->page1_login_register, &QPushButton::clicked,
            this, &page1_login::onRegisterButtonClicked);

    connect(ui->btn_get_code, &QPushButton::clicked,
            this, &page1_login::onSendCodeClicked);


    // 在验证码框按回车
    connect(ui->password, &QLineEdit::returnPressed, this, &page1_login::onLoginButtonClicked);
    // (可选) 在手机号框按回车 -> 触发登录
    connect(ui->username, &QLineEdit::returnPressed, this, &page1_login::onLoginButtonClicked);

    // 初始化定时器
    m_timer = new QTimer(this);
    m_countdown = 60;
    connect(m_timer, &QTimer::timeout, this, &page1_login::updateCountdown);

    //6. 输入校验

    // 限制手机号：只能输入 1 开头，第二位 3-9，共 11 位数字
    QRegularExpression rxPhone("^1[3-9]\\d{9}$");
    QValidator *phoneValidator = new QRegularExpressionValidator(rxPhone, this);
    ui->username->setValidator(phoneValidator);

    // 限制验证码：只能输入 4 位数字
    QRegularExpression rxCode("^\\d{4}$");
    QValidator *codeValidator = new QRegularExpressionValidator(rxCode, this);
    ui->password->setValidator(codeValidator);
}

page1_login::~page1_login()
{
    delete ui;
}


void page1_login::updateCountdown()
{
    m_countdown--;
    if (m_countdown <= 0) {
        m_timer->stop();
        ui->btn_get_code->setText("获取验证码");
        ui->btn_get_code->setEnabled(true);
    } else {
        ui->btn_get_code->setText(QString::number(m_countdown) + " 秒后重试");
    }
}


void page1_login::onSendCodeClicked()
{
    QString phone = ui->username->text().trimmed();

    if (phone.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入手机号");
        return;
    }
    // 简单的长度校验（虽然正则已经限制了输入，但防御性编程更好）
    if (phone.length() != 11) {
        QMessageBox::warning(this, "提示", "请输入完整的11位手机号");
        return;
    }

    // --- 启动倒计时 ---
    m_countdown = 60;
    ui->btn_get_code->setEnabled(false);
    ui->btn_get_code->setText("60 秒后重试");
    m_timer->start(1000);

    // --- 发送请求 ---
    QString urlStr = SERVER_URL + "/api/get_code";
    QNetworkRequest request((QUrl(urlStr)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json.insert("phone", phone);

    // 发送 POST
    QNetworkReply *reply = manager->post(request, QJsonDocument(json).toJson());

    // 处理响应
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "Server Response:" << response;
            QMessageBox::information(this, "发送成功", "验证码已发送，请查看服务器控制台");
        } else {
            // 网络错误：停止倒计时，允许重试
            m_timer->stop();
            ui->btn_get_code->setText("获取验证码");
            ui->btn_get_code->setEnabled(true);

            QMessageBox::critical(this, "网络错误", "连接服务器失败：" + reply->errorString());
        }
        reply->deleteLater();
    });
}

// --- 点击“登录” ---
void page1_login::onLoginButtonClicked()
{
    const QString phone = ui->username->text().trimmed();
    const QString code  = ui->password->text().trimmed();

    if (phone.isEmpty() || code.isEmpty()) {
        QMessageBox::warning(this, "提示", "手机号和验证码不能为空");
        return;
    }

    if (code == "1111") {
        qDebug() << "检测到测试验证码，跳过服务器验证...";

        // 可选：弹个窗提示一下（正式上线前删掉这行）
        // QMessageBox::information(this, "测试模式", "欢迎！已使用万能验证码登录。");

        // 直接发送登录成功信号
        emit loginSuccess(phone);

        // 【关键】直接 return，不执行后面的网络请求代码
        return;
    }

    // --- 【优化】Loading 状态：锁定界面 ---
    this->setCursor(Qt::WaitCursor);       // 鼠标变沙漏
    ui->page1_login_push_Botton->setEnabled(false);
    ui->page1_login_push_Botton->setText("登录中...");
    ui->username->setEnabled(false);       // 锁定输入框
    ui->password->setEnabled(false);

    // --- 发送请求 ---
    QString urlStr = SERVER_URL + "/api/login";
    QNetworkRequest request((QUrl(urlStr)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json.insert("phone", phone);
    json.insert("code", code);

    QNetworkReply *reply = manager->post(request, QJsonDocument(json).toJson());

    // 处理响应
    connect(reply, &QNetworkReply::finished, [=]() {

        // --- 【优化】恢复界面状态 ---
        this->setCursor(Qt::ArrowCursor);  // 恢复鼠标
        ui->page1_login_push_Botton->setEnabled(true);
        ui->page1_login_push_Botton->setText("登 录");
        ui->username->setEnabled(true);    // 解锁输入框
        ui->password->setEnabled(true);

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonObject respObj = QJsonDocument::fromJson(response).object();

            QString status = respObj["status"].toString();
            QString msg = respObj["msg"].toString();

            if (status == "success") {
                QString token = respObj["token"].toString();
                qDebug() << "登录成功，Token:" << token;
                emit loginSuccess(phone); // 发送信号切换页面
            } else {
                QMessageBox::warning(this, "登录失败", msg);
            }
        } else {
            QMessageBox::critical(this, "网络错误", reply->errorString());
        }
        reply->deleteLater();
    });
}

void page1_login::onRegisterButtonClicked()
{
    emit requestRegister();
    QMessageBox::information(this, "提示", "手机验证码登录模式下，新用户将自动注册。");
}

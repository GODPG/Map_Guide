#include "page1_login.h"
#include "ui_page1_login.h"

#include <QMessageBox>

page1_login::page1_login(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::page1_login)
{
    ui->setupUi(this);

    // 设置密码框为密码模式
    ui->password->setEchoMode(QLineEdit::Password);

    // 连接按钮信号到槽
    connect(ui->page1_login_push_Botton, &QPushButton::clicked,
            this, &page1_login::onLoginButtonClicked);

    connect(ui->page1_login_register, &QPushButton::clicked,
            this, &page1_login::onRegisterButtonClicked);
}

page1_login::~page1_login()
{
    delete ui;
}

bool page1_login::checkCredentials(const QString &username, const QString &password)
{
    // TODO：这里先简单判断非空，后续可以替换为真实的账号验证逻辑
    return !username.isEmpty() && !password.isEmpty();
}

void page1_login::onLoginButtonClicked()
{
    const QString user = ui->username->text().trimmed();
    const QString pwd  = ui->password->text();

    if (!checkCredentials(user, pwd)) {
        QMessageBox::warning(this,
                             tr("登录失败"),
                             tr("用户名或密码不能为空（当前为示例校验）"));
        return;
    }

    // 校验通过，发出登录成功信号
    emit loginSuccess(user);
}

void page1_login::onRegisterButtonClicked()
{
    // 暂时只发信号，由 MainWindow 决定如何处理（打开注册页/弹窗等）
    emit requestRegister();

    // 也可以先给出一个占位提示
    QMessageBox::information(this,
                             tr("提示"),
                             tr("注册功能尚未实现（这里是占位）。"));
}

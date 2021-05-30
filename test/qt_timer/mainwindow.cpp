#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <sstream>
#include <thread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , timerHolder_()
{
    ui->setupUi(this);

    auto updateText = [=](int sourceLine) {
        std::stringstream os;
        os << "thread " << std::this_thread::get_id() << ", source line = " << sourceLine;
        ui->label->setText(QString::fromStdString(os.str()));
    };

    // delay tasks
    timerHolder_.yield().then([=]() {
        updateText(__LINE__);
        return timerHolder_.delay(1000);
    }).then([=]() {
        updateText(__LINE__);
        return timerHolder_.delay(1000);
    }).then([=]() {
        updateText(__LINE__);
        return timerHolder_.delay(1000);
    }).then([=]() {
        updateText(__LINE__);
        return timerHolder_.delay(1000);
    }).then([=]() {
        updateText(__LINE__);
        return timerHolder_.delay(1000);
    }).then([=]() {
        updateText(__LINE__);
        return timerHolder_.delay(1000);
    }).then([=]() {
        updateText(__LINE__);
        return timerHolder_.delay(1000);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}


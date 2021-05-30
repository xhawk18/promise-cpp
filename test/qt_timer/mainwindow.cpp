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
        os << "thread id   = " << std::this_thread::get_id() << "\n"
           << "source line = " << sourceLine;
        ui->label->setText(QString::fromStdString(os.str()));
    };
    updateText(__LINE__);

    // delay tasks
    using namespace promise;
    doWhile([=](Defer d) {
        timerHolder_.delay(1000).then([=]() {
            updateText(__LINE__);
            return timerHolder_.delay(1000);
        }).then([=]() {
            updateText(__LINE__);
            return timerHolder_.delay(1000);
        }).then([=]() {
            updateText(__LINE__);
            return timerHolder_.yield();
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
        }).then(d); //call doWhile
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}


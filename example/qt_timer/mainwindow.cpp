#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <sstream>
#include <thread>

using namespace promise;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
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
#if 1
    doWhile([=](DeferLoop &loop) {
        delay(1000).then([=]() {
            updateText(__LINE__);
            return delay(1000);
        }).then([=]() {
            updateText(__LINE__);
            return delay(1000);
        }).then([=]() {
            updateText(__LINE__);
            return yield();
        }).then([=]() {
            updateText(__LINE__);
            return delay(1000);
        }).then([=]() {
            updateText(__LINE__);
            return delay(1000);
        }).then([=]() {
            updateText(__LINE__);
            return delay(1000);
        }).then([=]() {
            updateText(__LINE__);
            return delay(1000);
        }).then(loop); //call doWhile
    });
#endif

}

MainWindow::~MainWindow()
{
    delete ui;
}


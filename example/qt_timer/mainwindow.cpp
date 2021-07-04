#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <sstream>
#include <thread>

using namespace promise;
template<typename FUNC>
inline any anyCall2 (const FUNC &func, const any &arg) {
    const auto &stdFunc = call_traits<FUNC>::to_std_function(func);


    printf("%s\n%s\n%s\n",
        typeid(stdFunc).name(),
        typeid(call_traits<FUNC>::result_type).name(),
        typeid(call_traits<FUNC>::argument_type).name());

    if (!stdFunc)
        return nullptr;
    else
        return stdFuncCall<typename call_traits<FUNC>::result_type>(stdFunc, arg);
}


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
#if 1
    doWhile([=](LoopCallback &cb) {
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
        }).then(cb); //call doWhile
    });
#endif

}

MainWindow::~MainWindow()
{
    delete ui;
}


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <promise-cpp/add_ons/qt/promise_qt.hpp>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;   
};
#endif // MAINWINDOW_H

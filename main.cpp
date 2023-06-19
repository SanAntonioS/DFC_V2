#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle("分布式流动量热仪测控系统 V2");
    w.show();
    return a.exec();
}

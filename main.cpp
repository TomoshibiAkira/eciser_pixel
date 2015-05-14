#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    if (argc == 2 && strcmp(argv[1], "--debug") == 0) w.setRasterHandlerDebugOn();
    w.show();

    return a.exec();
}

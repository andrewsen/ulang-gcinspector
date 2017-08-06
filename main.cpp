#include "mainwindow.h"
#include <QApplication>
#include <csignal>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    if(w.getPid() != -1)
        kill(w.getPid(), SIGKILL);

    return a.exec();
}

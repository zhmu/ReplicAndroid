#include "ReplicAndroid.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ReplicAndroid w;
    w.show();
    return a.exec();
}

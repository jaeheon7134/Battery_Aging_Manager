#include "battery_aging.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Battery_Aging w;
    w.show();
    return QCoreApplication::exec();
}

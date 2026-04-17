#include "battery_aging.h"
#include "UI_Styling.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    UI_Styling::applyStyle(a);

    Battery_Aging w;
    w.show();

    return a.exec();
}
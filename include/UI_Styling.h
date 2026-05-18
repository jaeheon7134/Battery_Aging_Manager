#ifndef UI_STYLING_H
#define UI_STYLING_H

#include <QApplication>
#include <QWidget>

class UI_Styling
{
public:
    static void applyStyle(QApplication &app);
    static void applyShadow(QWidget* parent);
};

#endif
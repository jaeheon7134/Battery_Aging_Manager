#ifndef UI_STYLING_H
#define UI_STYLING_H

#include <QWidget>

class UI_Styling
{
public:
    static void applyShadow(QWidget* parent);
    static void applyStyle(QApplication &app);
};

#endif // UI_STYLING_H
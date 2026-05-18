#include "live_page.h"

#include "ui_battery_aging.h"

Live_Page::Live_Page(Ui::Battery_Aging *ui)
    : ui(ui)
{
}

void Live_Page::init()
{
    (void)ui;
}

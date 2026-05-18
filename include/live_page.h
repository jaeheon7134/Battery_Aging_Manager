#ifndef LIVE_PAGE_H
#define LIVE_PAGE_H

namespace Ui { class Battery_Aging; }

class Live_Page
{
public:
    explicit Live_Page(Ui::Battery_Aging *ui);
    void init();

private:
    Ui::Battery_Aging *ui = nullptr;
};

#endif

#ifndef PLAN_PAGE_H
#define PLAN_PAGE_H

#include <QDate>
#include <QJsonObject>
#include <QMap>
namespace Ui { class Battery_Aging; }

class Plan_Page
{
public:
    explicit Plan_Page(Ui::Battery_Aging *ui);

    void init();
    void saveMemo();

private:
    Ui::Battery_Aging *ui = nullptr;

    QMap<QString, QJsonObject> noteMap;
    QDate selectedDate;
    int targetHours = 0;

    void onDateSelected(const QDate &date);
    void updateCalendarMarks();
    void saveToJson();
    void loadFromJson();
    void loadConfig();
    int calculateTotalHours();
    void updateCurrentTime();
};

#endif

#ifndef BATTERY_AGING_H
#define BATTERY_AGING_H

#include <QMainWindow>
#include <QMap>
#include <QDate>
#include <QJsonObject>

QT_BEGIN_NAMESPACE
namespace Ui { class Battery_Aging; }
QT_END_NAMESPACE

class Battery_Aging : public QMainWindow
{
    Q_OBJECT

public:
    Battery_Aging(QWidget *parent = nullptr);
    ~Battery_Aging();

private slots:
    void onDateSelected(const QDate &date);
    void on_pushButton_save_clicked();

private:
    Ui::Battery_Aging *ui;

    QMap<QString, QJsonObject> noteMap;
    QDate selectedDate;

    int targetHours = 0;
    int calculateTotalHours();

    void saveToJson();
    void loadFromJson();
    void loadConfig();
    void updateCurrentTime();
};

#endif
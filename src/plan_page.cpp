#include "plan_page.h"

#include "ui_battery_aging.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QTextCharFormat>

Plan_Page::Plan_Page(Ui::Battery_Aging *ui)
    : ui(ui)
{
}

void Plan_Page::init()
{
    QObject::connect(ui->calendarWidget, &QCalendarWidget::clicked,
                     [=](const QDate &date) { onDateSelected(date); });

    loadFromJson();
    updateCalendarMarks();
    loadConfig();
    updateCurrentTime();
}

void Plan_Page::saveMemo()
{
    if (!selectedDate.isValid())
        return;

    const QString key = selectedDate.toString("yyyy-MM-dd");

    QJsonObject dayObj;
    dayObj["note"] = ui->Memo_box->toPlainText();
    dayObj["cycle"] = ui->Cycle_time->text().toInt();
    dayObj["aging_hours"] = ui->Aging_time->text().toInt();

    noteMap[key] = dayObj;

    saveToJson();
    updateCurrentTime();
    updateCalendarMarks();
}

void Plan_Page::onDateSelected(const QDate &date)
{
    selectedDate = date;

    const QString key = date.toString("yyyy-MM-dd");

    if (noteMap.contains(key))
    {
        const QJsonObject dayObj = noteMap[key];

        ui->Memo_box->setPlainText(dayObj["note"].toString());
        ui->Cycle_time->setText(QString::number(dayObj["cycle"].toInt()));
        ui->Aging_time->setText(QString::number(dayObj["aging_hours"].toInt()));
    }
    else
    {
        ui->Memo_box->clear();
        ui->Cycle_time->clear();
        ui->Aging_time->clear();
    }
}

void Plan_Page::updateCalendarMarks()
{
    ui->calendarWidget->setDateTextFormat(QDate(), QTextCharFormat());

    for (auto it = noteMap.begin(); it != noteMap.end(); ++it)
    {
        const QDate date = QDate::fromString(it.key(), "yyyy-MM-dd");
        if (!date.isValid())
            continue;

        const QJsonObject obj = it.value();

        if (!obj.contains("aging_hours"))
            continue;

        const int aging = obj["aging_hours"].toInt();
        if (aging == 0)
            continue;

        QTextCharFormat format;
        if (aging < 8)
            format.setBackground(QColor(255, 100, 100, 60));
        else
            format.setBackground(QColor(100, 180, 255, 50));

        ui->calendarWidget->setDateTextFormat(date, format);
    }
}

void Plan_Page::saveToJson()
{
    QJsonObject root;

    for (auto it = noteMap.begin(); it != noteMap.end(); ++it)
        root[it.key()] = it.value();

    const QString base = QCoreApplication::applicationDirPath();
    const QString path = base + "/schedule.json";

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qDebug() << "save fail:" << path;
        return;
    }

    const QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();
}

void Plan_Page::loadFromJson()
{
    const QString base = QCoreApplication::applicationDirPath();
    QFile file(base + "/schedule.json");

    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    const QJsonObject root = doc.object();

    for (const QString &key : root.keys())
        noteMap[key] = root[key].toObject();

    file.close();
}

void Plan_Page::loadConfig()
{
    const QString base = QCoreApplication::applicationDirPath();
    QFile file(base + "/config.json");

    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    targetHours = obj["goal"].toObject()["target_hours"].toInt();

    ui->goal_time->setText(QString("%1 h").arg(targetHours));
    file.close();
}

int Plan_Page::calculateTotalHours()
{
    int total = 0;

    for (auto it = noteMap.begin(); it != noteMap.end(); ++it)
    {
        const QJsonObject obj = it.value();
        total += obj["aging_hours"].toInt();
    }

    return total;
}

void Plan_Page::updateCurrentTime()
{
    const int total = calculateTotalHours();
    ui->Current_time->setText(QString("%1 h").arg(total));

    if (targetHours > 0)
    {
        const int progress = (total * 100) / targetHours;
        ui->progressBar->setValue(progress);
    }
}

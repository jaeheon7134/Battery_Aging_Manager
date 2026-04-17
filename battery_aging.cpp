#include "battery_aging.h"
#include "./ui_battery_aging.h"
#include "UI_Styling.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

Battery_Aging::Battery_Aging(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Battery_Aging)
{
    ui->setupUi(this);
    UI_Styling::applyShadow(this);   // ← 한 줄로 끝

    ui->textEdit->setPlaceholderText("메모를 입력하세요");

    connect(ui->calendarWidget, &QCalendarWidget::clicked,
            this, &Battery_Aging::onDateSelected);

    loadFromJson();
    loadConfig();
    updateCurrentTime();
}

int Battery_Aging::calculateTotalHours()
{
    int total = 0;

    for (auto it = noteMap.begin(); it != noteMap.end(); ++it)
    {
        QJsonObject obj = it.value();
        total += obj["aging_hours"].toInt();
    }

    return total;
}


void Battery_Aging::updateCurrentTime()
{
    int total = calculateTotalHours();

    ui->Current_time->setText(QString("%1 h").arg(total));

    // progress 계산
    if (targetHours > 0)
    {
        int progress = (total * 100) / targetHours;
        ui->progressBar->setValue(progress);
    }
}


Battery_Aging::~Battery_Aging()
{
    delete ui;
}

// 날짜 선택
void Battery_Aging::onDateSelected(const QDate &date)
{
    selectedDate = date;

    QString key = date.toString("yyyy-MM-dd");

    if (noteMap.contains(key))
    {
        QJsonObject dayObj = noteMap[key];

        ui->textEdit->setPlainText(dayObj["note"].toString());
        ui->Cycle_time->setText(QString::number(dayObj["cycle"].toInt()));
        ui->Aging_time->setText(QString::number(dayObj["aging_hours"].toInt()));
    }
    else
    {
        ui->textEdit->clear();
        ui->Cycle_time->clear();
        ui->Aging_time->clear();
    }
}

// 저장
void Battery_Aging::on_pushButton_save_clicked()
{
    if (!selectedDate.isValid())
        return;

    QString key = selectedDate.toString("yyyy-MM-dd");

    QJsonObject dayObj;
    dayObj["note"] = ui->textEdit->toPlainText();
    dayObj["cycle"] = ui->Cycle_time->text().toInt();
    dayObj["aging_hours"] = ui->Aging_time->text().toInt();

    noteMap[key] = dayObj;

    saveToJson();
    updateCurrentTime();
}

// JSON 저장
void Battery_Aging::saveToJson()
{
    QJsonObject root;

    for (auto it = noteMap.begin(); it != noteMap.end(); ++it)
        root[it.key()] = it.value();

    QString base = QCoreApplication::applicationDirPath();
    QString path = base + "/schedule.json";
    QFile file(path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qDebug() << "save fail:" << path;   
        return;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();

    qDebug() << "saved to:" << path;
}
// JSON 로드
void Battery_Aging::loadFromJson()
{
    QString base = QCoreApplication::applicationDirPath();
    QFile file(base + "/schedule.json");   

    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();

    for (auto key : root.keys())
        noteMap[key] = root[key].toObject();

    file.close();
}

// config 로드
void Battery_Aging::loadConfig()
{
    QString base = QCoreApplication::applicationDirPath();
    QFile file(base + "/config.json");   

    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "config load fail";
        return;
    }

    QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();

    targetHours = obj["goal"].toObject()["target_hours"].toInt();

    ui->goal_time->setText(QString("%1 h").arg(targetHours));

    file.close();
}
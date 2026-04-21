#include "battery_aging.h"
#include "./ui_battery_aging.h"
#include "UI_Styling.h"

#include <QFile>
#include <QJsonDocument>
#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include <QSignalBlocker>
#include <QStringConverter>
#include <QTextStream>
#include <functional>
#include <QVBoxLayout>
#include <QLayout>
#include <QPainter>
#include <QFont>
#include <limits>
#include <cmath>

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>



class ChartViewEx : public QChartView
{
public:
    using QChartView::QChartView;

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        if (event->angleDelta().y() > 0)
            chart()->zoom(1.2);
        else
            chart()->zoom(0.8);

        event->accept();
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        chart()->zoomReset();
        QChartView::mouseDoubleClickEvent(event);
    }
};


// ================= Plan_Page =================

Plan_Page::Plan_Page(Ui::Battery_Aging *ui)
{
    this->ui = ui;
}

void Plan_Page::init()
{
    QObject::connect(ui->calendarWidget, &QCalendarWidget::clicked, [=](const QDate &date) { onDateSelected(date); });

    loadFromJson();
    updateCalendarMarks();
    loadConfig();
    updateCurrentTime();
}

void Plan_Page::saveMemo()
{
    if (!selectedDate.isValid())
        return;

    QString key = selectedDate.toString("yyyy-MM-dd");

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

    QString key = date.toString("yyyy-MM-dd");

    if (noteMap.contains(key))
    {
        QJsonObject dayObj = noteMap[key];

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
        QDate date = QDate::fromString(it.key(), "yyyy-MM-dd");
        if (!date.isValid()) continue;

        QJsonObject obj = it.value();

        if (!obj.contains("aging_hours"))
            continue;

        int aging = obj["aging_hours"].toInt();

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
}

void Plan_Page::loadFromJson()
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

void Plan_Page::loadConfig()
{
    QString base = QCoreApplication::applicationDirPath();
    QFile file(base + "/config.json");

    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();

    targetHours = obj["goal"].toObject()["target_hours"].toInt();

    ui->goal_time->setText(QString("%1 h").arg(targetHours));

    file.close();
}

int Plan_Page::calculateTotalHours()
{
    int total = 0;

    for (auto it = noteMap.begin(); it != noteMap.end(); ++it)
    {
        QJsonObject obj = it.value();
        total += obj["aging_hours"].toInt();
    }

    return total;
}

void Plan_Page::updateCurrentTime()
{
    int total = calculateTotalHours();

    ui->Current_time->setText(QString("%1 h").arg(total));

    if (targetHours > 0)
    {
        int progress = (total * 100) / targetHours;
        ui->progressBar->setValue(progress);
    }
}


// ================= 구현 =================

Log_Analyize::Log_Analyize(Ui::Battery_Aging *ui)
{
    this->ui = ui;
}

void Log_Analyize::init()
{
    QObject::connect(ui->CSV_Load_Btn, &QPushButton::clicked,
                     [=]() { openCSV(); });

    QObject::connect(ui->CSV_Folder_Load_Btn, &QPushButton::clicked,
                     [=]() { openCSVFolder(); });

    QObject::connect(ui->File_List, &QListWidget::itemClicked,
                     [=](QListWidgetItem *item) {

                         if (item->checkState() == Qt::Checked)
                             item->setCheckState(Qt::Unchecked);
                         else
                             item->setCheckState(Qt::Checked);
                     });

    QObject::connect(ui->File_List, &QListWidget::itemChanged,
                     [=](QListWidgetItem *item) {

                         QString name = item->text();

                         if (item->checkState() == Qt::Checked)
                             selectedFiles.insert(name);
                         else
                             selectedFiles.remove(name);

                         updateGraph();
                     });

    QObject::connect(ui->Graph_Clear_Btn, &QPushButton::clicked,
                     [=]() { clearGraph(); });

    // 조건 활성화 체크박스 → groupBox_Log_Condition enable/disable + 리스트 필터
    ui->groupBox_Log_Condition->setEnabled(ui->CBox_Condition_ONOFF->isChecked());
    QObject::connect(ui->CBox_Condition_ONOFF, &QCheckBox::toggled,
                     [=](bool checked) {
                         ui->groupBox_Log_Condition->setEnabled(checked);
                         updateListFilter();
                         updateGraph();
                     });

    // 온도 슬라이더 → label_7 에 현재 값 표시
    ui->label_7->setText(QString::number(ui->Temp_Slider->value()) + " °C");
    QObject::connect(ui->Temp_Slider, &QSlider::valueChanged,
                     [=](int value) {
                         ui->label_7->setText(QString::number(value) + " °C");
                     });
    QObject::connect(ui->Temp_Slider, &QSlider::sliderReleased,
                     [=]() {
                         updateListFilter();
                         updateGraph();
                     });

    QObject::connect(ui->CbBox_Cycle_Count, &QComboBox::currentIndexChanged,
                     [=](int) {
                         updateListFilter();
                         updateGraph();
                     });

    if (!ui->CBox_Charge->isChecked() && !ui->CBox_Discharge->isChecked() && !ui->CBox_All->isChecked())
        ui->CBox_All->setChecked(true);

    QObject::connect(ui->CBox_Charge, &QCheckBox::toggled,
                     [=](bool checked) {
                         QSignalBlocker blockCharge(ui->CBox_Charge);
                         QSignalBlocker blockDischarge(ui->CBox_Discharge);
                         QSignalBlocker blockAll(ui->CBox_All);

                         if (checked)
                         {
                             ui->CBox_Discharge->setChecked(false);
                             ui->CBox_All->setChecked(false);
                         }
                         else if (!ui->CBox_Discharge->isChecked())
                         {
                             ui->CBox_All->setChecked(true);
                         }

                         updateGraph();
                     });
    QObject::connect(ui->CBox_Discharge, &QCheckBox::toggled,
                     [=](bool checked) {
                         QSignalBlocker blockCharge(ui->CBox_Charge);
                         QSignalBlocker blockDischarge(ui->CBox_Discharge);
                         QSignalBlocker blockAll(ui->CBox_All);

                         if (checked)
                         {
                             ui->CBox_Charge->setChecked(false);
                             ui->CBox_All->setChecked(false);
                         }
                         else if (!ui->CBox_Charge->isChecked())
                         {
                             ui->CBox_All->setChecked(true);
                         }

                         updateGraph();
                     });
    QObject::connect(ui->CBox_All, &QCheckBox::toggled,
                     [=](bool checked) {
                         QSignalBlocker blockCharge(ui->CBox_Charge);
                         QSignalBlocker blockDischarge(ui->CBox_Discharge);
                         QSignalBlocker blockAll(ui->CBox_All);

                         if (checked)
                         {
                             ui->CBox_Charge->setChecked(false);
                             ui->CBox_Discharge->setChecked(false);
                         }
                         else if (!ui->CBox_Charge->isChecked() && !ui->CBox_Discharge->isChecked())
                         {
                             ui->CBox_All->setChecked(true);
                         }

                         updateGraph();
                     });

    updateGraph();
}


void Log_Analyize::openCSV()
{
    QString path = QFileDialog::getOpenFileName(
        nullptr,
        "CSV 선택",
        "",
        "CSV Files (*.csv)");

    if (path.isEmpty())
        return;

    addFileToList(path);
    updateListFilter();
    updateGraph();
}

void Log_Analyize::updateListFilter()
{
    const bool conditionEnabled = ui->CBox_Condition_ONOFF->isChecked(); 
    const double targetTemp = static_cast<double>(ui->Temp_Slider->value());
    const int targetCycle = ui->CbBox_Cycle_Count->currentText().toInt();
    QSignalBlocker blockList(ui->File_List);

    for (int i = 0; i < ui->File_List->count(); ++i)
    {
        QListWidgetItem *item = ui->File_List->item(i);
        const QString path = fileMap.value(item->text());

        if (!conditionEnabled)
        {
            item->setHidden(false);
            continue;
        }

        const FileFilterMeta &meta = getFilterMeta(path);
        const bool tempMatch = !meta.hasFirstTemp || std::abs(meta.firstTemp - targetTemp) < 1.0;
        const bool cycleMatch = meta.hasRepeatColumn && meta.repeatTimes.contains(targetCycle);
        const bool match = tempMatch && cycleMatch;
        item->setHidden(!match);

        // 조건 미충족 항목은 체크 해제
        if (!match && item->checkState() == Qt::Checked)
        {
            item->setCheckState(Qt::Unchecked);
            selectedFiles.remove(item->text());
        }
    }
}

const Log_Analyize::FileFilterMeta &Log_Analyize::getFilterMeta(const QString &filePath)
{
    auto it = filterMetaMap.find(filePath);
    if (it == filterMetaMap.end())
        it = filterMetaMap.insert(filePath, FileFilterMeta{});

    FileFilterMeta &meta = it.value();
    if (meta.parsed)
        return meta;

    meta.parsed = true;
    meta.hasFirstTemp = false;
    meta.firstTemp = 0.0;
    meta.hasRepeatColumn = false;
    meta.repeatTimes.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return meta;

    QTextStream in(&file);
    in.setEncoding(QStringConverter::System);
    in.readLine(); // line 1
    in.readLine(); // line 2

    const QStringList headers = in.readLine().split(",");
    int tempIdx = -1;
    int repeatIdx = -1;
    for (int i = 0; i < headers.size(); ++i)
    {
        QString h = headers[i];
        h.remove('"');
        h = h.trimmed().toLower();
        if (h.contains("battery") && h.contains("temp"))
            tempIdx = i;
        if (h.contains("repeat") && h.contains("time"))
            repeatIdx = i;
    }
    meta.hasRepeatColumn = (repeatIdx >= 0);

    while (!in.atEnd())
    {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;

        if (tempIdx >= 0 && !meta.hasFirstTemp)
        {
            QString tempRaw = line.section(",", tempIdx, tempIdx).trimmed();
            tempRaw.remove("C", Qt::CaseInsensitive);
            tempRaw.remove("\u00B0");

            bool okTemp = false;
            const double tempVal = tempRaw.toDouble(&okTemp);
            if (okTemp)
            {
                meta.hasFirstTemp = true;
                meta.firstTemp = tempVal;
            }
        }

        if (repeatIdx >= 0)
        {
            const QString repeatRaw = line.section(",", repeatIdx, repeatIdx).trimmed();
            bool okRepeat = false;
            const double repeatVal = repeatRaw.toDouble(&okRepeat);
            if (okRepeat)
                meta.repeatTimes.insert(static_cast<int>(std::lround(repeatVal)));
        }

    }

    return meta;
}

bool Log_Analyize::matchesSelectedMode(const QString &modeValue)
{
    if (!ui->CBox_Condition_ONOFF->isChecked())
        return true;

    const bool chargeChecked = ui->CBox_Charge->isChecked();
    const bool dischargeChecked = ui->CBox_Discharge->isChecked();
    const bool allChecked = ui->CBox_All->isChecked();

    if (allChecked || chargeChecked == dischargeChecked)
        return true;

    const QString normalizedMode = modeValue.trimmed();

    if (chargeChecked)
        return normalizedMode.contains("충전");

    if (dischargeChecked)
        return normalizedMode.contains("방전");

    return true;
}

void Log_Analyize::openCSVFolder()
{
    const QString folderPath = QFileDialog::getExistingDirectory(
        nullptr,
        "CSV 폴더 선택",
        "");

    if (folderPath.isEmpty())
        return;

    std::function<void(const QString&)> scanDir = [&](const QString &dirPath) {
        QDir dir(dirPath);
        const QFileInfoList csvFiles = dir.entryInfoList(
            QStringList() << "*.csv", QDir::Files | QDir::NoSymLinks);
        for (const QFileInfo &fi : csvFiles)
            addFileToList(fi.absoluteFilePath());
        const QFileInfoList subDirs = dir.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        for (const QFileInfo &fi : subDirs)
            scanDir(fi.absoluteFilePath());
    };
    scanDir(folderPath);
    updateListFilter();
    updateGraph();
}



void Log_Analyize::addFileToList(const QString &path)
{
    QFileInfo info(path);
    QString name = info.fileName();

    if (fileMap.contains(name))
        return;

    fileMap[name] = path;

    QListWidgetItem *item = new QListWidgetItem(name);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Unchecked);

    ui->File_List->addItem(item);
}


// 🔹 리스트 클릭 → 해당 그래프만 표시
void Log_Analyize::onFileSelected()
{
    QListWidgetItem *item = ui->File_List->currentItem();
    if (!item) return;

    QString name = item->text();
    QString path = fileMap[name];

    xData.clear();
    yData.clear();

    loadCSV(path);

    clearGraph();          // 기존 그래프 제거
    drawCSVGraph(path);    // 새 그래프
}


void Log_Analyize::clearGraph()
{
    if (chart)
        chart->removeAllSeries();
    if (tempChart)
        tempChart->removeAllSeries();
    if (interResChart)
        interResChart->removeAllSeries();

    selectedFiles.clear();
    fileMap.clear();
    filterMetaMap.clear();

    // 체크 해제
    for (int i = 0; i < ui->File_List->count(); i++)
    {
        QListWidgetItem *item = ui->File_List->item(i);
        item->setCheckState(Qt::Unchecked);
    }

    ui->File_List->clear();
}

// 🔹 CSV 파싱 (total_volt + battery_temp + inter_res 자동 찾기)
void Log_Analyize::loadCSV(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;

    QTextStream in(&file);
    in.setEncoding(QStringConverter::System);

    in.readLine(); // 1
    in.readLine(); // 2

    QStringList parts = in.readLine().split(",");

    int voltIdx = -1;
    int tempIdx = -1;
    int interResIdx = -1;
    int modeIdx = -1;
    hasInterResValue = false;
    interResMilliOhmValue = 0.0;

    for (int i = 0; i < parts.size(); ++i)
    {
        QString h = parts[i].toLower();
        h.remove('"');
        h = h.trimmed();

        if (h.contains("total") && h.contains("volt"))
            voltIdx = i;

        if (h.contains("battery") && h.contains("temp"))
            tempIdx = i;

        if (h.contains("inter") && h.contains("res"))
            interResIdx = i;

        if (h.contains("mode"))
            modeIdx = i;
    }

    if (voltIdx < 0 && tempIdx < 0 && interResIdx < 0) return;

    int row = 0;

    while (!in.atEnd())
    {
        const QString line = in.readLine();

        if (modeIdx >= 0)
        {
            const QString modeRaw = line.section(",", modeIdx, modeIdx).trimmed();
            if (!matchesSelectedMode(modeRaw))
            {
                ++row;
                continue;
            }
        }

        if (voltIdx >= 0)
        {
            QString voltRaw = line.section(",", voltIdx, voltIdx).trimmed();
            voltRaw.remove("V", Qt::CaseInsensitive);

            bool okVolt = false;
            const double voltValue = voltRaw.toDouble(&okVolt);
            if (okVolt)
            {
                xData.append(row);
                yData.append(voltValue);
            }
        }

        if (tempIdx >= 0)
        {
            QString tempRaw = line.section(",", tempIdx, tempIdx).trimmed();
            tempRaw.remove("C", Qt::CaseInsensitive);
            tempRaw.remove("\u00B0", Qt::CaseInsensitive);

            bool okTemp = false;
            const double tempValue = tempRaw.toDouble(&okTemp);
            if (okTemp)
            {
                tempXData.append(row);
                tempYData.append(tempValue);
            }
        }

        if (interResIdx >= 0 && !hasInterResValue)
        {
            QString interRaw = line.section(",", interResIdx, interResIdx).trimmed();
            interRaw.remove("ohm", Qt::CaseInsensitive);
            interRaw.remove("\u03A9", Qt::CaseInsensitive);

            bool okInterRes = false;
            const double interResValue = interRaw.toDouble(&okInterRes);
            if (okInterRes && interResValue != 0.0)
            {
                hasInterResValue = true;
                interResMilliOhmValue = interResValue * 1000.0;
            }
        }

        ++row;
    }

}
// 🔹 그래프 업데이트 함수 (핵심)

void Log_Analyize::updateGraph()
{
    if (!chart)
    {
        chart = new QChart();
        chart->setTitle("배터리 전압 추이");
        QFont chartTitleFont;
        chartTitleFont.setPointSize(15);
        chartTitleFont.setBold(true);
        chart->setTitleFont(chartTitleFont);
        chart->legend()->setAlignment(Qt::AlignBottom);

        QValueAxis *axisX = new QValueAxis;
        QValueAxis *axisY = new QValueAxis;

        chart->addAxis(axisX, Qt::AlignBottom);
        chart->addAxis(axisY, Qt::AlignLeft);

        view = new ChartViewEx(chart);
        view->setRenderHint(QPainter::Antialiasing);

        view->setRubberBand(QChartView::RectangleRubberBand);
        view->setDragMode(QGraphicsView::ScrollHandDrag);

        QLayout *layout = ui->Log_Graph->layout();
        if (!layout)
            layout = new QVBoxLayout(ui->Log_Graph);

        layout->addWidget(view);
    }

    if (!tempChart)
    {
        tempChart = new QChart();
        tempChart->setTitle("배터리 온도 변화");
        QFont tempTitleFont;
        tempTitleFont.setPointSize(15);
        tempTitleFont.setBold(true);
        tempChart->setTitleFont(tempTitleFont);
        tempChart->legend()->setAlignment(Qt::AlignBottom);

        QValueAxis *tempAxisX = new QValueAxis;
        QValueAxis *tempAxisY = new QValueAxis;

        tempChart->addAxis(tempAxisX, Qt::AlignBottom);
        tempChart->addAxis(tempAxisY, Qt::AlignLeft);

        tempView = new ChartViewEx(tempChart);
        tempView->setRenderHint(QPainter::Antialiasing);
        tempView->setRubberBand(QChartView::RectangleRubberBand);
        tempView->setDragMode(QGraphicsView::ScrollHandDrag);

        QLayout *tempLayout = ui->Temp_Graph->layout();
        if (!tempLayout)
            tempLayout = new QVBoxLayout(ui->Temp_Graph);

        tempLayout->addWidget(tempView);
    }

    if (!interResChart)
    {
        interResChart = new QChart();
        interResChart->setTitle("배터리 내부저항 변화");
        QFont interResTitleFont;
        interResTitleFont.setPointSize(15);
        interResTitleFont.setBold(true);
        interResChart->setTitleFont(interResTitleFont);
        interResChart->legend()->setAlignment(Qt::AlignBottom);

        interResView = new ChartViewEx(interResChart);
        interResView->setRenderHint(QPainter::Antialiasing);
        interResView->setRubberBand(QChartView::RectangleRubberBand);
        interResView->setDragMode(QGraphicsView::ScrollHandDrag);

        QLayout *interResLayout = ui->InterRes_Graph->layout();
        if (!interResLayout)
            interResLayout = new QVBoxLayout(ui->InterRes_Graph);

        interResLayout->addWidget(interResView);
    }

    // 🔥 기존 그래프만 삭제 (layout은 유지)
    chart->removeAllSeries();
    tempChart->removeAllSeries();
    interResChart->removeAllSeries();
    interResChart->removeAxis(interResChart->axisX());
    interResChart->removeAxis(interResChart->axisY());

    double logMinX = std::numeric_limits<double>::max();
    double logMaxX = std::numeric_limits<double>::lowest();
    double logMinY = std::numeric_limits<double>::max();
    double logMaxY = std::numeric_limits<double>::lowest();
    bool hasLogData = false;

    double tempMinX = std::numeric_limits<double>::max();
    double tempMaxX = std::numeric_limits<double>::lowest();
    double tempMinY = std::numeric_limits<double>::max();
    double tempMaxY = std::numeric_limits<double>::lowest();
    bool hasTempData = false;

    double interResMinY = std::numeric_limits<double>::max();
    double interResMaxY = std::numeric_limits<double>::lowest();

    QBarSeries *interResSeries = new QBarSeries();
    int interResIndex = 0;

    const bool conditionEnabled = ui->CBox_Condition_ONOFF->isChecked();
    const double targetTemp = static_cast<double>(ui->Temp_Slider->value());
    const int targetCycle = ui->CbBox_Cycle_Count->currentText().toInt();

    for (const QString &name : selectedFiles)
    {
        QString path = fileMap[name];

        if (conditionEnabled)
        {
            const FileFilterMeta &meta = getFilterMeta(path);
            const bool tempMatch = !meta.hasFirstTemp || std::abs(meta.firstTemp - targetTemp) < 1.0;
            const bool cycleMatch = meta.hasRepeatColumn && meta.repeatTimes.contains(targetCycle);
            if (!(tempMatch && cycleMatch))
                continue;
        }

        xData.clear();
        yData.clear();
        tempXData.clear();
        tempYData.clear();

        loadCSV(path);

        QLineSeries *voltSeries = new QLineSeries();
        voltSeries->setName(name);

        for (int i = 0; i < xData.size(); i++)
        {
            voltSeries->append(xData[i], yData[i]);
            hasLogData = true;
            if (xData[i] < logMinX) logMinX = xData[i];
            if (xData[i] > logMaxX) logMaxX = xData[i];
            if (yData[i] < logMinY) logMinY = yData[i];
            if (yData[i] > logMaxY) logMaxY = yData[i];
        }

        chart->addSeries(voltSeries);

        for (auto axis : chart->axes(Qt::Horizontal))
            voltSeries->attachAxis(axis);

        for (auto axis : chart->axes(Qt::Vertical))
            voltSeries->attachAxis(axis);

        QLineSeries *tempSeries = new QLineSeries();
        tempSeries->setName(name);

        for (int i = 0; i < tempXData.size(); i++)
        {
            tempSeries->append(tempXData[i], tempYData[i]);
            hasTempData = true;
            if (tempXData[i] < tempMinX) tempMinX = tempXData[i];
            if (tempXData[i] > tempMaxX) tempMaxX = tempXData[i];
            if (tempYData[i] < tempMinY) tempMinY = tempYData[i];
            if (tempYData[i] > tempMaxY) tempMaxY = tempYData[i];
        }

        tempChart->addSeries(tempSeries);

        for (auto axis : tempChart->axes(Qt::Horizontal))
            tempSeries->attachAxis(axis);

        for (auto axis : tempChart->axes(Qt::Vertical))
            tempSeries->attachAxis(axis);

        if (hasInterResValue)
        {
            QBarSet *interResBarSet = new QBarSet(name);
            *interResBarSet << interResMilliOhmValue;
            interResSeries->append(interResBarSet);
            if (interResMilliOhmValue < interResMinY) interResMinY = interResMilliOhmValue;
            if (interResMilliOhmValue > interResMaxY) interResMaxY = interResMilliOhmValue;
            interResIndex++;
        }
    }

    QValueAxis *logAxisX = qobject_cast<QValueAxis *>(chart->axes(Qt::Horizontal).value(0));
    QValueAxis *logAxisY = qobject_cast<QValueAxis *>(chart->axes(Qt::Vertical).value(0));
    if (logAxisX && logAxisY)
    {
        if (hasLogData)
        {
            double xPad = (logMaxX - logMinX) * 0.02;
            double yPad = (logMaxY - logMinY) * 0.08;
            if (xPad <= 0.0) xPad = 1.0;
            if (yPad <= 0.0) yPad = 0.5;

            const double xMin = std::max(0.0, logMinX - xPad);
            logAxisX->setRange(xMin, logMaxX + xPad);
            logAxisY->setRange(logMinY - yPad, logMaxY + yPad);
        }
        else
        {
            logAxisX->setRange(0.0, 1.0);
            logAxisY->setRange(0.0, 1.0);
        }
    }

    QValueAxis *tempAxisX = qobject_cast<QValueAxis *>(tempChart->axes(Qt::Horizontal).value(0));
    QValueAxis *tempAxisY = qobject_cast<QValueAxis *>(tempChart->axes(Qt::Vertical).value(0));
    if (tempAxisX && tempAxisY)
    {
        if (hasTempData)
        {
            double xPad = (tempMaxX - tempMinX) * 0.02;
            double yPad = (tempMaxY - tempMinY) * 0.08;
            if (xPad <= 0.0) xPad = 1.0;
            if (yPad <= 0.0) yPad = 0.5;

            const double xMin = std::max(0.0, tempMinX - xPad);
            tempAxisX->setRange(xMin, tempMaxX + xPad);
            tempAxisY->setRange(tempMinY - yPad, tempMaxY + yPad);
        }
        else
        {
            tempAxisX->setRange(0.0, 1.0);
            tempAxisY->setRange(0.0, 1.0);
        }
    }

    interResChart->addSeries(interResSeries);

    QBarCategoryAxis *interResAxisX = new QBarCategoryAxis();
    interResAxisX->append(QStringList() << "inter_res");

    QValueAxis *interResAxisY = new QValueAxis();
    interResAxisY->setTitleText("mΩ");
    interResAxisY->setLabelFormat("%.2f");
    if (interResIndex > 0)
    {
        const double span = interResMaxY - interResMinY;
        double yPad = span * 0.03;
        if (yPad <= 0.0)
        {
            double base = interResMaxY;
            if (base < 0.0) base = -base;
            yPad = (base > 0.0) ? base * 0.03 : 0.1;
        }

        const double yMin = interResMinY - yPad;
        const double yMax = interResMaxY + yPad;
        interResAxisY->setRange(yMin, yMax);
        interResAxisY->setTickCount(6);
    }
    else
    {
        interResAxisY->setRange(0.0, 1.0);
        interResAxisY->setTickCount(6);
    }

    interResChart->addAxis(interResAxisX, Qt::AlignBottom);
    interResChart->addAxis(interResAxisY, Qt::AlignLeft);
    interResSeries->attachAxis(interResAxisX);
    interResSeries->attachAxis(interResAxisY);
}


// 🔹 그래프 그리기
void Log_Analyize::drawCSVGraph(const QString &fileName)
{
    if (!chart)
    {
        chart = new QChart();
        chart->setTitle("배터리 전압 추이");
        QFont chartTitleFont;
        chartTitleFont.setPointSize(10);
        chartTitleFont.setBold(true);
        chart->setTitleFont(chartTitleFont);
        chart->legend()->setAlignment(Qt::AlignBottom);

        QValueAxis *axisX = new QValueAxis;
        QValueAxis *axisY = new QValueAxis;

        chart->addAxis(axisX, Qt::AlignBottom);
        chart->addAxis(axisY, Qt::AlignLeft);

        view = new ChartViewEx(chart);
        view->setRenderHint(QPainter::Antialiasing);

        view->setRubberBand(QChartView::RectangleRubberBand);
        view->setDragMode(QGraphicsView::ScrollHandDrag);
        view->setInteractive(true);

        // 🔥 핵심 수정
        QLayout *layout = ui->Log_Graph->layout();

        if (!layout)
        {
            layout = new QVBoxLayout(ui->Log_Graph);
        }

        layout->addWidget(view);
    }

    QLineSeries *series = new QLineSeries();

    QFileInfo info(fileName);
    series->setName(info.fileName());

    for (int i = 0; i < xData.size(); i++)
        series->append(xData[i], yData[i]);

    chart->addSeries(series);

    for (auto axis : chart->axes(Qt::Horizontal))
        series->attachAxis(axis);

    for (auto axis : chart->axes(Qt::Vertical))
        series->attachAxis(axis);
}




// ================= Battery_Aging =================

Battery_Aging::Battery_Aging(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Battery_Aging)
{
    ui->setupUi(this);
    UI_Styling::applyShadow(this);

    ui->Memo_box->setPlaceholderText("메모를 입력하세요");

    // 페이지 전환
    connect(ui->Log_Analyze_Btn, &QPushButton::clicked, this, [=]() {
        ui->stackedWidget->setCurrentWidget(ui->page_3);
    });

    connect(ui->Schedule_Btn, &QPushButton::clicked, this, [=]() {
        ui->stackedWidget->setCurrentWidget(ui->page_4);
    });

    connect(ui->Live_Btn, &QPushButton::clicked, this, [=]() {
        ui->stackedWidget->setCurrentWidget(ui->page_5);
    });

    // Plan_Page 연결
    planPage = new Plan_Page(ui);
    planPage->init();

    Log_Analyize *logPage = new Log_Analyize(ui);
    logPage->init();

    connect(ui->Memo_Save, &QPushButton::clicked,
            this, [=]() { planPage->saveMemo(); });
}

Battery_Aging::~Battery_Aging()
{
    delete ui;
}
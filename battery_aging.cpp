#include "battery_aging.h"
#include "./ui_battery_aging.h"
#include "UI_Styling.h"

// Qt: Core / IO
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QJsonDocument>
#include <QStringConverter>
#include <QTextStream>

// Qt: Widgets / GUI
#include <QLayout>
#include <QFont>
#include <QPainter>
#include <QSignalBlocker>
#include <QVBoxLayout>

// Qt Charts
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSet>
#include <QtCharts/QLineSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QValueAxis>

// C++ STL
#include <cmath>
#include <functional>
#include <limits>



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
    : ui(ui)
{
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

// ================= Log Analyze =================

Log_Analyize::Log_Analyize(Ui::Battery_Aging *ui)
    : ui(ui)
{
}

void Log_Analyize::init()
{
    // UI: 버튼
    QObject::connect(ui->CSV_Load_Btn, &QPushButton::clicked,
                     this, &Log_Analyize::openCSV);

    QObject::connect(ui->CSV_Folder_Load_Btn, &QPushButton::clicked,
                     this, &Log_Analyize::openCSVFolder);

    QObject::connect(ui->Graph_Clear_Btn, &QPushButton::clicked,
                     this, &Log_Analyize::clearGraph);

    // UI: 파일 리스트 동작
    QObject::connect(ui->File_List, &QListWidget::itemClicked,
                     this, &Log_Analyize::toggleItemCheck);

    QObject::connect(ui->File_List, &QListWidget::itemChanged,
                     this, &Log_Analyize::onItemChanged);

    // UI: 조건 적용 ON/OFF
    ui->groupBox_Log_Condition->setEnabled(ui->CBox_Condition_ONOFF->isChecked());

    QObject::connect(ui->CBox_Condition_ONOFF, &QCheckBox::toggled,
                     this, &Log_Analyize::onConditionToggled);

    // UI: 온도 슬라이더
    ui->label_7->setText(QString::number(ui->Temp_Slider->value()) + " °C");

    QObject::connect(ui->Temp_Slider, &QSlider::valueChanged,
                     this, &Log_Analyize::onTempChanged);

    QObject::connect(ui->Temp_Slider, &QSlider::sliderReleased,
                     this, &Log_Analyize::onTempReleased);

    // UI: 사이클 선택
    QObject::connect(ui->CbBox_Cycle_Count, &QComboBox::currentIndexChanged,
                     this, &Log_Analyize::onCycleChanged);

    // UI: 모드 체크박스(상호 배타)
    if (!ui->CBox_Charge->isChecked() &&
        !ui->CBox_Discharge->isChecked() &&
        !ui->CBox_All->isChecked())
    {
        ui->CBox_All->setChecked(true);
    }

    QObject::connect(ui->CBox_Charge, &QCheckBox::toggled,
                     this, &Log_Analyize::onModeChanged);

    QObject::connect(ui->CBox_Discharge, &QCheckBox::toggled,
                     this, &Log_Analyize::onModeChanged);

    QObject::connect(ui->CBox_All, &QCheckBox::toggled,
                     this, &Log_Analyize::onModeChanged);

    // 초기 그래프 렌더링
    updateGraph();
}

// ----- UI 이벤트 핸들러 -----

void Log_Analyize::toggleItemCheck(QListWidgetItem *item)
{
    item->setCheckState(
        item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
}

void Log_Analyize::onItemChanged(QListWidgetItem *item)
{
    const QString name = item->text();

    if (item->checkState() == Qt::Checked)
        selectedFiles.insert(name);
    else
        selectedFiles.remove(name);

    updateGraph();
}

void Log_Analyize::onConditionToggled(bool checked)
{
    ui->groupBox_Log_Condition->setEnabled(checked);
    updateListFilter();
    updateGraph();
}

void Log_Analyize::onTempChanged(int value)
{
    ui->label_7->setText(QString::number(value) + " °C");
}

void Log_Analyize::onTempReleased()
{
    updateListFilter();
    updateGraph();
}

void Log_Analyize::onCycleChanged()
{
    updateListFilter();
    updateGraph();
}

void Log_Analyize::onModeChanged()
{
    QSignalBlocker b1(ui->CBox_Charge);
    QSignalBlocker b2(ui->CBox_Discharge);
    QSignalBlocker b3(ui->CBox_All);

    if (ui->CBox_Charge->isChecked())
    {
        ui->CBox_Discharge->setChecked(false);
        ui->CBox_All->setChecked(false);
    }
    else if (ui->CBox_Discharge->isChecked())
    {
        ui->CBox_Charge->setChecked(false);
        ui->CBox_All->setChecked(false);
    }
    else
    {
        ui->CBox_All->setChecked(true);
    }

    updateGraph();
}

// ----- 파일 로드 / 리스트 필터 -----

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

    for (int i = 0; i < ui->File_List->count(); ++i)
    {
        QListWidgetItem *item = ui->File_List->item(i);
        item->setCheckState(Qt::Unchecked);
    }

    ui->File_List->clear();
}

// ----- CSV 파싱 -----
void Log_Analyize::loadCSV(const QString &filePath)
{

    interRes.valid = false;
    interRes.value = 0.0;

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

        if (interResIdx >= 0 && !interRes.valid)
        {
            QString interRaw = line.section(",", interResIdx, interResIdx).trimmed();
            interRaw.remove("ohm", Qt::CaseInsensitive);
            interRaw.remove("\u03A9", Qt::CaseInsensitive);

            bool okInterRes = false;
            const double interResValue = interRaw.toDouble(&okInterRes);
            if (okInterRes && interResValue != 0.0)
            {
                interRes.valid = true;
                interRes.value = interResValue * 1000.0;
            }
        }

        ++row;
    }

}

// ----- 그래프 처리 파이프라인 -----

void Log_Analyize::updateGraph()
{
    initCharts();
    resetCharts();
    processSelectedFiles();
    updateAxes();
}

void Log_Analyize::initCharts()
{
    if (!chart)
    {
        chart = createChart("배터리 전압 추이", ui->Log_Graph, view);
    }

    if (!tempChart)
    {
        tempChart = createChart("배터리 온도 변화", ui->Temp_Graph, tempView);
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

        QValueAxis *axisX = new QValueAxis();
        QValueAxis *axisY = new QValueAxis();

        axisX->setRange(0, 1);
        axisY->setRange(0, 1);

        interResChart->addAxis(axisX, Qt::AlignBottom);
        interResChart->addAxis(axisY, Qt::AlignLeft);

        interResView = new ChartViewEx(interResChart);
        interResView->setRenderHint(QPainter::Antialiasing);
        interResView->setRubberBand(QChartView::RectangleRubberBand);
        interResView->setDragMode(QGraphicsView::ScrollHandDrag);

        QLayout *layout = ui->InterRes_Graph->layout();
        if (!layout)
            layout = new QVBoxLayout(ui->InterRes_Graph);

        layout->addWidget(interResView);
    }
}

QChart* Log_Analyize::createChart(const QString &title, QWidget *parent, QChartView *&outView)
{
    QChart *c = new QChart();
    c->setTitle(title);
    QFont titleFont;
    titleFont.setPointSize(15);
    titleFont.setBold(true);
    c->setTitleFont(titleFont);
    c->legend()->setAlignment(Qt::AlignBottom);

    QValueAxis *axisX = new QValueAxis;
    QValueAxis *axisY = new QValueAxis;

    c->addAxis(axisX, Qt::AlignBottom);
    c->addAxis(axisY, Qt::AlignLeft);

    outView = new ChartViewEx(c);
    outView->setRenderHint(QPainter::Antialiasing);
    outView->setRubberBand(QChartView::RectangleRubberBand);
    outView->setDragMode(QGraphicsView::ScrollHandDrag);

    QLayout *layout = parent->layout();
    if (!layout)
        layout = new QVBoxLayout(parent);

    layout->addWidget(outView);

    return c;
}

void Log_Analyize::resetCharts()
{
    chart->removeAllSeries();
    tempChart->removeAllSeries();
    interResChart->removeAllSeries();

    for (auto axis : interResChart->axes())
        interResChart->removeAxis(axis);
}

void Log_Analyize::processSelectedFiles()
{
    QStackedBarSeries *series = new QStackedBarSeries();
    QVector<QString> interResNames;
    QVector<double> interResValues;
    QStringList categories;
    bool hasInterResData = false;

    QStringList orderedSelected;
    for (int i = 0; i < ui->File_List->count(); ++i)
    {
        QListWidgetItem *item = ui->File_List->item(i);
        if (selectedFiles.contains(item->text()))
            orderedSelected << item->text();
    }

    for (const QString &name : orderedSelected)
    {
        QString path = fileMap[name];

        xData.clear();
        yData.clear();
        tempXData.clear();
        tempYData.clear();

        loadCSV(path);

        addVoltageSeries(name);
        addTempSeries(name);

        if (interRes.valid)
        {
            interResNames.append(name);
            interResValues.append(interRes.value);
            hasInterResData = true;
        }
    }

    if (hasInterResData)
    {
        for (const QString &name : interResNames)
            categories << name;

        const int n = interResValues.size();
        for (int i = 0; i < n; ++i)
        {
            QBarSet *set = new QBarSet(interResNames[i]);
            for (int j = 0; j < n; ++j)
                *set << ((i == j) ? interResValues[i] : 0.0);
            series->append(set);
        }

        drawInterResBar(series, categories, true);
    }
    else
    {
        delete series;
        drawInterResBar(nullptr, QStringList(), false);
    }
}


void Log_Analyize::drawInterResBar(QAbstractBarSeries *series, const QStringList &categories, bool hasData)
{
    // 기존 axis 제거
    for (auto axis : interResChart->axes())
        interResChart->removeAxis(axis);

    if (series)
    {
        series->setBarWidth(0.5);
        interResChart->addSeries(series);
    }

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    if (!categories.isEmpty())
        axisX->append(categories);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("mΩ");
    axisY->setLabelFormat("%.2f");

    if (hasData)
    {
        double minY = std::numeric_limits<double>::max();
        double maxY = std::numeric_limits<double>::lowest();
        bool found = false;

        for (QBarSet *set : series->barSets())
        {
            for (int i = 0; i < set->count(); ++i)
            {
                const double v = set->at(i);
                if (v <= 0.0)
                    continue;
                if (v < minY) minY = v;
                if (v > maxY) maxY = v;
                found = true;
            }
        }

        if (!found)
        {
            axisY->setRange(0.0, 1.0);
        }
        else
        {
            double pad = (maxY - minY) * 0.05;
            if (pad <= 0.0)
                pad = (maxY > 0.0) ? maxY * 0.05 : 0.1;

            axisY->setRange(minY - pad, maxY + pad);
        }
    }
    else
    {
        axisY->setRange(0.0, 1.0);
    }

    interResChart->addAxis(axisX, Qt::AlignBottom);
    interResChart->addAxis(axisY, Qt::AlignLeft);

    if (series)
    {
        series->attachAxis(axisX);
        series->attachAxis(axisY);
    }
}

void Log_Analyize::addVoltageSeries(const QString &name)
{
    QLineSeries *series = new QLineSeries();
    series->setName(name);

    for (int i = 0; i < xData.size(); ++i)
        series->append(xData[i], yData[i]);

    attachSeries(chart, series);
}

void Log_Analyize::addTempSeries(const QString &name)
{
    QLineSeries *series = new QLineSeries();
    series->setName(name);

    for (int i = 0; i < tempXData.size(); ++i)
        series->append(tempXData[i], tempYData[i]);

    attachSeries(tempChart, series);
}


void Log_Analyize::attachSeries(QChart *chart, QAbstractSeries *series)
{
    chart->addSeries(series);

    for (auto axis : chart->axes(Qt::Horizontal))
        series->attachAxis(axis);

    for (auto axis : chart->axes(Qt::Vertical))
        series->attachAxis(axis);
}

void Log_Analyize::updateAxes()
{
    // 전압/온도 그래프 모두 동일한 축 업데이트 로직을 재사용합니다.
    auto setAxis = [](QChart* chart, const QVector<double>& x, const QVector<double>& y)
    {
        QValueAxis *axisX = qobject_cast<QValueAxis *>(chart->axes(Qt::Horizontal).value(0));
        QValueAxis *axisY = qobject_cast<QValueAxis *>(chart->axes(Qt::Vertical).value(0));

        if (!axisX || !axisY || x.isEmpty() || y.isEmpty())
        {
            axisX->setRange(0, 1);
            axisY->setRange(0, 1);
            return;
        }

        double minX = *std::min_element(x.begin(), x.end());
        double maxX = *std::max_element(x.begin(), x.end());

        double minY = *std::min_element(y.begin(), y.end());
        double maxY = *std::max_element(y.begin(), y.end());

        double xPad = (maxX - minX) * 0.02;
        double yPad = (maxY - minY) * 0.08;

        if (xPad == 0) xPad = 1;
        if (yPad == 0) yPad = 0.5;

        axisX->setRange(std::max(0.0, minX - xPad), maxX + xPad);
        axisY->setRange(std::max(0.0, minY - yPad), maxY + yPad);
    };

    setAxis(chart, xData, yData);
    setAxis(tempChart, tempXData, tempYData);
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
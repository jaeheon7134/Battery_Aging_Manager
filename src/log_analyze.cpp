#include "log_analyze.h"

#include "ui_battery_aging.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QLayout>
#include <QPainter>
#include <QSignalBlocker>
#include <QStringConverter>
#include <QTextStream>
#include <QVBoxLayout>

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSet>
#include <QtCharts/QLineSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QValueAxis>

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

Log_Analyze::Log_Analyze(Ui::Battery_Aging *ui)
    : ui(ui)
{
}

void Log_Analyze::init()
{
    QObject::connect(ui->CSV_Load_Btn, &QPushButton::clicked,
                     this, &Log_Analyze::openCSV);

    QObject::connect(ui->CSV_Folder_Load_Btn, &QPushButton::clicked,
                     this, &Log_Analyze::openCSVFolder);

    QObject::connect(ui->Graph_Clear_Btn, &QPushButton::clicked,
                     this, &Log_Analyze::clearGraph);

    QObject::connect(ui->File_List, &QListWidget::itemClicked,
                     this, &Log_Analyze::toggleItemCheck);

    QObject::connect(ui->File_List, &QListWidget::itemChanged,
                     this, &Log_Analyze::onItemChanged);

    ui->groupBox_Log_Condition->setEnabled(ui->CBox_Condition_ONOFF->isChecked());

    QObject::connect(ui->CBox_Condition_ONOFF, &QCheckBox::toggled,
                     this, &Log_Analyze::onConditionToggled);

    ui->label_7->setText(QString::number(ui->Temp_Slider->value()) + " °C");

    QObject::connect(ui->Temp_Slider, &QSlider::valueChanged,
                     this, &Log_Analyze::onTempChanged);

    QObject::connect(ui->Temp_Slider, &QSlider::sliderReleased,
                     this, &Log_Analyze::onTempReleased);

    QObject::connect(ui->CbBox_Cycle_Count, &QComboBox::currentIndexChanged,
                     this, &Log_Analyze::onCycleChanged);

    if (!ui->CBox_Charge->isChecked() &&
        !ui->CBox_Discharge->isChecked() &&
        !ui->CBox_All->isChecked())
    {
        ui->CBox_All->setChecked(true);
    }

    QObject::connect(ui->CBox_Charge, &QCheckBox::toggled,
                     this, &Log_Analyze::onModeChanged);

    QObject::connect(ui->CBox_Discharge, &QCheckBox::toggled,
                     this, &Log_Analyze::onModeChanged);

    QObject::connect(ui->CBox_All, &QCheckBox::toggled,
                     this, &Log_Analyze::onModeChanged);

    updateGraph();
}

void Log_Analyze::toggleItemCheck(QListWidgetItem *item)
{
    item->setCheckState(
        item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
}

void Log_Analyze::onItemChanged(QListWidgetItem *item)
{
    const QString name = item->text();

    if (item->checkState() == Qt::Checked)
        selectedFiles.insert(name);
    else
        selectedFiles.remove(name);

    updateGraph();
}

void Log_Analyze::onConditionToggled(bool checked)
{
    ui->groupBox_Log_Condition->setEnabled(checked);
    updateListFilter();
    updateGraph();
}

void Log_Analyze::onTempChanged(int value)
{
    ui->label_7->setText(QString::number(value) + " °C");
}

void Log_Analyze::onTempReleased()
{
    updateListFilter();
    updateGraph();
}

void Log_Analyze::onCycleChanged()
{
    updateListFilter();
    updateGraph();
}

void Log_Analyze::onModeChanged()
{
    QSignalBlocker b1(ui->CBox_Charge);
    QSignalBlocker b2(ui->CBox_Discharge);
    QSignalBlocker b3(ui->CBox_All);
    QCheckBox *changed = qobject_cast<QCheckBox *>(sender());

    if (changed == ui->CBox_All && ui->CBox_All->isChecked())
    {
        ui->CBox_Charge->setChecked(false);
        ui->CBox_Discharge->setChecked(false);
    }
    else if (changed == ui->CBox_Charge && ui->CBox_Charge->isChecked())
    {
        ui->CBox_Discharge->setChecked(false);
        ui->CBox_All->setChecked(false);
    }
    else if (changed == ui->CBox_Discharge && ui->CBox_Discharge->isChecked())
    {
        ui->CBox_Charge->setChecked(false);
        ui->CBox_All->setChecked(false);
    }

    if (!ui->CBox_Charge->isChecked() &&
        !ui->CBox_Discharge->isChecked() &&
        !ui->CBox_All->isChecked())
    {
        ui->CBox_All->setChecked(true);
    }

    updateListFilter();
    updateGraph();
}

void Log_Analyze::openCSV()
{
    const QString path = QFileDialog::getOpenFileName(
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

void Log_Analyze::updateListFilter()
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

        if (!match && item->checkState() == Qt::Checked)
        {
            item->setCheckState(Qt::Unchecked);
            selectedFiles.remove(item->text());
        }
    }
}

const Log_Analyze::FileFilterMeta &Log_Analyze::getFilterMeta(const QString &filePath)
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
    in.readLine();
    in.readLine();

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

bool Log_Analyze::matchesSelectedMode(const QString &modeValue)
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

void Log_Analyze::openCSVFolder()
{
    const QString folderPath = QFileDialog::getExistingDirectory(
        nullptr,
        "CSV 폴더 선택",
        "");

    if (folderPath.isEmpty())
        return;

    std::function<void(const QString &)> scanDir = [&](const QString &dirPath) {
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

void Log_Analyze::addFileToList(const QString &path)
{
    const QFileInfo info(path);
    const QString name = info.fileName();

    if (fileMap.contains(name))
        return;

    fileMap[name] = path;

    QListWidgetItem *item = new QListWidgetItem(name);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Unchecked);

    ui->File_List->addItem(item);
}

void Log_Analyze::clearGraph()
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

Log_Analyze::ParsedCsvData Log_Analyze::loadCSV(const QString &filePath)
{
    ParsedCsvData data;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return data;

    QTextStream in(&file);
    in.setEncoding(QStringConverter::System);

    in.readLine();
    in.readLine();

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

    if (voltIdx < 0 && tempIdx < 0 && interResIdx < 0)
        return data;

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
                data.voltageX.append(row);
                data.voltageY.append(voltValue);
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
                data.tempX.append(row);
                data.tempY.append(tempValue);
            }
        }

        if (interResIdx >= 0 && !data.interRes.valid)
        {
            QString interRaw = line.section(",", interResIdx, interResIdx).trimmed();
            interRaw.remove("ohm", Qt::CaseInsensitive);
            interRaw.remove("\u03A9", Qt::CaseInsensitive);

            bool okInterRes = false;
            const double interResValue = interRaw.toDouble(&okInterRes);
            if (okInterRes && interResValue != 0.0)
            {
                data.interRes.valid = true;
                data.interRes.value = interResValue * 1000.0;
            }
        }

        ++row;
    }

    return data;
}

void Log_Analyze::updateGraph()
{
    initCharts();
    resetCharts();
    processSelectedFiles();
}

void Log_Analyze::initCharts()
{
    if (!chart)
        chart = createChart("배터리 전압 추이", ui->Log_Graph, view);

    if (!tempChart)
        tempChart = createChart("배터리 온도 변화", ui->Temp_Graph, tempView);

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

QChart *Log_Analyze::createChart(const QString &title, QWidget *parent, QChartView *&outView)
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

void Log_Analyze::resetCharts()
{
    chart->removeAllSeries();
    tempChart->removeAllSeries();
    interResChart->removeAllSeries();

    for (auto axis : interResChart->axes())
        interResChart->removeAxis(axis);
}

void Log_Analyze::processSelectedFiles()
{
    QStackedBarSeries *series = new QStackedBarSeries();
    QVector<QString> interResNames;
    QVector<double> interResValues;
    QVector<double> allVoltageX;
    QVector<double> allVoltageY;
    QVector<double> allTempX;
    QVector<double> allTempY;
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
        const QString path = fileMap[name];
        const ParsedCsvData data = loadCSV(path);

        allVoltageX += data.voltageX;
        allVoltageY += data.voltageY;
        allTempX += data.tempX;
        allTempY += data.tempY;

        addVoltageSeries(name, data.voltageX, data.voltageY);
        addTempSeries(name, data.tempX, data.tempY);

        if (data.interRes.valid)
        {
            interResNames.append(name);
            interResValues.append(data.interRes.value);
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

    updateAxes(allVoltageX, allVoltageY, allTempX, allTempY);
}

void Log_Analyze::drawInterResBar(QAbstractBarSeries *series, const QStringList &categories, bool hasData)
{
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
                if (v < minY)
                    minY = v;
                if (v > maxY)
                    maxY = v;
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

void Log_Analyze::addVoltageSeries(const QString &name, const QVector<double> &x, const QVector<double> &y)
{
    QLineSeries *series = new QLineSeries();
    series->setName(name);

    for (int i = 0; i < x.size(); ++i)
        series->append(x[i], y[i]);

    attachSeries(chart, series);
}

void Log_Analyze::addTempSeries(const QString &name, const QVector<double> &x, const QVector<double> &y)
{
    QLineSeries *series = new QLineSeries();
    series->setName(name);

    for (int i = 0; i < x.size(); ++i)
        series->append(x[i], y[i]);

    attachSeries(tempChart, series);
}

void Log_Analyze::attachSeries(QChart *chart, QAbstractSeries *series)
{
    chart->addSeries(series);

    for (auto axis : chart->axes(Qt::Horizontal))
        series->attachAxis(axis);

    for (auto axis : chart->axes(Qt::Vertical))
        series->attachAxis(axis);
}

void Log_Analyze::updateAxes(const QVector<double> &allVoltageX,
                             const QVector<double> &allVoltageY,
                             const QVector<double> &allTempX,
                             const QVector<double> &allTempY)
{
    auto setAxis = [](QChart *chart, const QVector<double> &x, const QVector<double> &y)
    {
        QValueAxis *axisX = qobject_cast<QValueAxis *>(chart->axes(Qt::Horizontal).value(0));
        QValueAxis *axisY = qobject_cast<QValueAxis *>(chart->axes(Qt::Vertical).value(0));

        if (!axisX || !axisY || x.isEmpty() || y.isEmpty())
        {
            if (axisX)
                axisX->setRange(0, 1);
            if (axisY)
                axisY->setRange(0, 1);
            return;
        }

        const double minX = *std::min_element(x.begin(), x.end());
        const double maxX = *std::max_element(x.begin(), x.end());

        const double minY = *std::min_element(y.begin(), y.end());
        const double maxY = *std::max_element(y.begin(), y.end());

        double xPad = (maxX - minX) * 0.02;
        double yPad = (maxY - minY) * 0.08;

        if (xPad == 0)
            xPad = 1;
        if (yPad == 0)
            yPad = 0.5;

        axisX->setRange(std::max(0.0, minX - xPad), maxX + xPad);
        axisY->setRange(std::max(0.0, minY - yPad), maxY + yPad);
    };

    setAxis(chart, allVoltageX, allVoltageY);
    setAxis(tempChart, allTempX, allTempY);
}


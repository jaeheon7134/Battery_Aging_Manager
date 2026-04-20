#include "battery_aging.h"
#include "./ui_battery_aging.h"
#include "UI_Styling.h"

#include <QFile>
#include <QJsonDocument>
#include <QDebug>
#include <QFileDialog>
#include <QTextStream>
#include <QVBoxLayout>
#include <QLayout>
#include <QPainter>

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>



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

    selectedFiles.clear();

    // 체크 해제
    for (int i = 0; i < ui->File_List->count(); i++)
    {
        QListWidgetItem *item = ui->File_List->item(i);
        item->setCheckState(Qt::Unchecked);
    }
}

// 🔹 CSV 파싱 (total volt 자동 찾기)
void Log_Analyize::loadCSV(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;

    QTextStream in(&file);

    in.readLine(); // 1
    in.readLine(); // 2

    QStringList parts = in.readLine().split(",");

    int idx = -1;
    for (int i = 0; i < parts.size(); ++i)
    {
        QString h = parts[i].toLower();

        if (h.contains("total") && h.contains("volt"))
        {
            idx = i;
            break;
        }
    }

    if (idx < 0) return;

    int row = 0;

    // 2. 데이터는 바로 읽는다 (깔끔하게 가정)
    while (!in.atEnd())
    {
        QString raw = in.readLine().section(",", idx, idx).trimmed();
        raw.remove("V", Qt::CaseInsensitive);

        bool ok;
        double value = raw.toDouble(&ok);
        if (!ok) continue;

        xData.append(row++);
        yData.append(value);
    }

}
// 🔹 그래프 업데이트 함수 (핵심)

void Log_Analyize::updateGraph()
{
    if (!chart)
    {
        chart = new QChart();

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

    // 🔥 기존 그래프만 삭제 (layout은 유지)
    chart->removeAllSeries();

    // 🔥 체크된 파일들만 다시 그림
    for (const QString &name : selectedFiles)
    {
        QString path = fileMap[name];

        xData.clear();
        yData.clear();

        loadCSV(path);

        QLineSeries *series = new QLineSeries();
        series->setName(name);

        for (int i = 0; i < xData.size(); i++)
            series->append(xData[i], yData[i]);

        chart->addSeries(series);

        for (auto axis : chart->axes(Qt::Horizontal))
            series->attachAxis(axis);

        for (auto axis : chart->axes(Qt::Vertical))
            series->attachAxis(axis);
    }
}


// 🔹 그래프 그리기
void Log_Analyize::drawCSVGraph(const QString &fileName)
{
    if (!chart)
    {
        chart = new QChart();

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
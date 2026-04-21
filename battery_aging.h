#ifndef BATTERY_AGING_H
#define BATTERY_AGING_H

#include <QMainWindow>
#include <QJsonObject>
#include <QDate>
#include <QMap>
#include <QSet>

#include <QVector>
#include <QString>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>


QT_BEGIN_NAMESPACE
namespace Ui { class Battery_Aging; }
QT_END_NAMESPACE



// ===== Plan_Page 클래스 =====
class Plan_Page
{
public:
    Plan_Page(Ui::Battery_Aging *ui);

    void init();
    void saveMemo();

private:
    Ui::Battery_Aging *ui;

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


class Log_Analyize
{
public:
    Log_Analyize(Ui::Battery_Aging *ui);

    void init();
    void openCSV();
    void openCSVFolder();

private:
    struct FileFilterMeta
    {
        bool parsed = false;
        bool hasFirstTemp = false;
        double firstTemp = 0.0;
        bool hasRepeatColumn = false;
        QSet<int> repeatTimes;
    };

    Ui::Battery_Aging *ui;

    QVector<double> xData;
    QVector<double> yData;
    QVector<double> tempXData;
    QVector<double> tempYData;
    bool hasInterResValue = false;
    double interResMilliOhmValue = 0.0;

    QChart *chart = nullptr;
    QChartView *view = nullptr;
    QChart *tempChart = nullptr;
    QChartView *tempView = nullptr;
    QChart *interResChart = nullptr;
    QChartView *interResView = nullptr;
    QSet<QString> selectedFiles;

    void loadCSV(const QString &filePath);
    void drawCSVGraph(const QString &fileName);
    void addFileToList(const QString &path);
    void onFileSelected();
    void clearGraph();
    void updateGraph();
    void updateListFilter();
    const FileFilterMeta &getFilterMeta(const QString &filePath);
    bool matchesSelectedMode(const QString &modeValue);

    QMap<QString, QString> fileMap;   // 파일명 → 전체경로
    QMap<QString, FileFilterMeta> filterMetaMap;

};


// ===== 메인 클래스 =====
class Battery_Aging : public QMainWindow
{
    Q_OBJECT

public:
    Battery_Aging(QWidget *parent = nullptr);
    ~Battery_Aging();

private:
    Ui::Battery_Aging *ui;

    Plan_Page *planPage;
};

#endif
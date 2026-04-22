#ifndef BATTERY_AGING_H
#define BATTERY_AGING_H

#include <QMainWindow>
#include <QJsonObject>
#include <QDate>
#include <QMap>
#include <QSet>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QListWidgetItem>
#include <QObject>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QAbstractSeries>
#include <QtCharts/QAbstractBarSeries>

QT_BEGIN_NAMESPACE
namespace Ui { class Battery_Aging; }
QT_END_NAMESPACE

// ================= 계획/스케줄 =================
class Plan_Page
{
public:
    Plan_Page(Ui::Battery_Aging *ui);

    // 페이지 상태를 초기화하고 UI 이벤트를 연결합니다.
    void init();
    // 선택한 날짜의 메모/사이클/에이징 시간 데이터를 저장합니다.
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

// ================= 로그 분석 =================
class Log_Analyize : public QObject
{
    Q_OBJECT

public:
    explicit Log_Analyize(Ui::Battery_Aging *ui);

    // 모든 시그널/슬롯을 연결하고 초기 그래프를 구성합니다.
    void init();

    // 사용자 동작
    void openCSV();
    void openCSVFolder();

private:
    // 리스트 필터에서 사용하는 파싱 메타데이터 캐시
    struct FileFilterMeta
    {
        bool parsed = false;
        bool hasFirstTemp = false;
        double firstTemp = 0.0;
        bool hasRepeatColumn = false;
        QSet<int> repeatTimes;
    };

    // 단일 CSV에서 추출한 내부저항 값
    struct InterResData
    {
        bool valid = false;
        double value = 0.0;
    };

private:
    // UI
    Ui::Battery_Aging *ui = nullptr;

    // 현재 그래프 출력에 사용하는 파일별 데이터
    QVector<double> xData;
    QVector<double> yData;
    QVector<double> tempXData;
    QVector<double> tempYData;

    InterResData interRes;

    // 파일 리스트와 필터 캐시
    QSet<QString> selectedFiles;
    QMap<QString, QString> fileMap;
    QMap<QString, FileFilterMeta> filterMetaMap;

    // 그래프
    QChart *chart = nullptr;
    QChartView *view = nullptr;

    QChart *tempChart = nullptr;
    QChartView *tempView = nullptr;

    QChart *interResChart = nullptr;
    QChartView *interResView = nullptr;

private:
    // UI 이벤트 핸들러
    void toggleItemCheck(QListWidgetItem *item);
    void onItemChanged(QListWidgetItem *item);
    void onConditionToggled(bool checked);
    void onTempChanged(int value);
    void onTempReleased();
    void onCycleChanged();
    void onModeChanged();

    // CSV 파싱/필터 로직
    void loadCSV(const QString &filePath);
    void addFileToList(const QString &path);
    void updateListFilter();
    bool matchesSelectedMode(const QString &modeValue);
    const FileFilterMeta &getFilterMeta(const QString &filePath);

    // 그래프 처리 파이프라인
    void updateGraph();
    void clearGraph();
    void initCharts();
    void resetCharts();
    void processSelectedFiles();
    void addVoltageSeries(const QString &name);
    void addTempSeries(const QString &name);
    void attachSeries(QChart *chart, QAbstractSeries *series);
    void updateAxes();
    void drawInterResBar(QAbstractBarSeries *series, const QStringList &categories, bool hasData);

    QChart *createChart(const QString &title, QWidget *parent, QChartView *&outView);
};

// ================= 메인 윈도우 =================
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
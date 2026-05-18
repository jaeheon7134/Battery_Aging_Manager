#ifndef LOG_ANALYZE_H
#define LOG_ANALYZE_H

#include <QListWidgetItem>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include <QtCharts/QAbstractBarSeries>
#include <QtCharts/QAbstractSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>

namespace Ui { class Battery_Aging; }

class Log_Analyze : public QObject
{
    Q_OBJECT

public:
    explicit Log_Analyze(Ui::Battery_Aging *ui);

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

    struct InterResData
    {
        bool valid = false;
        double value = 0.0;
    };

    struct ParsedCsvData
    {
        QVector<double> voltageX;
        QVector<double> voltageY;
        QVector<double> tempX;
        QVector<double> tempY;
        InterResData interRes;
    };

private:
    Ui::Battery_Aging *ui = nullptr;

    QSet<QString> selectedFiles;
    QMap<QString, QString> fileMap;
    QMap<QString, FileFilterMeta> filterMetaMap;

    QChart *chart = nullptr;
    QChartView *view = nullptr;

    QChart *tempChart = nullptr;
    QChartView *tempView = nullptr;

    QChart *interResChart = nullptr;
    QChartView *interResView = nullptr;

private:
    void toggleItemCheck(QListWidgetItem *item);
    void onItemChanged(QListWidgetItem *item);
    void onConditionToggled(bool checked);
    void onTempChanged(int value);
    void onTempReleased();
    void onCycleChanged();
    void onModeChanged();

    ParsedCsvData loadCSV(const QString &filePath);
    void addFileToList(const QString &path);
    void updateListFilter();
    bool matchesSelectedMode(const QString &modeValue);
    const FileFilterMeta &getFilterMeta(const QString &filePath);

    void updateGraph();
    void clearGraph();
    void initCharts();
    void resetCharts();
    void processSelectedFiles();
    void addVoltageSeries(const QString &name, const QVector<double> &x, const QVector<double> &y);
    void addTempSeries(const QString &name, const QVector<double> &x, const QVector<double> &y);
    void attachSeries(QChart *chart, QAbstractSeries *series);
    void updateAxes(const QVector<double> &allVoltageX,
                    const QVector<double> &allVoltageY,
                    const QVector<double> &allTempX,
                    const QVector<double> &allTempY);
    void drawInterResBar(QAbstractBarSeries *series, const QStringList &categories, bool hasData);

    QChart *createChart(const QString &title, QWidget *parent, QChartView *&outView);
};

#endif

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class Battery_Aging; }
QT_END_NAMESPACE

class Plan_Page;
class Log_Analyze;
class Live_Page;

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
    Log_Analyze *logPage;
    Live_Page *livePage;
};

#endif

#include "battery_aging.h"
#include "./ui_battery_aging.h"
#include "UI_Styling.h"
#include "log_analyze.h"
#include "live_page.h"
#include "plan_page.h"

// ================= Battery_Aging =================

Battery_Aging::Battery_Aging(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Battery_Aging)
    , planPage(nullptr)
    , logPage(nullptr)
    , livePage(nullptr)
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

    // 페이지별 로직 초기화
    planPage = new Plan_Page(ui);
    planPage->init();

    logPage = new Log_Analyze(ui);
    logPage->init();

    livePage = new Live_Page(ui);
    livePage->init();

    connect(ui->Memo_Save, &QPushButton::clicked,
            this, [=]() { planPage->saveMemo(); });
}

Battery_Aging::~Battery_Aging()
{
    delete livePage;
    delete logPage;
    delete planPage;
    delete ui;
}

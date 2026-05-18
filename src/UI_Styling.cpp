#include "UI_Styling.h"

#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QGroupBox>
#include <QCalendarWidget>
#include <QTextEdit>
#include <QLineEdit>

// ✅ 스타일 적용 함수
void UI_Styling::applyStyle(QApplication &app)
{
    app.setStyleSheet(R"(

        QWidget#centralwidget,
        QWidget#page_3,
        QWidget#page_4,
        QWidget#page_5 {
            background-color: #330000;
        }

        QFrame {
            background-color: #f5f6f8;
            border-radius: 12px;
            border: 2px solid #dcdfe3;
        }

        QPushButton {
            background-color: #dfe3e8;
            border-radius: 8px;
            padding: 6px 12px;
            border: none;
            color: #222;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #cfd6dd;
        }

        QTextEdit, QPlainTextEdit {
            border-radius: 12px;
            border: 1px solid #dcdfe3;
            background: white;
            padding: 8px;
            color: #222;
        }

        QWidget#page_3 QLabel#Label_Temp_Condition,
        QWidget#page_3 QLabel#Label_Range_Select,
        QWidget#page_3 QLabel#Label_Cycle_Count
        {
            background: #660000;
            color: #ffffff;
            font-size: 16px;
            border: none;
            font-weight: bold;
        }

        QWidget#page_3 QLabel#Log_Analyize_Title {
            background: #990000;
            color: #ffffff;
            border: none;
            font-weight: bold;
            font-size: 40px;
        }

        QWidget#page_3 QGroupBox#groupBox_Log_Condition {
            /*border: 2px solid #bf1a1a;*/
            border-radius: 16px;
            margin-top: 14px;
            background: #7d0000;
        }

        QWidget#page_3 QGroupBox#groupBox_Log_Condition::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 14px;
            padding: 2px 10px;
            border: 1px solid #bf1a1a;
            border-radius: 8px;
            font-size: 15px;
            font-weight: bold;
            color: #ffffff;
            background-color: #8f0f0f;
        }

        QWidget#page_3 QFrame#frame_1,
        QWidget#page_3 QFrame#frame_2,
        QWidget#page_3 QFrame#frame_3 {
            background-color: #660000;
            border-radius: 10px;
            border: 1px solid #9f2a2a;
        }

        QWidget#page_3 QCheckBox {
            color: #f4f6f8;
            background: transparent;
            spacing: 8px;
            font-weight: 600;
        }

        QWidget#page_3 QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #cfcfcf;
            border-radius: 3px;
            background: #3a0d0d;
        }

        QWidget#page_3 QCheckBox::indicator:checked {
            border: 1px solid #83c97a;
            background: #57b947;
        }

        QWidget#page_3 QCheckBox#CBox_Condition_ONOFF {
            background: transparent;
            color: #ffffff;
            font-size: 28px;
            font-weight: 700;
        }

        QWidget#page_3 QCheckBox#CBox_Condition_ONOFF::indicator {
            width: 18px;
            height: 18px;
            border: 1px solid #d5d5d5;
            border-radius: 3px;
            background: #230808;
        }

        QWidget#page_3 QCheckBox#CBox_Condition_ONOFF::indicator:checked {
            background: #d02a2a;
            image: none;
        }

        QWidget#page_3 QSlider#Temp_Slider::groove:horizontal {
            border: none;
            height: 6px;
            border-radius: 3px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #3f8cff, stop:1 #d54040);
        }

        QWidget#page_3 QSlider#Temp_Slider::handle:horizontal {
            width: 18px;
            margin: -6px 0;
            border-radius: 9px;
            border: 2px solid #f2d7d7;
            background: #b22a2a;
        }

        QWidget#page_3 QComboBox#CbBox_Cycle_Count {
            color: #ffffff;
            background: #2a0707;
            border: 2px solid #8d1e1e;
            border-radius: 8px;
            padding: 3px 26px 3px 8px;
            font-weight: bold;
        }

        QWidget#page_3 QComboBox#CbBox_Cycle_Count::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 22px;
            border-left: 1px solid #8d1e1e;
        }

        QWidget#page_3 QComboBox#CbBox_Cycle_Count::down-arrow {
            image: none;
            width: 0;
            height: 0;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 7px solid #ff4d4d;
        }
        
        QWidget#page_4 QLabel {
            background: #660000;
            color: #ffffff;
            font-size: 12px;
            border: none;
            font-weight: bold;
        }

        QWidget#page_4 QLabel#Plan_Tab_Tiltle {
            background: #990000;
            color: #ffffff;
            border: none;
            font-weight: bold;
            font-size: 40px;
        }

        QWidget#page_4 QLabel#Current_time,
        QWidget#page_4 QLabel#goal_time {
            background-color: #003366;
            color: #ffffff;
            padding: 2px 6px;
            border : 2px solid #001f4d;
        }

        QWidget#page_4 QLineEdit#Cycle_time,
        QWidget#page_4 QLineEdit#Aging_time {
            background-color: #003366;
            color: #ffffff;
            padding: 2px 6px;
            border : 2px solid #001f4d;
            font-weight : bold;
        }

        QWidget#page_4 QTextEdit#Memo_box {
            background-color: #000000;
            color: #ffffff;
            border: 2px solid #555555;
            padding: 6px;
            font-family: Consolas;
        }

        QWidget#page_4 QPushButton#Memo_Save {
            background-color: #CC0000;
            color: #ffffff;
            border: 1px solid #660000;
            border-radius: 4px;
            padding: 6px 10px;
            font-weight: bold;
        }

        QWidget#page_4 QProgressBar {
            border-radius: 8px;
            background: #cfcfcf;
            text-align: center;
            color: black;
            font-weight: bold;
        }

        QWidget#page_4 QProgressBar::chunk {
            border-radius: 8px;
            background: #4caf50;
        }

        QWidget#page_4 QCalendarWidget {
            border-radius: 12px;
            background: #f5f6f8;
            border: 1px solid #dcdfe3;
            padding: 6px;
        }

        QWidget#page_4 QCalendarWidget QAbstractItemView {
            border-radius: 10px;
            background: white;
            selection-background-color: #4caf50;
            selection-color: black;
            color: #333;
        }

        QWidget#page_4 QCalendarWidget QAbstractItemView:disabled {
            color: #cccccc;
        }

        QWidget#page_4 QCalendarWidget QWidget#qt_calendar_navigationbar {
            background: #efeff2;
            border: 1px solid #d6d6dc;
            border-radius: 8px;
            padding: 5px;
        }

        QWidget#page_4 QCalendarWidget QToolButton {
            color: #1d1d22;
            font-weight: bold;
            background: transparent;
            border: none;
            padding: 4px 10px;
        }

        QWidget#page_4 QCalendarWidget QToolButton#qt_calendar_monthbutton,
        QWidget#page_4 QCalendarWidget QToolButton#qt_calendar_yearbutton {
            color: #111111;
            font-weight: 700;
        }

        QWidget#page_4 QCalendarWidget QToolButton#qt_calendar_prevmonth,
        QWidget#page_4 QCalendarWidget QToolButton#qt_calendar_nextmonth {
            min-width: 25px;
            color: #8a1212;
        }

        QWidget#page_4 QCalendarWidget QHeaderView::section {
            color: #1f1f24;
            background: #e7e7eb;
            border: 1px solid #d8d8df;
            font-weight: bold;
        }

        QWidget#page_4 QGroupBox {
            border: 1px solid #dcdfe3;
            border-radius: 12px;
            margin-top: 12px;
            background: #660000;
            border: none;
        }

        QWidget#page_4 QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 12px;
            padding: 0 6px;
            font-size: 16px;
            font-weight: bold;
            color: #ffffff;
            background-color: #990000;
        }

    )");
}

#if 1
// ✅ 그림자 적용 (기존 그대로 유지)
void UI_Styling::applyShadow(QWidget* parent)
{
    // 👉 GroupBox만 적용 (패널 느낌)
    QList<QGroupBox*> boxes = parent->findChildren<QGroupBox*>();

    for (auto box : boxes)
    {
        auto shadow = new QGraphicsDropShadowEffect(parent);

        shadow->setBlurRadius(8);
        shadow->setOffset(0, 2);
        shadow->setColor(QColor(0, 0, 0, 120));

        box->setGraphicsEffect(shadow);
    }

    // 👉 메모창만 살짝 강조
    QList<QTextEdit*> edits = parent->findChildren<QTextEdit*>();

    for (auto edit : edits)
    {
        if (edit->objectName() == "Memo_box")
        {
            auto shadow = new QGraphicsDropShadowEffect(parent);

            shadow->setBlurRadius(10);
            shadow->setOffset(0, 2);
            shadow->setColor(QColor(0, 0, 0, 150));

            edit->setGraphicsEffect(shadow);
        }
    }
}
#endif

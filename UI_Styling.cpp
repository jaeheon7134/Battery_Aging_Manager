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
        QWidget {
            background-color: #330000;
        }

        QFrame {
            background-color: #f5f6f8;
            border-radius: 12px;
            border: 1px solid #dcdfe3;
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

        QProgressBar {
            border-radius: 8px;
            background: #cfcfcf;
            text-align: center;
            color: black;
            font-weight: bold;
        }
        QProgressBar::chunk {
            border-radius: 8px;
            background: #4caf50;
        }

        QCalendarWidget {
            border-radius: 12px;
            background: #f5f6f8;
            border: 1px solid #dcdfe3;
            padding: 6px;
        }

        QCalendarWidget QAbstractItemView {
            border-radius: 10px;
            background: white;
            selection-background-color: #4caf50;
            selection-color: black;
            color: #333;
        }


        QCalendarWidget QAbstractItemView:disabled {
            color: #cccccc;
        }

        QCalendarWidget QWidget#qt_calendar_navigationbar {
            padding: 5px;
        }

        QCalendarWidget QToolButton {
            color: black;
            font-weight: bold;
            background: transparent;
            border: none;
            padding: 4px 10px;
        }

        QCalendarWidget QToolButton#qt_calendar_prevmonth,
        QCalendarWidget QToolButton#qt_calendar_nextmonth {
            min-width: 25px;
        }

        QCalendarWidget QHeaderView::section {
            color: #333;
            background: transparent;
            font-weight: bold;
        }

        QLabel {
            background: #660000;
            color: #ffffff;
            font-size: 12px;
            border: none;
            font-weight: bold;
        }

        QLabel#title {
            font-size: 20px;
            font-weight: bold;
            color: #111;
        }

        QLabel#section {
            font-size: 14px;
            font-weight: bold;
            color: #222;
        }

        QLabel#unitLabel {
            color: #555;
            font-weight: normal;
        }

        QLabel#valueLabel {
            color: #111;
            font-weight: bold;
            font-size: 13px;
        }

        QGroupBox {
            border: 1px solid #dcdfe3;
            border-radius: 12px;
            margin-top: 12px;
            background: #660000;
            border: none;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 12px;
            padding: 0 6px;
            font-size: 16px;
            font-weight: bold;
            color: #ffffff;
            background-color: #990000;
        }

        QLabel#Plan_Tab_Tiltle {
            background: #990000;
            color: #ffffff;
            border: none;
            font-weight: bold;
            font-size: 40px;
        }

        QLabel#Current_time,
        QLabel#goal_time {
            background-color: #003366;
            color: #ffffff;
            padding: 2px 6px;
            border : 2px solid #001f4d;
        }

        QLineEdit#Cycle_time,
        QLineEdit#Aging_time {
            background-color: #003366;
            color: #ffffff;
            padding: 2px 6px;
            border : 2px solid #001f4d;
            font-weight : bold;
        }

        QTextEdit#Memo_box {
            background-color: #000000;
            color: #ffffff;
            border: 2px solid #555555;
            padding: 6px;
            font-family: Consolas;
        }

        QPushButton#Memo_Save {
            background-color: #CC0000;
            color: #ffffff;
            border: 1px solid #660000;
            border-radius: 4px;
            padding: 6px 10px;
            font-weight: bold;
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

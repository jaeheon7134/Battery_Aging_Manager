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
            background-color: #e6e8eb;
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

        QLineEdit {
            border-radius: 8px;
            border: 1px solid #ccc;
            padding: 4px;
            background: white;
            color: #222;
            font-weight: bold;
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
            background: transparent;
            color: #222;
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
            background-color: #e0e0e0;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 12px;
            padding: 0 6px;
            font-size: 16px;
            font-weight: bold;
            color: #222;
            background-color: #e6e8eb;
        }

        QLineEdit#lineEdit_3 {
            background: #e0e0e0;
        }
    )");
}

// ✅ 그림자 적용 (기존 그대로 유지)
void UI_Styling::applyShadow(QWidget* parent)
{
    QList<QGroupBox*> boxes = parent->findChildren<QGroupBox*>();
    for (auto box : boxes)
    {
        auto shadow = new QGraphicsDropShadowEffect(parent);
        shadow->setBlurRadius(20);
        shadow->setOffset(0, 4);
        shadow->setColor(QColor(0, 0, 0, 40));
        box->setGraphicsEffect(shadow);
    }

    QList<QCalendarWidget*> calendars = parent->findChildren<QCalendarWidget*>();
    for (auto cal : calendars)
    {
        auto shadow = new QGraphicsDropShadowEffect(parent);
        shadow->setBlurRadius(20);
        shadow->setOffset(0, 4);
        shadow->setColor(QColor(0, 0, 0, 40));
        cal->setGraphicsEffect(shadow);
    }

    QList<QTextEdit*> edits = parent->findChildren<QTextEdit*>();
    for (auto edit : edits)
    {
        auto shadow = new QGraphicsDropShadowEffect(parent);
        shadow->setBlurRadius(25);
        shadow->setOffset(0, 4);
        shadow->setColor(QColor(0, 0, 0, 50));
        edit->setGraphicsEffect(shadow);
    }

    QList<QLineEdit*> lines = parent->findChildren<QLineEdit*>();
    for (auto line : lines)
    {
        if (line->objectName() == "lineEdit_3")
        {
            auto shadow = new QGraphicsDropShadowEffect(parent);
            shadow->setBlurRadius(30);
            shadow->setOffset(0, 4);
            shadow->setColor(QColor(0, 0, 0, 40));
            line->setGraphicsEffect(shadow);
        }
    }
}
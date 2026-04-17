#include "battery_aging.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyleSheet(R"(
        QWidget {
            background-color: #e6e8eb;
        }

        /* 카드 느낌 */
        QFrame {
            background-color: #f5f6f8;
            border-radius: 12px;
            border: 1px solid #dcdfe3;
        }

        /* 버튼 */
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

        /* 입력창 */
        QLineEdit {
            border-radius: 8px;
            border: 1px solid #ccc;
            padding: 4px;
            background: white;
            color: #222;
            font-weight: bold;
        }

        /* 메모창 */
        QTextEdit, QPlainTextEdit {
            border-radius: 12px;
            border: 1px solid #dcdfe3;
            background: white;
            padding: 8px;
            color: #222;
        }

        /* 프로그레스바 */
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

        /* 달력 전체 */
        QCalendarWidget {
            border-radius: 12px;
            background: #f5f6f8;
            border: 1px solid #dcdfe3;
            padding: 6px;

        }

        /* 날짜 영역 */
        QCalendarWidget QAbstractItemView {
            border-radius: 10px;
            background: white;
            selection-background-color: #4caf50;
            selection-color: black;
            color: #333;
        }

        /* 달력 상단 (월/년도 + 화살표) */
        QCalendarWidget QWidget#qt_calendar_navigationbar {
            padding: 5px;
        }

        QCalendarWidget QToolButton {
            color: black;
            font-weight: bold;
            background: transparent;
            border: none;
            padding: 4px 10px;   /* ← 겹침 해결 핵심 */
        }

        QCalendarWidget QToolButton#qt_calendar_prevmonth,
        QCalendarWidget QToolButton#qt_calendar_nextmonth {
            min-width: 25px;
        }

        /* 요일 */
        QCalendarWidget QHeaderView::section {
            color: #333;
            background: transparent;
            font-weight: bold;
        }

        /* 라벨 */
        QLabel {
            background: transparent;
            color: #222;
            font-size: 12px;
            border: none;
            font-weight: bold;

        }

        /* 메인 제목 */
        QLabel#title {
            font-size: 20px;
            font-weight: bold;
            color: #111;
        }

        /* 섹션 제목 */
        QLabel#section {
            font-size: 14px;
            font-weight: bold;
            color: #222;
        }

        /* 단위 */
        QLabel#unitLabel {
            color: #555;
            font-weight: normal;
        }

        /* 값 강조 */
        QLabel#valueLabel {
            color: #111;
            font-weight: bold;
            font-size: 13px;
        }

        /* 그룹박스 */
        QGroupBox {
            border: 1px solid #dcdfe3;
            border-radius: 12px;
            margin-top: 12px;

            background-color : #e0e0e0;
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

    Battery_Aging w;
    w.show();

    return a.exec();
}
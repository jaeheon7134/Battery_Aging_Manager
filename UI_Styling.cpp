#include "UI_Styling.h"

#include <QGraphicsDropShadowEffect>
#include <QGroupBox>
#include <QCalendarWidget>
#include <QTextEdit>
#include <QLineEdit>

void UI_Styling::applyShadow(QWidget* parent)
{
    /* =========================
       GroupBox 그림자
    ========================= */
    QList<QGroupBox*> boxes = parent->findChildren<QGroupBox*>();

    for (auto box : boxes)
    {
        auto shadow = new QGraphicsDropShadowEffect(parent);
        shadow->setBlurRadius(20);
        shadow->setOffset(0, 4);
        shadow->setColor(QColor(0, 0, 0, 40));

        box->setGraphicsEffect(shadow);
    }

    /* =========================
       달력 그림자
    ========================= */
    QList<QCalendarWidget*> calendars = parent->findChildren<QCalendarWidget*>();

    for (auto cal : calendars)
    {
        auto shadow = new QGraphicsDropShadowEffect(parent);
        shadow->setBlurRadius(20);
        shadow->setOffset(0, 4);
        shadow->setColor(QColor(0, 0, 0, 40));

        cal->setGraphicsEffect(shadow);
    }

    /* =========================
       메모창 (textEdit) 그림자
    ========================= */
    QList<QTextEdit*> edits = parent->findChildren<QTextEdit*>();

    for (auto edit : edits)
    {
        auto shadow = new QGraphicsDropShadowEffect(parent);
        shadow->setBlurRadius(25);
        shadow->setOffset(0, 4);
        shadow->setColor(QColor(0, 0, 0, 50));

        edit->setGraphicsEffect(shadow);
    }

    /* =========================
       제목 (lineEdit_3) 그림자
    ========================= */
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
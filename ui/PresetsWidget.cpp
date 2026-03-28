#include "PresetsWidget.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QDebug>

PresetsWidget::PresetsWidget(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    QLabel* title = new QLabel("Quick Presets", this);
    title->setStyleSheet("font-size: 28px; font-weight: 800; color: #f8fafc; margin-bottom: 20px;");
    mainLayout->addWidget(title);

    QGridLayout* grid = new QGridLayout();
    grid->setSpacing(20);

    QString buttonStyle = R"(
        QPushButton { background-color: rgba(30, 41, 59, 0.6); color: white; border: 1px solid #1e293b; border-radius: 12px; padding: 30px; font-size: 16px; font-weight: bold; }
        QPushButton:hover { background-color: rgba(30, 41, 59, 0.9); border: 1px solid #38bdf8; color: #38bdf8; }
    )";

    QPushButton* workModeBtn = new QPushButton("Work Mode\n(Hides Social & Personal)", this);
    QPushButton* gameModeBtn = new QPushButton("Gaming Mode\n(Hides Browsers & Passwords)", this);
    QPushButton* strictModeBtn = new QPushButton("Strict Mode\n(Hides Everything but Active)", this);

    workModeBtn->setStyleSheet(buttonStyle);
    gameModeBtn->setStyleSheet(buttonStyle);
    strictModeBtn->setStyleSheet(buttonStyle);

    grid->addWidget(workModeBtn, 0, 0);
    grid->addWidget(gameModeBtn, 0, 1);
    grid->addWidget(strictModeBtn, 1, 0, 1, 2); // Span 2 columns

    mainLayout->addLayout(grid);
    mainLayout->addStretch();

    // Placeholder Logic
    connect(workModeBtn, &QPushButton::clicked, []() { qDebug() << "[Backend Placeholder] Loading 'Work Mode' profile rules."; });
    connect(gameModeBtn, &QPushButton::clicked, []() { qDebug() << "[Backend Placeholder] Loading 'Gaming Mode' profile rules."; });
}
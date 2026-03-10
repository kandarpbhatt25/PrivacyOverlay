#pragma once
#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void setupSidebar();
    void setupAppListScreen();
    void loadActiveApps();

    QListWidget* m_sidebar;
    QStackedWidget* m_stackedWidget;
    QWidget* m_appListContainer;
    QVBoxLayout* m_appListLayout;
};
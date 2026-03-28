#include <QApplication>
#include <QDebug>
#include <windows.h>
#include <vector>
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    qDebug() << "--- PrivacyOverlay Started ---";

    MainWindow w;
    w.show();

    return app.exec(); 
}
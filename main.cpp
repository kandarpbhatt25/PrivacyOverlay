#include <QApplication>
#include <QDebug>
#include <windows.h>
#include <vector>
#include <QSettings>
#include <QProcess>
#include "ui/MainWindow.h"

// Helper to check if currently running as Administrator
bool IsUserAdmin() {
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup; 
    b = AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &AdministratorsGroup); 
    if(b) {
        if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) {
            b = FALSE;
        } 
        FreeSid(AdministratorsGroup); 
    }
    return b;
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Auto-Elevation logic
    QSettings settings("PrivacyOverlay", "App");
    if (settings.value("ElevatedStartup", false).toBool() && !IsUserAdmin()) {
        qDebug() << "--- PrivacyOverlay Auto-Elevating to Admin! ---";
        // Attempt to launch our previously created scheduled task
        QProcess::startDetached("schtasks", QStringList() << "/run" << "/tn" << "PrivacyOverlay_Elevated");
        // Exit this standard user instance immediately
        return 0;
    }

    qDebug() << "--- PrivacyOverlay Started " << (IsUserAdmin() ? "(Admin)" : "(Standard)") << " ---";

    MainWindow w;
    w.show();

    return app.exec(); 
}
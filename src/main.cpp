#include <QApplication>

#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("comm-sampling"));
    app.setOrganizationName(QStringLiteral("comm-sampling"));

    comm::MainWindow window;
    window.resize(1000, 800);
    window.show();

    return app.exec();
}

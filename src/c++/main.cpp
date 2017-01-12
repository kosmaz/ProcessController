#include <QApplication>
#include <QLoggingCategory>
#include "MainController.hpp"

int main(int argc, char *argv[])
{
#ifndef _DEBUG
    QLoggingCategory::setFilterRules("qt.network.ssl.warning=false");
#endif
    QApplication app(argc, argv);
    new MainController;
    return app.exec();
}

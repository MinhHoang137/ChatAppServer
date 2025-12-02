#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "header.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    initLog();

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("Server", "Main");

    return app.exec();
}

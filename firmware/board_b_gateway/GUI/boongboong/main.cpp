#include <QGuiApplication>
#include <QFont>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "SerialBridge.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setFont(QFont("Arial"));

    SerialBridge serialBridge;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("serialBridge", &serialBridge);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("boongboong", "Main");

    return QCoreApplication::exec();
}

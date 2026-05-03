#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "controllers/BodyController.h"
#include "controllers/EngineController.h"
#include "controllers/GatewayController.h"
#include "controllers/UdsController.h"
#include "models/AppLogModel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName("cinematown");
    QGuiApplication::setApplicationName("CAN Cluster GUI");
    QQuickStyle::setStyle("Fusion");

    AppLogModel appLog;
    EngineController engine;
    GatewayController gateway;
    BodyController body;
    UdsController uds;

    engine.setLogModel(&appLog);
    gateway.setLogModel(&appLog);
    body.setLogModel(&appLog);
    uds.setLogModel(&appLog);

    QQmlApplicationEngine qmlEngine;
    qmlEngine.rootContext()->setContextProperty("appLog", &appLog);
    qmlEngine.rootContext()->setContextProperty("engine", &engine);
    qmlEngine.rootContext()->setContextProperty("gateway", &gateway);
    qmlEngine.rootContext()->setContextProperty("body", &body);
    qmlEngine.rootContext()->setContextProperty("uds", &uds);

    QObject::connect(&qmlEngine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    qmlEngine.loadFromModule("CanClusterGui", "Main");
    return app.exec();
}

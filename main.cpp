#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "socketconnector.h"
#include "mytreemodel2.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("qainspector");
    app.setOrganizationName("coderus");
    app.setOrganizationDomain("org.coderus");

    QScopedPointer<SocketConnector> connector(new SocketConnector);
    connector->setProperty("applicationName", "inspector");
    qmlRegisterSingletonInstance("org.qaengine.qainspector", 1, 0, "SocketConnector", connector.get());

    qmlRegisterType<MyTreeModel2>("org.qaengine.qainspector", 1, 0, "TreeModel");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("qainspector-qt6", "Main");

    return app.exec();
}

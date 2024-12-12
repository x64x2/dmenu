#if defined(laiin_USE_QT)
#if defined(laiin_USE_QT_WIDGETS)
#include <QApplication> // For QWidget based Qt applications (QWidget depends on QApplication), but we will be using QML so no need for this
#endif
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext> // QQmlContext *	QQmlApplicationEngine::rootContext()
////#include <QQuickView> // we're not using QQuickView for now//#include <QQmlComponent>
#include <QStandardPaths>
#endif

// laiin (includes both the core headers and the gui headers)
#include "../laiin_config.hpp"
#include "../laiin.hpp"
using namespace laiin;
namespace laiin_tools = laiin::tools;

static const QString WALLET_QR_PROVIDER {"wallet_qr"};
static const QString AVATAR_IMAGE_PROVIDER {"avatar"};
static const QString CATALOG_IMAGE_PROVIDER {"listing"};

bool isIOS = false;
bool isAndroid = false;
bool isWindows = false;
bool isMac = false;
bool isLinux = false;
bool isTails = false;
bool isDesktop = false;

int main(int argc, char *argv[])
{
#if defined(laiin_USE_QT)
    std::cout << "Using Qt version: " << qVersion() << "\n";
    // platform dependant settings
    #if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    bool isDesktop = true;
    #elif defined(Q_OS_ANDROID)
    bool isAndroid = true;
    #elif defined(Q_OS_IOS)
    bool isIOS = true;
    #endif
    #ifdef Q_OS_WIN
    bool isWindows = true;
    #elif defined(Q_OS_LINUX)
    bool isLinux = true;
    #elif defined(Q_OS_MAC)
    bool isMac = true;
    #endif    
    // On Linux desktop, use QApplication since the Qt Labs Platform module uses Qt Widgets as a fallback on platforms that do not have a native platform file dialog implementation available. See https://doc.qt.io/qt-5/qml-qt-labs-platform-filedialog.html#availability
    #if defined(laiin_USE_QT_WIDGETS)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    #else
    QGuiApplication app(argc, argv);
    #endif
    app.setApplicationName("laiin");

    qmlRegisterSingletonType<CurrencyExchangeRatesProvider>(
        "laiin",
        1, 0,
        "CurrencyExchangeRatesProvider",
        &CurrencyExchangeRatesProvider::qmlInstance);
    qmlRegisterSingletonType(QUrl("qrc:/qml/components/CurrencyExchangeRates.qml"),
                             "laiin.CurrencyExchangeRates",
                             1,
                             0,
                             "CurrencyExchangeRates");

    QQmlApplicationEngine engine;
    // open config script
    laiin::load_nodes_from_memory();
    
    // create "datastore" folder within "~/.config/laiin/" path
    std::string data_dir = laiin_DEFAULT_DATABASE_PATH;
    if(!laiin::filesystem::is_directory(data_dir)) {
        laiin::print(std::string("Creating directory \"") + data_dir + "\"", 3);
        if(!laiin::filesystem::make_directory(data_dir)) {
            throw std::runtime_error("Failed to create laiin data dir");
            return 1;
        }
    }
    // create "keys" folder within "~/.config/laiin/" path
    std::string keys_dir = laiin_DEFAULT_KEYS_PATH;
    if(!laiin::filesystem::is_directory(keys_dir)) {
        laiin::print(std::string("Creating directory \"") + keys_dir + "\"", 3);
        if(!laiin::filesystem::make_directory(keys_dir)) {
            throw std::runtime_error("Failed to create laiin keys dir");
            return 1;
        }
    }
    // start database
    Backend::initializeDatabase();
    // import paths
    engine.addImportPath(":/assets/fonts"); // import FontAwesome 1.0
    // platform macros
    engine.rootContext()->setContextProperty("isLinux", isLinux);
    engine.rootContext()->setContextProperty("isWindows", isWindows);
    engine.rootContext()->setContextProperty("isMac", isMac);
    engine.rootContext()->setContextProperty("isDesktop", isDesktop);
    engine.rootContext()->setContextProperty("isAndroid", isAndroid);
    engine.rootContext()->setContextProperty("isIOS", isIOS);
    engine.rootContext()->setContextProperty("isTails", isTails);
    // custom macros
    engine.rootContext()->setContextProperty("laiinAppDirPath", QCoreApplication::applicationDirPath());
    engine.rootContext()->setContextProperty("laiinVersion", laiin_VERSION);
    engine.rootContext()->setContextProperty("laiinDataDirPath", QString::fromStdString(laiin_DEFAULT_CONFIGURATION_PATH));
    engine.rootContext()->setContextProperty("laiinDefaultWalletDirPath", QString::fromStdString(laiin_DEFAULT_WALLET_DIRECTORY_PATH));
    // create laiin wallet directory
    if(!laiin::filesystem::is_directory(laiin_DEFAULT_WALLET_DIRECTORY_PATH)) {
        laiin::print(std::string("Creating directory \"") + laiin_DEFAULT_WALLET_DIRECTORY_PATH + "\"", 3);
        if(!laiin::filesystem::make_directory(laiin_DEFAULT_WALLET_DIRECTORY_PATH)) {
            throw std::runtime_error("Failed to create laiin wallet dir");
            return 1;
        }
    }    
    // Create an instance of DaemonManager and expose it to QML
    DaemonManager * daemonManager = new DaemonManager(&engine);
    daemonManager->startDaemonProcessDetached();
    engine.rootContext()->setContextProperty("DaemonManager", daemonManager);
    ProxyManager * proxyManager = new ProxyManager(&engine);
    engine.rootContext()->setContextProperty("ProxyManager", proxyManager);
    engine.setNetworkAccessManagerFactory(proxyManager);
    // we can also register an instance of a class instead of the class itself
    WalletController *wallet = new WalletController(&engine);
    engine.rootContext()->setContextProperty("Wallet", wallet);//new WalletController());//qmlRegisterUncreatableType<WalletProxy>("laiin.Wallet", 1, 0, "Wallet", "Wallet cannot be instantiated directly.");//qmlRegisterType<WalletProxy>("laiin.Wallet", 1, 0, "Wallet"); // Usage: import laiin.Wallet  ...  Wallet { id: wallet }
    qRegisterMetaType<WalletController*>(); // Wallet can now be used as an argument in function parameters
    // register script
    ScriptManager * script_manager = new ScriptManager(&engine);
    engine.rootContext()->setContextProperty("Script", script_manager);
    // register backend
    engine.rootContext()->setContextProperty("Backend", new Backend(&engine));
    // Register user
    engine.rootContext()->setContextProperty("User", new UserController(&engine));
    qRegisterMetaType<UserController *>();
    // Register enums
    qmlRegisterUncreatableType<EnumWrapper>("laiin.Enums", 1, 0, "Enum", "Enums cannot be created.");//laiin::EnumWrapper::registerEnums(&engine);

    engine.addImageProvider(WALLET_QR_PROVIDER, new WalletQrProvider(WALLET_QR_PROVIDER));
    engine.addImageProvider(AVATAR_IMAGE_PROVIDER, new ImageProvider(AVATAR_IMAGE_PROVIDER));
    engine.addImageProvider(CATALOG_IMAGE_PROVIDER, new ImageProvider(CATALOG_IMAGE_PROVIDER));    

    // Load main.qml from the "qml/" directory
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }
    return app.exec(); // starts 'event loop'
    #endif
    return 0;
}

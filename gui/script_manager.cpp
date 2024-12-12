#include "script_manager.hpp"

#include "../core/tools/script.hpp"
#include "../core/settings.hpp" // laiin::lua_state

#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>

laiin::ScriptManager::ScriptManager(QObject *parent) : QObject(parent) {
    laiin::create_json(); // Create settings.json if does not yet exist
    QString json = QString::fromStdString(laiin::load_json());
    QJsonParseError json_error;
    const auto json_doc = QJsonDocument::fromJson(json.toUtf8(), &json_error);
    if(json_error.error != QJsonParseError::NoError) {
        throw std::runtime_error("Error parsing settings.json");
    }
    assert(json_doc.isObject());
    json_object = json_doc.object();
}

//laiin::ScriptManager::~Script() {}

QString laiin::ScriptManager::getString(const QString& key) const {
    return QString::fromStdString(laiin::Script::get_string(laiin::lua_state, key.toStdString()));
}

double laiin::ScriptManager::getNumber(const QString& key) const {
    return laiin::Script::get_number(laiin::lua_state, key.toStdString());
}    

bool laiin::ScriptManager::getBoolean(const QString& key) const {
    return laiin::Script::get_boolean(laiin::lua_state, key.toStdString());
}

QVariantList laiin::ScriptManager::getTableStrings(const QString& key) const {
    std::vector<std::string> table_strings = laiin::Script::get_table_string(laiin::lua_state, key.toStdString());
    QVariantList result;
    for(auto strings : table_strings) {
        result << QString::fromStdString(strings);//std::cout << strings << std::endl;
    }
    return result;
}

laiin::ScriptManager::get_(const QString& key) const {
laiin::Script::get_(laiin::lua_state, key.toStdString());
}

QString laiin::ScriptManager::getJsonString(const QString& key) {
    QJsonValue json_value = json_object.value(key);
    assert(json_value.isString());
    return json_value.toString();
}

int laiin::ScriptManager::getJsonInt(const QString& key) {
    QJsonValue json_value = json_object.value(key);
    //assert(json_value.is?());
    return json_value.toInt();
}

bool laiin::ScriptManager::getJsonBool(const QString& key) {
    QJsonValue json_value = json_object.value(key);
    assert(json_value.isBool());
    return json_value.toBool();
}

double laiin::ScriptManager::getJsonDouble(const QString& key) {
    QJsonValue json_value = json_object.value(key);
    assert(json_value.isDouble());
    return json_value.toDouble();
}

QVariantList laiin::ScriptManager::getJsonArray(const QString& key) {
    QJsonValue json_value = json_object.value(key);
    assert(json_value.isArray());
    QJsonArray json_array = json_value.toArray();
    return json_array.toVariantList();
}

QVariantMap laiin::ScriptManager::getJsonObject(const QString& key) {
    QJsonValue json_value = json_object.value(key);
    assert(json_value.isObject());
    QJsonObject json_obj = json_value.toObject();
    return json_obj.toVariantMap();
}

QVariantMap laiin::ScriptManager::getJsonRootObject() const {
    return json_object.toVariantMap();
}

QString laiin::ScriptManager::getJsonLiteral() {
    QJsonDocument json_doc(json_object);
    return QString(json_doc.toJson(QJsonDocument::Compact));
}

void laiin::ScriptManager::saveJson(const QString& settings) {
    laiin::modify_json(settings.toStdString());
}

QJsonObject laiin::ScriptManager::getJsonRootObjectCpp() const {
    return json_object;
}

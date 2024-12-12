#include "settings.hpp"

#include "../laiin_config.hpp"
#include "tools/logger.hpp"

#if defined(laiin_USE_QT)
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>
#include <QTextStream>
#else
#include <nlohmann/json.hpp>
#endif

#include <filesystem>

namespace laiin {

const static std::string lua_string = R"(monero = {
    nodes = {
        mainnet = {
            "node.community.rino.io:18081",
            "node.sethforprivacy.com:18089",
            "node2.sethforprivacy.com:18089",
            "selsta1.featherwallet.net:18081",
            "selsta2.featherwallet.net:18081",
            "node.monerooutreach.org:18081",
            "node.majesticbank.is:18089",
            "node.majesticbank.su:18089",
            "xmr-node-eu.cakewallet.com:18081",
            "xmr-node-usa-east.cakewallet.com:18081",
            "canada.node.xmr.pm:18089",
            "singapore.node.xmr.pm:18089",
            "nodes.hashvault.pro:18081",
            "node.supportxmr.com:18081",
            "node.xmr.ru:18081"
        },
        stagenet = {
            "http://node.monerodevs.org:38089",
            "http://node2.monerodevs.org:38089",
            "http://node3.monerodevs.org:38089",
            "http://singapore.node.xmr.pm:38081",
            "http://stagenet.community.rino.io:38081",
            "http://stagenet.xmr-tw.org:38081",
            "http://xmr-lux.boldsuck.org:38081",
        },
        testnet = {
            "http://node.monerodevs.org:28089",
            "http://node2.monerodevs.org:28089",
            "http://node3.monerodevs.org:28089",
            "http://testnet.community.rino.io:28081",
            "http://testnet.xmr-tw.org:28081",
        }
    }
}
wownero = {
    nodes = {
        mainnet = {
            "http://88.198.199.23:34568",
            "http://amsterdam.muchwow.lol:34568",
            "http://eu-west-2.wow.xmr.pm:34568",
            "http://idontwanttogototoronto.wow.fail:34568",
            "http://node.monerodevs.org:34568",
            "http://node2.monerodevs.org:34568",
            "http://node.suchwow.xyz:34568",
            "http://nyc.muchwow.lol:34568",
            "http://singapore.muchwow.lol:34568",
            "https://muchwow.lol:34568",
        },
    }
})";

lua_State * lua_state(luaL_newstate());

bool load_lua() {
        ////std::string user = laiin::device::get_user();
        // "/home/<user>/.config/laiin"
        std::string configuration_path = laiin_DEFAULT_CONFIGURATION_PATH;
        std::string configuration_file = laiin_NODES_FILENAME;
        // "/home/<user>/.config/laiin/settings.lua"
        std::string laiin_config_name = configuration_path + "/" + configuration_file;
        Script script;
        if(!script.load(lua_state, laiin_config_name)) {
            return false;
        }
        #ifdef laiin_DEBUG0
        laiin::print("\033[1;94mloaded script \"" + script.get_file() + "\"");
        #endif
        return true;
	}

bool export_lua() {
    ////std::string user = laiin::device::get_user();
    std::string text(lua_string);
        // "/home/<user>/.config/laiin"
        std::string configuration_path = laiin_DEFAULT_CONFIGURATION_PATH;//"/home/" + user + "/.config/laiin";
        std::string configuration_file = laiin_NODES_FILENAME;
        // "/home/<user>/.config/laiin/config.lua"
        std::string laiin_config_name = configuration_path + "/" + configuration_file;
        // if file already exists, no need to create it again
        if(std::filesystem::is_regular_file(laiin_config_name)) return false; // false because it will not be created // if true then it will cause "PANIC: unprotected error in call to Lua API (attempt to index a nil value)" error
        // check if script works before saving
        if(luaL_dostring(lua_state, text.c_str()) != 0) {
		    laiin::print(LUA_TAG "\033[0;91minvalid Lua code");
		    lua_error(lua_state);
		    return false; // exit function so it does not save text
	    }
        // if path does not exist
        if(!std::filesystem::is_directory(configuration_path)) 
        {   // create the path
            laiin::print("directory \"" + configuration_path + "\" does not exist, but I will create it for you (^_^)", 2);
            if(!std::filesystem::create_directories(configuration_path)) { laiin::print("export_lua error: failed to make the path. Sorry (ᵕ人ᵕ)! ...", 1); return false; }
            laiin::print("\033[1;97;49mcreated path \"" + configuration_path + "\"");
        }
        // if path exists, but the file is missing or deleted
        if(!std::filesystem::is_regular_file(laiin_config_name)) {
            // create config file (one time)
            std::ofstream cfg;
            cfg.open (laiin_config_name, std::ios::out | std::ios::trunc);
            cfg << text << "\n"; // write to file
            cfg.close();
            #ifdef laiin_DEBUG0
            laiin::print("\033[1;97;49mcreated file \"" + laiin_config_name + "\"\033[0m");  
            #endif
        }
        return true;		
	}

extern bool open_lua() {
    if(!laiin::export_lua()) { 
        if(!laiin::load_lua()) {
            laiin::print("Failed to load configuration file", 1);
            return false;
        }
    }
    return true;
}

bool load_nodes_from_memory() {
    // Load and compile the Lua code into a Lua function
    int result = luaL_loadstring(lua_state, lua_string.c_str()); // pushes the compiled Lua chunk (function) onto the stack
    if (result != LUA_OK) {
        const char* error_message = lua_tostring(lua_state, -1);
        std::cerr << "Lua compilation error: " << error_message << std::endl;
        lua_pop(lua_state, 1); // Remove the error message from the stack
        lua_close(lua_state);
        return false;
    }
    
    // Execute the Lua function
    result = lua_pcall(lua_state, 0, 0, 0);
    if (result != LUA_OK) {
        const char* error_message = lua_tostring(lua_state, -1);
        std::cerr << "Lua runtime error: " << error_message << std::endl;
        lua_pop(lua_state, 1); // Remove the error message from the stack
        lua_close(lua_state);
        return false;
    }

    return true;    
}

lua_State * get_lua_state() {
	return lua_state;
}

bool create_json() {
    std::string config_path = laiin_DEFAULT_CONFIGURATION_PATH;
    std::string settings_filename = laiin_SETTINGS_FILENAME;
    std::string config_file = config_path + "/" + settings_filename;
    #if defined(laiin_USE_QT)
    // Exit function if file already exists
    if(QFileInfo::exists(QString::fromStdString(config_file)) && QFileInfo(QString::fromStdString(config_file)).isFile()) {
        ////laiin::print(config_file + " already exists", 2);
        return false;
    }
    // If path does not exist, create it
    if(!QDir(QString::fromStdString(config_path)).exists()) {
        laiin::print("directory \"" + config_path + "\" does not exist, but I will create it for you (^_^)", 2);
        if(!QDir().mkdir(QString::fromStdString(config_path))) { laiin::print("create_json error: failed to make the path. Sorry (ᵕ人ᵕ)! ...", 1); return false; }
        laiin::print("\033[1;97;49mcreated path \"" + config_path + "\"");    
    }
    // if path exists, but the file is missing or deleted
    if(!QFileInfo::exists(QString::fromStdString(config_file)) && !QFileInfo(QString::fromStdString(config_file)).isFile()) {
        // Open settings file for writing
        QFile settings_file(QString::fromStdString(config_file));
        if (!settings_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            laiin::print("Error writing to " + settings_filename, 1); return false;
        }
        // Create JSON
        QJsonObject root_obj;
        root_obj.insert(QString("preferred_currency"), QJsonValue("USD"));
        root_obj.insert(QString("dark_theme"), QJsonValue(true));
        root_obj.insert(QString("theme"), QJsonValue("PurpleDust"));
        root_obj.insert(QString("language"), QJsonValue("English"));
        root_obj.insert(QString("hide_homepage_button"), QJsonValue(false));
        root_obj.insert(QString("hide_price_display"), QJsonValue(false));
        root_obj.insert(QString("hide_wallet_sync_bar_on_full"), QJsonValue(true));
        root_obj.insert(QString("wallet_directory"), QJsonValue(""));
        root_obj.insert(QString("last_opened_wallet"), QJsonValue(""));
        root_obj.insert(QString("remember_wallet"), QJsonValue(false));
        /*root_obj.insert(QString("window_width"), QJsonValue(1280));
        root_obj.insert(QString("window_height"), QJsonValue(900));//720));
        root_obj.insert(QString("window_mode"), QJsonValue(0));*/
        
        QJsonObject catalog_obj;
        catalog_obj.insert(QString("price_display"), QJsonValue("All prices"));
        catalog_obj.insert(QString("hide_product_details"), QJsonValue(false));
        catalog_obj.insert(QString("catalog_view"), QJsonValue("Grid view"));
        catalog_obj.insert(QString("grid_details_align_center"), QJsonValue(false));
        catalog_obj.insert(QString("hide_illegal_products"), QJsonValue(true));
        root_obj.insert(QString("catalog"), QJsonValue(catalog_obj));
        
        QJsonObject monero_obj;
        //monero_obj.insert(QString("restore_height"), QJsonValue());
        QJsonObject daemon_obj;
        //daemon_obj.insert(QString("network_type"), QJsonValue("stagenet")); // has no effect when changed manually
        daemon_obj.insert(QString("confirm_external_bind"), QJsonValue(false));
        daemon_obj.insert(QString("restricted_rpc"), QJsonValue(true));
        daemon_obj.insert(QString("data_dir"), QJsonValue(""));
        daemon_obj.insert(QString("auto_sync"), QJsonValue(true));
        daemon_obj.insert(QString("node_type"), QJsonValue(0));
        daemon_obj.insert(QString("executable"), QJsonValue(""));
        daemon_obj.insert(QString("last_selected_node"), QJsonValue(""));
        monero_obj.insert(QString("daemon"), QJsonValue(daemon_obj));
        QJsonObject wallet_obj;
        wallet_obj.insert(QString("balance_display"), QJsonValue("All balances"));
        wallet_obj.insert(QString("balance_amount_precision"), QJsonValue(12));
        wallet_obj.insert(QString("show_currency_sign"), QJsonValue(false));
        wallet_obj.insert(QString("block_explorer"), QJsonValue("xmrchain.net"));
        ////wallet_obj.insert(QString("require_password_on_withdrawal"), QJsonValue(true));
        monero_obj.insert(QString("wallet"), QJsonValue(wallet_obj));
        root_obj.insert(QString("monero"), QJsonValue(monero_obj));
        
        QJsonObject wownero_obj;
        //wownero_obj.insert(QString("restore_height"), QJsonValue());
        QJsonObject daemon2_obj;
        daemon2_obj.insert(QString("confirm_external_bind"), QJsonValue(false));
        daemon2_obj.insert(QString("restricted_rpc"), QJsonValue(true));
        daemon2_obj.insert(QString("data_dir"), QJsonValue(""));
        daemon2_obj.insert(QString("auto_sync"), QJsonValue(true));
        daemon2_obj.insert(QString("node_type"), QJsonValue(0));
        daemon2_obj.insert(QString("executable"), QJsonValue(""));
        daemon2_obj.insert(QString("last_selected_node"), QJsonValue(""));
        wownero_obj.insert(QString("daemon"), QJsonValue(daemon2_obj));
        QJsonObject wallet2_obj;
        wallet2_obj.insert(QString("balance_display"), QJsonValue("All balances"));
        wallet2_obj.insert(QString("balance_amount_precision"), QJsonValue(11));
        wallet2_obj.insert(QString("show_currency_sign"), QJsonValue(false));
        wallet2_obj.insert(QString("block_explorer"), QJsonValue("explore.wownero.com"));
        ////wallet2_obj.insert(QString("require_password_on_withdrawal"), QJsonValue(true));
        wownero_obj.insert(QString("wallet"), QJsonValue(wallet2_obj));
        root_obj.insert(QString("wownero"), QJsonValue(wownero_obj));
        
        QJsonObject data_expiration_obj;
        ////data_expiration_obj.insert(QString("user"), QJsonValue("Never"));
        data_expiration_obj.insert(QString("listing"), QJsonValue("Never"));
        data_expiration_obj.insert(QString("product_rating"), QJsonValue("Never"));
        data_expiration_obj.insert(QString("seller_rating"), QJsonValue("Never"));
        data_expiration_obj.insert(QString("order"), QJsonValue("2 years"));
        data_expiration_obj.insert(QString("message"), QJsonValue("30 days"));
        root_obj.insert(QString("data_expiration"), QJsonValue(data_expiration_obj));
        //QJsonObject _obj;
        //_obj.insert(QString(""), QJsonValue());
        //root_obj.insert(QString(""), QJsonValue(_obj));
        // Convert to JSON string
        QJsonDocument json_doc(root_obj);
        QString settings_json = json_doc.toJson();
        // Write to settings file
        QTextStream out(&settings_file);
        out << settings_json;
        settings_file.close();
        #ifdef laiin_DEBUG0
        std::cout << "settings.json created\n";
        std::cout << settings_json.toStdString() << "\n";
        #endif
    }
    #else
    // Exit function if file already exists
    if(std::filesystem::is_regular_file(config_file)) {
        ////laiin::print(config_file + " already exists", 2);
        return false;
    } 
    // if path does not exist
    if(!std::filesystem::is_directory(config_path)) 
    {   // create the path
        laiin::print("directory \"" + config_path + "\" does not exist, but I will create it for you (^_^)", 2);
        if(!std::filesystem::create_directories(config_path)) { laiin::print("create_config error: failed to make the path. Sorry (ᵕ人ᵕ)! ...", 1); return false; }
        laiin::print("\033[1;97;49mcreated path \"" + config_path + "\"");
    }
    // if path exists, but the file is missing or deleted
    if(!std::filesystem::is_regular_file(config_file)) {
        // Open settings file for writing
        std::ofstream settings_file;
        settings_file.open (config_file, std::ios::out | std::ios::trunc);
        // Create JSON
        nlohmann::json settings_json;
        settings_json["preferred_currency"] = "USD";
        settings_json["dark_theme"] = true;
        settings_json["theme"] = "PurpleDust";
        settings_json["language"] = "English";
        settings_json["hide_homepage_button"] = false;
        settings_json["hide_price_display"] = false;
        settings_json["hide_wallet_sync_bar_on_full"] = true;
        settings_json["wallet_directory"] = ""; // leave blank to use default
        settings_json["last_opened_wallet"] = "";
        settings_json["remember_wallet"] = false;
        /*settings_json["window_width"] = 1280;
        settings_json["window_height"] = 900;//720;
        settings_json["window_mode"] = 0;*/
        //settings_json[""] = ;
        settings_json["catalog"]["price_display"] = "All prices";
        settings_json["catalog"]["hide_product_details"] = false;
        settings_json["catalog"]["catalog_view"] = "Grid view";
        settings_json["catalog"]["grid_details_align_center"] = false;
        settings_json["catalog"]["hide_illegal_products"] = true;
        
        //settings_json["monero"]["daemon"]["network_type"] = "stagenet";
        settings_json["monero"]["daemon"]["confirm_external_bind"] = false;
        settings_json["monero"]["daemon"]["restricted_rpc"] = true;
        settings_json["monero"]["daemon"]["data_dir"] = ""; // leave blank to use default
        settings_json["monero"]["daemon"]["auto_sync"] = true;
        settings_json["monero"]["daemon"]["node_type"] = 0;
        settings_json["monero"]["daemon"]["executable"] = "";
        settings_json["monero"]["daemon"]["last_selected_node"] = "";
        settings_json["monero"]["wallet"]["balance_display"] = "All balances";
        settings_json["monero"]["wallet"]["balance_amount_precision"] = 12;
        settings_json["monero"]["wallet"]["show_currency_sign"] = false;
        settings_json["monero"]["wallet"]["block_explorer"] = "xmrchain.net";
        ////settings_json["monero"]["wallet"]["require_password_on_withdrawal"] = true;
        
        settings_json["wownero"]["daemon"]["confirm_external_bind"] = false;
        settings_json["wownero"]["daemon"]["restricted_rpc"] = true;
        settings_json["wownero"]["daemon"]["data_dir"] = ""; // leave blank to use default
        settings_json["wownero"]["daemon"]["auto_sync"] = true;
        settings_json["wownero"]["daemon"]["node_type"] = 0;
        settings_json["wownero"]["daemon"]["executable"] = "";
        settings_json["wownero"]["daemon"]["last_selected_node"] = "";
        settings_json["wownero"]["wallet"]["balance_display"] = "All balances";
        settings_json["wownero"]["wallet"]["balance_amount_precision"] = 11;
        settings_json["wownero"]["wallet"]["show_currency_sign"] = false;
        settings_json["wownero"]["wallet"]["block_explorer"] = "explore.wownero.com";
        ////settings_json["wownero"]["wallet"]["require_password_on_withdrawal"] = true;
        
        ////settings_json["data_expiration"]["user"] = "Never"; // permanent
        settings_json["data_expiration"]["listing"] = "Never"; // can be modified
        settings_json["data_expiration"]["product_rating"] = "Never";
        settings_json["data_expiration"]["seller_rating"] = "Never";
        settings_json["data_expiration"]["order"] = "2 years"; // until order is completed or fails
        settings_json["data_expiration"]["message"] = "30 days"; // can be modified
        //settings_json[""][""] = ;
        // Write to file then close it
        settings_file << settings_json.dump(4);
        settings_file.close();
        #ifdef laiin_DEBUG0
        laiin::print("\033[1;97;49mcreated file \"" + config_file + "\"\033[0m");        
        std::cout << settings_json.dump(4) << "\n";
        #endif
    }
    #endif    
    return true;
}

std::string load_json() {
    std::string config_path = laiin_DEFAULT_CONFIGURATION_PATH;
    std::string settings_filename = laiin_SETTINGS_FILENAME;
    std::string config_file = config_path + "/" + settings_filename;
    #if defined(laiin_USE_QT)
    // Open settings file for reading
    QFile settings_file(QString::fromStdString(config_file));
    if (!settings_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        laiin::print("Error reading from " + settings_filename, 1);
        // The file most likely does not exist so we must create it
        return "";
    }
    // Validate the JSON    
    QJsonParseError json_error;
    const auto json_doc = QJsonDocument::fromJson(settings_file.readAll(), &json_error);
    if (json_error.error != QJsonParseError::NoError) {
        laiin::print("Error parsing " + settings_filename, 1);
        settings_file.close();
        return "";
    }    
    settings_file.close();
    // Create object from JSON or just return the JSON string
    return json_doc.toJson(QJsonDocument::Indented).toStdString();
    #else
    std::ifstream file(config_file);
    if(!file.is_open()) {
        laiin::print("Error reading from " + settings_filename, 1); return "";
    }
    nlohmann::json j = nlohmann::json::parse(file);
    if (j.is_discarded()) {
        laiin::print("Error parsing " + settings_filename, 1);
        file.close();
        return "";
    }
    file.close();
    return j.dump(4);
    #endif    
    return "";
}

bool open_json(std::string& out) {
    if(!create_json()) {
        std::string out = load_json();
        if(out.empty()) {
            laiin::print("Failed to load settings.json", 1);
            return false;
        }
    }
    return true;
}

void modify_json(const std::string& settings) { // saves settings
    std::string config_path = laiin_DEFAULT_CONFIGURATION_PATH;
    std::string settings_filename = laiin_SETTINGS_FILENAME;
    std::string config_file = config_path + "/" + settings_filename;
    #if defined(laiin_USE_QT)
    // Validate the JSON
    QJsonParseError json_error;
    const auto json_doc = QJsonDocument::fromJson(QString::fromStdString(settings).toUtf8(), &json_error);
    if(json_error.error != QJsonParseError::NoError) {
        laiin::print("Error parsing " + settings_filename, 1); return;
    }
    // Format the JSON
    QString settings_json = json_doc.toJson(QJsonDocument::Indented);
    // Open settings file for writing
    QFile settings_file(QString::fromStdString(config_file));
    if (!settings_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        laiin::print("Error writing to " + settings_filename, 1); return;
    }
    // Write to settings file
    QTextStream out(&settings_file);
    out << settings_json;//out << settings;
    settings_file.close();
    #else
    nlohmann::json j = nlohmann::json::parse(settings, nullptr, false);
    if (j.is_discarded()) {
        laiin::print("Error parsing " + settings_filename, 1); return;
    }
    std::string settings_json = j.dump(4);
    // Open file for writing
    std::ofstream file(config_file);
    if(!file.is_open()) {
        laiin::print("Error reading from " + settings_filename, 1); return;
    }    
    file << settings_json;
    file.close();
    #endif    
}

}


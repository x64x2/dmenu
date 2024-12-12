// Copyright (c) 2021-2024 the laiin core team
// Distributed under the GPLv3 software license, see the accompanying
// file LICENSE or https://opensource.org/license/gpl-3-0/.

#pragma once

#ifndef CONFIG_HPP_laiin
#define CONFIG_HPP_laiin

#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility> // std::pair

#if defined(laiin_USE_QT)
#include <QStandardPaths>
#else
#include "core/tools/device.hpp"
#endif

#define laiin_IPC_DEFAULT_PORT 57740
// This port will be used by the daemon to establish connections with p2p network
#define laiin_P2P_DEFAULT_PORT 50881 // Use ports between 49152-65535 that are not currently registered with IANA and are rarely used
// This port will allow outside clients to interact with laiin daemon RPC server
#define laiin_RPC_DEFAULT_PORT 50882

#define laiin_LOOPBACK_ADDRESS            "127.0.0.1"
#define laiin_ANY_ADDRESS                 "0.0.0.0"

#define laiin_RECV_BUFFER_SIZE            4096//8192// no IP packet can be above 64000 (64 KB), not even with fragmentation, thus recv on an UDP socket can at most return 64 KB (and what is not returned is discarded for the current packet!)

#define laiin_DHT_REPLICATION_FACTOR      3 // 10 to 20 (or even higher) // Usually 3 or 5 but a higher number would improve fault tolerant, mitigating the risk of data loss even if multiple nodes go offline simultaneously. It also helps distribute the load across more nodes, potentially improving read performance by allowing concurrent access from multiple replicas.
#define laiin_DHT_MAX_CLOSEST_NODES       20 // 50 to 100 (or even higher)
#define laiin_DHT_RECV_TIMEOUT            5 // A reasonable query recv timeout value for a DHT node could be between 5 to 30 seconds.
#define laiin_DHT_PING_TIMEOUT            2
#define laiin_DHT_ROUTING_TABLE_BUCKETS   256 // Recommended to use a number of buckets that is equal to the number of bits in the node id (in this case, sha-3-256 so 256 bits)
#define laiin_DHT_NODES_PER_BUCKET        20 // Each bucket should hold up to 20 nodes (same number as k closest nodes)
#define laiin_DHT_MAX_ROUTING_TABLE_NODES laiin_DHT_ROUTING_TABLE_BUCKETS * laiin_DHT_NODES_PER_BUCKET // = 5120
#define laiin_DHT_MAX_HEALTH_CHECKS       3 // Maximum number of consecutive failed checks before marking the node as dead
#define laiin_DHT_NODE_HEALTH_CHECK_INTERVAL 300 // Number of seconds between each node health check
#define laiin_DHT_DATA_REPUBLISH_INTERVAL    3600 // Number of seconds between each republishing of in-memory hash table data
#define laiin_DHT_DATA_REMOVAL_INTERVAL      1800 // Number of seconds between each removal of all expired in-memory hash table data
#define laiin_DHT_BUCKET_REFRESH_INTERVAL    3600 // Number of seconds between each find_node query to neighboring nodes

#define laiin_MAX_SEARCH_RESULTS          1000

#define laiin_PRIVATE_KEY_FILENAME             "<user_id>.key"
#define laiin_PUBLIC_KEY_FILENAME              "<user_id>.pub"
#define laiin_OPENSSL_PRIVATE_KEY_FILENAME     "<user_id>.pem"
#define laiin_PGP_PRIVATE_KEY_FILENAME         "<user_id>.pgp"
#define laiin_PGP_ARMORED_PRIVATE_KEY_FILENAME "<user_id>.asc"

#define laiin_RSA_DEFAULT_BIT_LENGTH 4096
#define laiin_RSA_DEFAULT_BITS       laiin_RSA_DEFAULT_BIT_LENGTH

#define laiin_APPLICATION_NAME       "laiin"
#define laiin_DAEMON_CONFIG_FILENAME "daemon.ini"
#define laiin_DATABASE_FILENAME      "data.sqlite3"
#define laiin_SETTINGS_FILENAME      "settings.json"
#define laiin_NODES_FILENAME         "nodes.lua"
#define laiin_LOG_FILENAME           "laiin.log"
#define laiin_USERDATA_FILENAME      "user.sqlite3"

#define laiin_KEYS_FOLDER_NAME    "keys"
#define laiin_DATA_FOLDER_NAME    "datastore"
#define laiin_CATALOG_FOLDER_NAME "listings"
#define laiin_AVATAR_FOLDER_NAME  "avatars"

#define laiin_MAX_IMAGE_SIZE      2097152 // 2 MB
#define laiin_MIN_USERNAME_LENGTH 2
#define laiin_MAX_USERNAME_LENGTH 30

#if defined(laiin_USE_QT)
#define laiin_DEFAULT_CONFIGURATION_PATH    QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation).toStdString()//(QStandardPaths::AppLocalDataLocation).toStdString()
#define laiin_DEFAULT_DATABASE_PATH         laiin_DEFAULT_CONFIGURATION_PATH + "/" + laiin_DATA_FOLDER_NAME
#define laiin_DEFAULT_KEYS_PATH             laiin_DEFAULT_CONFIGURATION_PATH + "/" + laiin_KEYS_FOLDER_NAME
#if defined(_WIN32)
#define laiin_DEFAULT_WALLET_DIRECTORY_PATH (QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/laiin/wallet").toStdString()
#else
#define laiin_DEFAULT_WALLET_DIRECTORY_PATH (QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/laiin/wallet").toStdString()
#endif
#endif // endif laiin_USE_QT

#if !defined(laiin_USE_QT)
#if defined(_WIN32)
#define laiin_DEFAULT_CONFIGURATION_PATH "C:/Users/" + laiin::device::get_user() + "/AppData/Local/laiin"//"C:/ProgramData/laiin"
#define laiin_DEFAULT_WALLET_DIRECTORY_PATH "C:/Users/" + laiin::device::get_user() + "/Documents" + "/laiin/wallet"
#endif

#if defined(__linux__) && !defined(__ANDROID__)
#define laiin_DEFAULT_CONFIGURATION_PATH "/home/" + laiin::device::get_user() + "/.config/laiin"//"/etc/xdg/laiin"//"/home/" + laiin::device::get_user() + "/.local/share/laiin"
#define laiin_DEFAULT_WALLET_DIRECTORY_PATH "/home/" + laiin::device::get_user() + "/laiin/wallet"
#endif

#if defined(__APPLE__) && defined(__MACH__)
#define laiin_DEFAULT_CONFIGURATION_PATH "~/Library/Preferences/laiin"
#endif

#if defined(__ANDROID__)
#define laiin_DEFAULT_CONFIGURATION_PATH "<APPROOT>/files/settings"
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#define laiin_DEFAULT_CONFIGURATION_PATH "<APPROOT>/Library/Preferences/laiin"
#endif
#endif

#define laiin_DEFAULT_DATABASE_PATH                 laiin_DEFAULT_CONFIGURATION_PATH + "/" + laiin_DATA_FOLDER_NAME
#define laiin_DEFAULT_KEYS_PATH                     laiin_DEFAULT_CONFIGURATION_PATH + "/" + laiin_KEYS_FOLDER_NAME
#endif // endif NOT laiin_USE_QT

namespace laiin {

const std::initializer_list<std::pair<std::string, uint16_t>> BOOTSTRAP_NODES = {
    {"node.laiin.org", laiin_P2P_DEFAULT_PORT},
    //{"127.0.0.1", laiin_P2P_DEFAULT_PORT}
};

}

#endif

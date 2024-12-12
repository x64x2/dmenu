#pragma once
#ifndef DATABASE_HPP_laiin
#define DATABASE_HPP_laiin

#include "sqlite3/sqlite3.hpp"

namespace laiin {

db::Sqlite3 * get_database();
db::Sqlite3 * get_user_database();

}
#endif

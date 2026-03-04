#pragma once
#include "../thirdparty/httplib.h"
#include "../thirdparty/json.hpp"
#include "../headers/odbc_db.hpp"

using json = nlohmann::json;

void handle_ip_pool(OdbcDb& db, const httplib::Request& req, httplib::Response& res);
void handle_reserve_ip(OdbcDb& db, const httplib::Request& req, httplib::Response& res, int ttlSeconds);
void handle_assign(OdbcDb& db, const httplib::Request& req, httplib::Response& res);
void handle_terminate(OdbcDb& db, const httplib::Request& req, httplib::Response& res);
void handle_serviceid_change(OdbcDb& db, const httplib::Request& req, httplib::Response& res);
void handle_get_by_serviceid(OdbcDb& db, const httplib::Request& req, httplib::Response& res);

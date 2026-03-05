#include "../thirdparty/httplib.h"
#include "../headers/handlers.hpp"
#include "../headers/odbc_db.hpp"
#include "../headers/logger.hpp"
#include "../headers/cleanup.hpp"
#include <cstdlib>
#include <iostream>
#include <csignal>
#include <winsock2.h>


static httplib::Server* g_server = NULL;

static void signal_handler(int) {
    if (g_server) g_server->stop();
}

int main() {
    WSADATA wsa{};
    int WSAresult = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (WSAresult != 0) {
        std::cerr << "WSAStartup failed with error: " << WSAresult << std::endl;
        return 1;
    }

    //we have some default values in case if issues with the CmakePresets json

    const char* odbcEnv = std::getenv("IPINV_ODBC");
    std::string odbcConn = odbcEnv ? odbcEnv :
        "Driver={PostgreSQL Unicode(x64)};"
        "Server=localhost;Port=5433;"
        "Database=ipinv;Uid=postgres;Pwd=postgres;";

    const char* portEnv = std::getenv("IPINV_PORT");
    int port = portEnv ? std::atoi(portEnv) : 8080;

    const char* ttlEnv = std::getenv("IPINV_RESERVE_TTL_SEC");
    int ttl = ttlEnv ? std::atoi(ttlEnv) : 600;

    const char* cleanupEnv = std::getenv("IPINV_CLEANUP_INTERVAL_SEC");
    int cleanupInterval = cleanupEnv ? std::atoi(cleanupEnv) : 30;

    OdbcDb db(odbcConn);
    CleanupJob job(db, cleanupInterval);
    job.start();
    httplib::Server svr;
    g_server = &svr;
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    svr.set_default_headers({//look up more info on CORS
    {"Access-Control-Allow-Origin", "*"},
    {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
    {"Access-Control-Allow-Headers", "Content-Type"}
        });

    // Handle preflight OPTIONS requests
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
        });

    svr.Post("/ip-inventory/ip-pool",
        [&db](const httplib::Request& req, httplib::Response& res) {
            handle_ip_pool(db, req, res);
        });

    svr.Post("/ip-inventory/reserve-ip",
        [&db, ttl](const httplib::Request& req, httplib::Response& res) {
            handle_reserve_ip(db, req, res, ttl);
        });

    svr.Post("/ip-inventory/assign-ip-serviceId",
        [&db](const httplib::Request& req, httplib::Response& res) {
            handle_assign(db, req, res);
        });

    svr.Post("/ip-inventory/terminate-ip-serviceId",
        [&db](const httplib::Request& req, httplib::Response& res) {
            handle_terminate(db, req, res);
        });

    svr.Post("/ip-inventory/serviceId-change",
        [&db](const httplib::Request& req, httplib::Response& res) {
            handle_serviceid_change(db, req, res);
        });

    svr.Get("/ip-inventory/serviceId",
        [&db](const httplib::Request& req, httplib::Response& res) {
            handle_get_by_serviceid(db, req, res);
        });

    std::cout << "API listening on http://localhost:" << port << std::endl;
    svr.listen("0.0.0.0", port);

    std::cout << "Shutting down..." << std::endl;
	job.stop();
    WSACleanup();
    return 0;
}

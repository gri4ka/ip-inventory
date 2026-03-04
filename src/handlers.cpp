#include "../headers/handlers.hpp"
#include "../headers/ip_util.hpp"
#include "../headers/logger.hpp"
static void json_ok(httplib::Response& res, const json& j) {
    res.status = 200;
    res.set_content(j.dump(), "application/json");
}

static void json_err(httplib::Response& res, int code, const std::string& msg) {
    json j;
    j["statusCode"] = code;
    j["statusMessage"] = msg;
    res.status = code;
    res.set_content(j.dump(), "application/json");
}

static json success_response() {
    json j;
    j["statusCode"] = "0";
    j["statusMessage"] = "Successful operation. OK";
    return j;
}

static json ip_list_response(const std::vector<IpRow>& rows) {
    json j;
    j["ipAddresses"] = json::array();
    for (size_t i = 0; i < rows.size(); ++i) {
        json entry;
        entry["ip"] = rows[i].ip;
        entry["ipType"] = rows[i].ip_type;
        j["ipAddresses"].push_back(entry);
    }
    return j;
}

// POST /ip-inventory/ip-pool
void handle_ip_pool(OdbcDb& db, const httplib::Request& req, httplib::Response& res) {
    try {
        json body = json::parse(req.body);
        if (!body.contains("ipAddresses") || !body["ipAddresses"].is_array())
            return json_err(res, 400, "Missing ipAddresses[]");

        std::vector<std::pair<std::string, std::string> > ips;
        for (size_t i = 0; i < body["ipAddresses"].size(); ++i) {
            const json& el = body["ipAddresses"][i];
            if (!el.contains("ip") || !el.contains("ipType"))
                return json_err(res, 400, "Each item must have ip and ipType");

            std::string ip = el["ip"];
            std::string ipType = el["ipType"];
            if (ipType != "IPv4" && ipType != "IPv6")
                return json_err(res, 400, "ipType must be IPv4 or IPv6");

            std::string canon;
            if (!normalize_ip(ipType, ip, canon))
                return json_err(res, 400, "Invalid IP: " + ip);

            ips.push_back(std::make_pair(canon, ipType));
        }

        db.add_ip_pool(ips);
        json_ok(res, success_response());
    }
    catch (const std::exception& e) {
        json_err(res, 400, e.what());
    }
}

// POST /ip-inventory/reserve-ip
void handle_reserve_ip(OdbcDb& db, const httplib::Request& req, httplib::Response& res, int ttlSeconds) {
    try {
        Logger::log(Logger::Level::L_DEBUG, "Assign request body: " + req.body);
        json body = json::parse(req.body);
        if (!body.contains("serviceId") || !body.contains("ipType"))
            return json_err(res, 400, "Missing serviceId or ipType");

        std::string serviceId = body["serviceId"];
        std::string ipType = body["ipType"];
        if (ipType != "IPv4" && ipType != "IPv6" && ipType != "Both")
            return json_err(res, 400, "ipType must be IPv4, IPv6, or Both");

        std::vector<IpRow> rows = db.reserve_ips(serviceId, ipType, ttlSeconds);
        json_ok(res, ip_list_response(rows));
    }
    catch (const std::exception& e) {
        std::string msg = e.what();
        int code = (msg.find("Not enough free IPs") != std::string::npos) ? 422 : 400;
        json_err(res, code, msg);
    }
}

// POST /ip-inventory/assign-ip-serviceId
void handle_assign(OdbcDb& db, const httplib::Request& req, httplib::Response& res) {
    try {
		
        json body = json::parse(req.body);
        if (!body.contains("serviceId") || !body.contains("ipAddresses"))
            return json_err(res, 400, "Missing serviceId or ipAddresses");

        std::string serviceId = body["serviceId"];
        std::vector<std::string> ips;
        for (size_t i = 0; i < body["ipAddresses"].size(); ++i)
            ips.push_back(body["ipAddresses"][i]["ip"]);

        db.assign_ips(serviceId, ips);
        json_ok(res, success_response());
    }
    catch (const std::exception& e) {
        json_err(res, 409, e.what());
    }
}

// POST /ip-inventory/terminate-ip-serviceId
void handle_terminate(OdbcDb& db, const httplib::Request& req, httplib::Response& res) {
    try {
        json body = json::parse(req.body);
        if (!body.contains("serviceId") || !body.contains("ipAddresses"))
            return json_err(res, 400, "Missing serviceId or ipAddresses");

        std::string serviceId = body["serviceId"];
        std::vector<std::string> ips;
        for (size_t i = 0; i < body["ipAddresses"].size(); ++i)
            ips.push_back(body["ipAddresses"][i]["ip"]);

        db.terminate_ips(serviceId, ips);
        json_ok(res, success_response());
    }
    catch (const std::exception& e) {
        json_err(res, 400, e.what());
    }
}

// POST /ip-inventory/serviceId-change
void handle_serviceid_change(OdbcDb& db, const httplib::Request& req, httplib::Response& res) {
    try {
        json body = json::parse(req.body);
        if (!body.contains("serviceIdOld") || !body.contains("serviceId"))
            return json_err(res, 400, "Missing serviceIdOld or serviceId");

        db.change_service_id(body["serviceIdOld"], body["serviceId"]);
        json_ok(res, success_response());
    }
    catch (const std::exception& e) {
        json_err(res, 400, e.what());
    }
}

// GET /ip-inventory/serviceId?serviceId=xxxyyy
void handle_get_by_serviceid(OdbcDb& db, const httplib::Request& req, httplib::Response& res) {
    std::string sid = req.get_param_value("serviceId");
    if (sid.empty()) return json_err(res, 400, "Missing query param serviceId");

    try {
        std::vector<IpRow> rows = db.get_ips_for_service(sid);
        json_ok(res, ip_list_response(rows));
    }
    catch (const std::exception& e) {
        json_err(res, 400, e.what());
    }
}

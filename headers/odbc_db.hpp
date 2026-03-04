#pragma once
#include <string>
#include <vector>
#include <utility>

struct IpRow {
    std::string ip;
    std::string ip_type;
    std::string status;
};

class OdbcDb {
public:
    OdbcDb(const std::string& connStr);

    int release_expired_reservations();
    void add_ip_pool(const std::vector<std::pair<std::string,std::string> >& ips);
    std::vector<IpRow> reserve_ips(const std::string& serviceId, const std::string& ipType, int ttlSeconds);
    void assign_ips(const std::string& serviceId, const std::vector<std::string>& ips);
    void terminate_ips(const std::string& serviceId, const std::vector<std::string>& ips);
    int change_service_id(const std::string& oldId, const std::string& newId);
    std::vector<IpRow> get_ips_for_service(const std::string& serviceId);

private:
    std::string m_connStr;
};

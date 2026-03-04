#include "../headers/odbc_db.hpp"
#include "../headers/odbc_common.hpp"
#include <stdexcept>
#include <sstream>

// Run a simple SQL string with no parameters or results.
static void exec(SQLHDBC dbc, const std::string& sql) {
    OdbcStmt st(dbc);
    SQLRETURN rc = SQLExecDirectA(st.stmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    if (!SQL_SUCCEEDED(rc))
        throw std::runtime_error("SQL failed: " + odbc_diag(SQL_HANDLE_STMT, st.stmt));
}

// Bind a string to a parameter position. Caller must keep param and len alive until execute.
static void bind_str(SQLHSTMT stmt, int pos, const std::string& param, SQLLEN& len) {
    len = (SQLLEN)param.size();
    SQLBindParameter(stmt, (SQLUSMALLINT)pos, SQL_PARAM_INPUT,
                     SQL_C_CHAR, SQL_VARCHAR, 0, 0,
                     (SQLPOINTER)param.c_str(), 0, &len);
}

// Fetch all (ip, ip_type, status) rows from an executed statement.
static std::vector<IpRow> fetch_rows(SQLHSTMT stmt) {
    char ip[128]{}, type[16]{}, status[16]{};
    SQLLEN ipInd{}, typeInd{}, stInd{};
    SQLBindCol(stmt, 1, SQL_C_CHAR, ip, sizeof(ip), &ipInd);
    SQLBindCol(stmt, 2, SQL_C_CHAR, type, sizeof(type), &typeInd);
    SQLBindCol(stmt, 3, SQL_C_CHAR, status, sizeof(status), &stInd);

    std::vector<IpRow> rows;
    while (SQLFetch(stmt) != SQL_NO_DATA) {
        IpRow r;
        r.ip = ip;
        r.ip_type = type;
        r.status = status;
        rows.push_back(r);
    }
    return rows;
}

OdbcDb::OdbcDb(const std::string& connStr) : m_connStr(connStr) {}

// ---------------------------------------------------------------------------

int OdbcDb::release_expired_reservations() {
    OdbcEnv env;
    OdbcConn conn(env.env);
    conn.connect(m_connStr);

    OdbcStmt st(conn.dbc);
    SQLRETURN rc = SQLExecDirectA(st.stmt, (SQLCHAR*)
        "UPDATE ip_address "
        "SET status='FREE', service_id=NULL, expires_at=NULL "
        "WHERE status='RESERVED' AND expires_at <= now()", SQL_NTS);
    if (!SQL_SUCCEEDED(rc))
        throw std::runtime_error("cleanup failed: " + odbc_diag(SQL_HANDLE_STMT, st.stmt));

    SQLLEN rows{};
    SQLRowCount(st.stmt, &rows);
    exec(conn.dbc, "COMMIT");
    return (int)rows;
}

// ---------------------------------------------------------------------------

void OdbcDb::add_ip_pool(const std::vector<std::pair<std::string,std::string> >& ips) {
    OdbcEnv env;
    OdbcConn conn(env.env);
    conn.connect(m_connStr);

    for (size_t i = 0; i < ips.size(); ++i) {
        OdbcStmt st(conn.dbc);
        SQLPrepareA(st.stmt, (SQLCHAR*)
            "INSERT INTO ip_address(ip, ip_type, status, service_id, expires_at) "
            "VALUES (?, ?, 'FREE', NULL, NULL) "
            "ON CONFLICT (ip) DO NOTHING", SQL_NTS);

        SQLLEN ipLen{}, typeLen{};
        bind_str(st.stmt, 1, ips[i].first,  ipLen);
        bind_str(st.stmt, 2, ips[i].second, typeLen);

        SQLRETURN rc = SQLExecute(st.stmt);
        if (!SQL_SUCCEEDED(rc))
            throw std::runtime_error("insert failed: " + odbc_diag(SQL_HANDLE_STMT, st.stmt));
    }
    exec(conn.dbc, "COMMIT");
}

// ---------------------------------------------------------------------------

std::vector<IpRow> OdbcDb::reserve_ips(const std::string& serviceId,
                                        const std::string& ipType,
                                        int ttlSeconds) {
    OdbcEnv env;
    OdbcConn conn(env.env);
    conn.connect(m_connStr);
    SQLHDBC dbc = conn.dbc;

    // Clean up expired reservations first
    exec(dbc, "UPDATE ip_address SET status='FREE', service_id=NULL, expires_at=NULL "
              "WHERE status='RESERVED' AND expires_at <= now()");

    // Check if service already has ASSIGNED IPs
    {
        OdbcStmt st(dbc);
        std::string q = "SELECT ip, ip_type, status FROM ip_address "
                        "WHERE service_id=? AND status='ASSIGNED'";
        if (ipType != "Both") q += " AND ip_type=?";
        q += " ORDER BY ip_type, ip";

        SQLPrepareA(st.stmt, (SQLCHAR*)q.c_str(), SQL_NTS);
        SQLLEN sidLen{}, typeLen{};
        bind_str(st.stmt, 1, serviceId, sidLen);
        if (ipType != "Both") bind_str(st.stmt, 2, ipType, typeLen);

        SQLExecute(st.stmt);
        std::vector<IpRow> rows = fetch_rows(st.stmt);
        if (!rows.empty()) { exec(dbc, "COMMIT"); return rows; }
    }

    // Check if service already has active RESERVED IPs
    {
        OdbcStmt st(dbc);
        std::string q = "SELECT ip, ip_type, status FROM ip_address "
                        "WHERE service_id=? AND status='RESERVED' AND expires_at > now()";
        if (ipType != "Both") q += " AND ip_type=?";
        q += " ORDER BY ip_type, ip";

        SQLPrepareA(st.stmt, (SQLCHAR*)q.c_str(), SQL_NTS);
        SQLLEN sidLen{}, typeLen{};
        bind_str(st.stmt, 1, serviceId, sidLen);
        if (ipType != "Both") bind_str(st.stmt, 2, ipType, typeLen);

        SQLExecute(st.stmt);
        std::vector<IpRow> rows = fetch_rows(st.stmt);
        if (!rows.empty()) { exec(dbc, "COMMIT"); return rows; }
    }

    // Reserve a free IP of the given type
    auto reserve_one = [&](const std::string& type) -> IpRow {
        OdbcStmt st(dbc);
        SQLPrepareA(st.stmt, (SQLCHAR*)
            "UPDATE ip_address "
            "SET status='RESERVED', service_id=?, "
            "    expires_at=now() + (? || ' seconds')::interval "
            "WHERE ip = ("
            "  SELECT ip FROM ip_address "
            "  WHERE status='FREE' AND ip_type=? "
            "  ORDER BY ip LIMIT 1 "
            "  FOR UPDATE SKIP LOCKED"
            ") RETURNING ip, ip_type, status", SQL_NTS);

        std::ostringstream oss;
        oss << ttlSeconds;
        std::string ttlStr = oss.str();

        SQLLEN sidLen{}, ttlLen{}, typeLen{};
        bind_str(st.stmt, 1, serviceId, sidLen);
        bind_str(st.stmt, 2, ttlStr,    ttlLen);
        bind_str(st.stmt, 3, type,      typeLen);

        SQLRETURN rc = SQLExecute(st.stmt);
        if (!SQL_SUCCEEDED(rc))
            throw std::runtime_error("reserve failed: " + odbc_diag(SQL_HANDLE_STMT, st.stmt));

        std::vector<IpRow> rows = fetch_rows(st.stmt);
        if (rows.empty())
            throw std::runtime_error("Not enough free IPs for " + type);
        return rows[0];
    };

    std::vector<IpRow> result;
    if (ipType == "Both") {
        result.push_back(reserve_one("IPv4"));
        result.push_back(reserve_one("IPv6"));
    } else {
        result.push_back(reserve_one(ipType));
    }

    exec(dbc, "COMMIT");
    return result;
}

// ---------------------------------------------------------------------------

void OdbcDb::assign_ips(const std::string& serviceId, const std::vector<std::string>& ips) {
    OdbcEnv env;
    OdbcConn conn(env.env);
    conn.connect(m_connStr);

    for (size_t i = 0; i < ips.size(); ++i) {
        OdbcStmt st(conn.dbc);
        SQLPrepareA(st.stmt, (SQLCHAR*)
            "UPDATE ip_address "
            "SET status='ASSIGNED', expires_at=NULL "
            "WHERE ip=? AND ("
            " (status='RESERVED' AND service_id=? AND expires_at > now()) "
            " OR (status='ASSIGNED' AND service_id=?)"
            ")", SQL_NTS);

        SQLLEN ipLen{}, sid1Len{}, sid2Len{};
        bind_str(st.stmt, 1, ips[i],    ipLen);
        bind_str(st.stmt, 2, serviceId, sid1Len);
        bind_str(st.stmt, 3, serviceId, sid2Len);

        SQLRETURN rc = SQLExecute(st.stmt);
        if (!SQL_SUCCEEDED(rc))
            throw std::runtime_error("assign failed: " + odbc_diag(SQL_HANDLE_STMT, st.stmt));

        SQLLEN rows{};
        SQLRowCount(st.stmt, &rows);
        if (rows == 0)
            throw std::runtime_error("Assign rejected for: " + ips[i]);
    }
    exec(conn.dbc, "COMMIT");
}

// ---------------------------------------------------------------------------

void OdbcDb::terminate_ips(const std::string& serviceId, const std::vector<std::string>& ips) {
    OdbcEnv env;
    OdbcConn conn(env.env);
    conn.connect(m_connStr);

    for (size_t i = 0; i < ips.size(); ++i) {
        OdbcStmt st(conn.dbc);
        SQLPrepareA(st.stmt, (SQLCHAR*)
            "UPDATE ip_address "
            "SET status='FREE', service_id=NULL, expires_at=NULL "
            "WHERE ip=? AND service_id=? AND status IN ('RESERVED','ASSIGNED')", SQL_NTS);

        SQLLEN ipLen{}, sidLen{};
        bind_str(st.stmt, 1, ips[i],    ipLen);
        bind_str(st.stmt, 2, serviceId, sidLen);

        SQLRETURN rc = SQLExecute(st.stmt);
        if (!SQL_SUCCEEDED(rc))
            throw std::runtime_error("terminate failed: " + odbc_diag(SQL_HANDLE_STMT, st.stmt));
    }
    exec(conn.dbc, "COMMIT");
}

// ---------------------------------------------------------------------------

int OdbcDb::change_service_id(const std::string& oldId, const std::string& newId) {
    OdbcEnv env;
    OdbcConn conn(env.env);
    conn.connect(m_connStr);

    OdbcStmt st(conn.dbc);
    SQLPrepareA(st.stmt, (SQLCHAR*)
        "UPDATE ip_address SET service_id=? "
        "WHERE service_id=? AND status IN ('RESERVED','ASSIGNED')", SQL_NTS);

    SQLLEN newLen{}, oldLen{};
    bind_str(st.stmt, 1, newId, newLen);
    bind_str(st.stmt, 2, oldId, oldLen);

    SQLRETURN rc = SQLExecute(st.stmt);
    if (!SQL_SUCCEEDED(rc))
        throw std::runtime_error("serviceId-change failed: " + odbc_diag(SQL_HANDLE_STMT, st.stmt));

    SQLLEN rows{};
    SQLRowCount(st.stmt, &rows);
    exec(conn.dbc, "COMMIT");
    return (int)rows;
}

// ---------------------------------------------------------------------------

std::vector<IpRow> OdbcDb::get_ips_for_service(const std::string& serviceId) {
    OdbcEnv env;
    OdbcConn conn(env.env);
    conn.connect(m_connStr);

    OdbcStmt st(conn.dbc);
    SQLPrepareA(st.stmt, (SQLCHAR*)
        "SELECT ip, ip_type, status FROM ip_address "
        "WHERE service_id=? ORDER BY ip_type, ip", SQL_NTS);

    SQLLEN sidLen{};
    bind_str(st.stmt, 1, serviceId, sidLen);

    SQLExecute(st.stmt);
    std::vector<IpRow> rows = fetch_rows(st.stmt);
    exec(conn.dbc, "COMMIT");
    return rows;
}
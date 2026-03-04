#include "../headers/odbc_common.hpp"
#include <sstream>
#include <stdexcept>
#include <iostream>

std::string odbc_diag(SQLSMALLINT htype, SQLHANDLE h) {
    SQLCHAR state[6]{};
    SQLCHAR msg[512]{};
    SQLINTEGER native = 0;
    SQLSMALLINT len = 0;
    std::ostringstream oss;

    for (SQLSMALLINT i = 1;; ++i) {
        SQLRETURN rc = SQLGetDiagRecA(htype, h, i, state, &native, msg, sizeof(msg), &len);
        if (rc == SQL_NO_DATA) break;
        if (SQL_SUCCEEDED(rc)) {
            oss << "[" << (const char*)state << "] " << (const char*)msg << " ";
        }
    }
    return oss.str();
}


OdbcEnv::OdbcEnv() : env(SQL_NULL_HENV) {
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env)))
        throw std::runtime_error("SQLAllocHandle ENV failed");
    if (!SQL_SUCCEEDED(SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0)))
        throw std::runtime_error("SQLSetEnvAttr ODBC3 failed");
}

OdbcEnv::~OdbcEnv() {
    if (env != SQL_NULL_HENV) SQLFreeHandle(SQL_HANDLE_ENV, env);
}

void OdbcEnv::test() {
    SQLHDBC dbc;
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc)))
        throw std::runtime_error("failed");
    else {
		std::cout << "connected" << std::endl;
    }
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
}

OdbcConn::OdbcConn(SQLHENV e) : dbc(SQL_NULL_HDBC) {
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_DBC, e, &dbc)))
        throw std::runtime_error("SQLAllocHandle DBC failed");
}

void OdbcConn::connect(const std::string& connStr) {
    SQLCHAR out[1024]{};
    SQLSMALLINT outlen = 0;
    SQLRETURN rc = SQLDriverConnectA(
        dbc, NULL,
        (SQLCHAR*)connStr.c_str(), SQL_NTS,
        out, sizeof(out), &outlen,
        SQL_DRIVER_NOPROMPT
    );
    if (!SQL_SUCCEEDED(rc))
        throw std::runtime_error("SQLDriverConnect failed: " + odbc_diag(SQL_HANDLE_DBC, dbc));
    SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);

}



OdbcConn::~OdbcConn() {
    if (dbc != SQL_NULL_HDBC) {
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    }
}

OdbcStmt::OdbcStmt(SQLHDBC dbc) : stmt(SQL_NULL_HSTMT) {
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt)))
        throw std::runtime_error("SQLAllocHandle STMT failed");
}

OdbcStmt::~OdbcStmt() {
    if (stmt != SQL_NULL_HSTMT) SQLFreeHandle(SQL_HANDLE_STMT, stmt);
}
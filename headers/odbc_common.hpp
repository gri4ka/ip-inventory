#pragma once
#include <string>
#include <windows.h>
#include <sqlext.h>

std::string odbc_diag(SQLSMALLINT handleType, SQLHANDLE handle);

// wrapper object for ODBC environment handle.
struct OdbcEnv {
    SQLHENV env;
    OdbcEnv();
    ~OdbcEnv();
    void test();
private:
    OdbcEnv(const OdbcEnv&);
    OdbcEnv& operator=(const OdbcEnv&);
};
// wrapper for ODBC connection handle.
struct OdbcConn {
    SQLHDBC dbc;
    explicit OdbcConn(SQLHENV env);
    void connect(const std::string& connStr);
    void set_autocommit(bool on);
    ~OdbcConn();

private:
    OdbcConn(const OdbcConn&);
    OdbcConn& operator=(const OdbcConn&);
};

// wrapper for ODBC statement handle.
struct OdbcStmt {
    SQLHSTMT stmt;
    explicit OdbcStmt(SQLHDBC dbc);
    ~OdbcStmt();

private:
    OdbcStmt(const OdbcStmt&);
    OdbcStmt& operator=(const OdbcStmt&);
};
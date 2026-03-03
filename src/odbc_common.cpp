#include "../headers/odbc_common.hpp"
#include <sstream>
#include <stdexcept>
#include <iostream>


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
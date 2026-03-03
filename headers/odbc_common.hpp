#pragma once
#include <string>


#include <windows.h>
#include <sqlext.h>


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
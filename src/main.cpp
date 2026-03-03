#include <iostream>
#include "../headers/odbc_common.hpp"
int main() {
	std::cout << "Hello, World!" << std::endl;
	OdbcEnv env;
	env.test();
	return 0;
}
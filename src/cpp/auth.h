#ifndef AUTH_H
#define AUTH_H

#include <string>

void set_admin_username(std::string user);
void set_admin_password(std::string pass);
bool authenticate(std::string user, std::string pass);

#endif

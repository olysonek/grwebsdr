#include "auth.h"

using namespace std;

string username;
string password;

void set_admin_username(std::string user)
{
	username = user;
}

void set_admin_password(std::string pass)
{
	password = pass;
}

bool authenticate(string user, string pass)
{
	return user == username && pass == password;
}

/*
 * GrWebSDR: a web SDR receiver
 *
 * Copyright (C) 2017 Ondřej Lysoněk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING).  If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "auth.h"
#include <sqlite3.h>
#include <stdio.h>

using namespace std;

string username;
string password;
sqlite3 *db;
sqlite3_stmt *stmt;

void set_admin_username(std::string user)
{
	username = user;
}

void set_admin_password(std::string pass)
{
	password = pass;
}

void auth_finalize()
{
	sqlite3_finalize(stmt);
	stmt = nullptr;
	sqlite3_close(db);
	db = nullptr;
}

bool set_user_db(const char *path)
{
	if (sqlite3_open_v2(path, &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
		fprintf(stderr, "Failed to open user database.\n");
		auth_finalize();
		return false;
	}
	if (sqlite3_prepare_v2(db,
			"SELECT COUNT(*) FROM users WHERE user=?1 AND pass=?2",
			-1, &stmt, nullptr) != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare SQL statement.\n");
		auth_finalize();
		return false;
	}

	return true;
}

bool authenticate(string user, string pass)
{
	if (db == nullptr) {
		return user == username && pass == password;
	} else {
		bool ret = false;
		if (sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC)
				!= SQLITE_OK) {
			fprintf(stderr, "Failed to bind user to SQL statement\n");
			goto reset;
		}
		if (sqlite3_bind_text(stmt, 2, pass.c_str(), -1, SQLITE_STATIC)
				!= SQLITE_OK) {
			fprintf(stderr, "Failed to bind password to SQL statement\n");
			goto reset;
		}
		if (sqlite3_step(stmt) != SQLITE_ROW) {
			fprintf(stderr, "Failed to execute SQL statement\n");
			goto reset;
		}

		if (sqlite3_column_int(stmt, 0) == 1)
			ret = true;
reset:
		sqlite3_reset(stmt);
		return ret;
	}
}

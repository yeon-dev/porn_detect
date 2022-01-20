#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <numeric>

#include <sqlite3.h>

using namespace std;

class DBManager
{
private:
	DBManager();
	DBManager(const char* filename);
	DBManager(string& filename);
public:
	~DBManager();
public:
	static DBManager& instance();
	static DBManager& instance(string& filename);
	bool check_table_exists(const char* table_nm);
	int create_table_with_name(const char* table_nm, vector<pair<string, string>>& columns);
	int insert(string& tablenm, vector<string>& column_nm, vector<string>& values);

	template<typename T> T select(string& tablenm, string& target, vector<string>& whereClause);
private:
	sqlite3* db;
	string dbname;
	static DBManager* instance_;
};

template<typename T>
T DBManager::select(string& tablenm, string& target, vector<string>& whereClause) {
	if (db == nullptr) {
		fprintf(stderr, "Can't load database instance\n");
		return reinterpret_cast<T>(nullptr);
	}
	string base = "SELECT " + target + " FROM " + tablenm;
	if (whereClause.size() >= 1) {
		base += " WHERE ";
		for_each(whereClause.begin(), whereClause.end(),
			[&base, &whereClause](const string& clause) {
				if (clause == *whereClause.begin()) {
					// Start index
					base += clause;
				}
				else {
					base += " AND ";
					base += clause;
				}
			}
		);
	}
	string query = base + ";";
	sqlite3_stmt *stmt;
	int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		fprintf(stderr, "Error: while compiling sql: %s\n", sqlite3_errmsg(db));
		return reinterpret_cast<T>(nullptr);
	}

	rc = sqlite3_step(stmt);
	if (rc == SQLITE_ROW) {
		return (T)sqlite3_column_text(stmt, 0);
	}
	return reinterpret_cast<T>(nullptr);
}

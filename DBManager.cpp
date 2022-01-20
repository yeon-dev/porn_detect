#include "DBManager.h"

DBManager* DBManager::instance_ = nullptr;

DBManager::DBManager() : dbname(string("test.db")), db(nullptr) {
	if (dbname.length() > 0) {
		int rc = sqlite3_open(dbname.c_str(), &db);
		if (rc) {
			fprintf(stderr, "Can't open database: %s\n", dbname.c_str());
			return;
		}
	}
}
DBManager::~DBManager() {
	if (db != nullptr) {
		sqlite3_close(db);
	}
}

DBManager::DBManager(string& filename) : db(nullptr) {
	int rc = 0;
	if (filename.length() <= 0) {
		rc = sqlite3_open("test.db", &db);
	}
	else {
		rc = sqlite3_open(filename.c_str(), &db);
	}
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", filename.c_str());
		return;
	}
	dbname = filename;
}

DBManager::DBManager(const char* filename) : db(nullptr) {
	if (filename == nullptr) {
		filename = "test.db";
	}
	int rc = sqlite3_open(filename, &db);
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", filename);
		return;
	}
	dbname = filename;
}

DBManager& DBManager::instance() {
	if (instance_ == NULL) {
		instance_ = new DBManager();
	}
	return *instance_;
}

DBManager& DBManager::instance(string& filename) {
	if (instance_ == NULL) {
		instance_ = new DBManager(filename);
	}
	return *instance_;
}

bool DBManager::check_table_exists(const char* table_nm) {
	string check_sql = 
		"SELECT name FROM sqlite_master WHERE type='table' AND name='" + string(table_nm) + "';";
	
	// string check_sql = "SELECT * FROM COMPANY;";
	sqlite3_stmt* res;

	if (db == nullptr) {
		fprintf(stderr, "Can't load database instance\n");
		return false;
	}

	int rc = sqlite3_prepare_v2(db, check_sql.c_str(), -1, &res, 0);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
		return false;
	}
	rc = sqlite3_step(res);
	printf("%d\n", rc);
	if (rc == SQLITE_ROW) {
		printf("%s\n", sqlite3_column_text(res, 0));
		sqlite3_finalize(res);
		return true;
	}
	sqlite3_finalize(res);
	return false;
}

int DBManager::create_table_with_name(const char* table_nm, vector<pair<string, string>>& columns) {
	char* zErrMsg = 0;
	vector<string> columns_;
	string base = "CREATE TABLE " + string(table_nm) + "(";
	string delimiter = ",";

	if (db == nullptr) {
		fprintf(stderr, "Can't load database instance\n");
		return 0;
	}

	for (pair<string, string>& column : columns) {
		string& column_nm = column.first;
		string& column_type = column.second;

		columns_.push_back(column_nm + " " + column_type);
	}
	string joined;
	for_each(columns_.begin(), columns_.end(), [&joined, &delimiter](const string& elem) {
		if (joined.empty())
			joined += elem;
		else
			joined += delimiter + elem;
	});
	string query = base + joined + ");";

	// create table logic
	sqlite3_stmt* res;
	int rc = sqlite3_exec(db, query.c_str(), nullptr, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Create table failed: %s\n", zErrMsg);
		return 0;
	}
	fprintf(stdout, "Create table successfully\n");
	return 1;
}

int DBManager::insert(string& tablenm, vector<string>& column_nm, vector<string>& values) {
	string nm_part, value_part;
	string delimiter = ",";
	char* zErrMsg = 0;

	if (db == nullptr) {
		fprintf(stderr, "Can't load databsae instance\n");
		return 0;
	}
	for (int idx = 0; idx < column_nm.size(); idx++) {
		if (nm_part.empty() && value_part.empty()) {
			nm_part += column_nm[idx];
			value_part += values[idx];
		}
		else {
			nm_part += delimiter + column_nm[idx];
			value_part += delimiter + values[idx];
		}
	}
	sqlite3_stmt* res;
	string query = "INSERT INTO " + tablenm + "(" + nm_part + ") VALUES (" + value_part + ");";

	int rc = sqlite3_exec(db, query.c_str(), nullptr, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Insert rows failed: %s\n", zErrMsg);
		return 0;
	}
	fprintf(stdout, "Insert rows successfully\n");
	return 1;
}

int DBManager::update(string& tablenm, vector<string>& column_nm, vector<string>& values, vector<string>& whereClause) {
	char* zErrMsg = 0;
	string delimiter = ",";

	if (db == nullptr) {
		fprintf(stderr, "Can't load database instance\n");
		return 0;
	}

	string set_part;
	for (int idx = 0; idx < column_nm.size(); idx++) {
		if (set_part.empty()) {
			set_part += column_nm[idx] + "=" + values[idx];
		}
		else {
			set_part += ", ";
			set_part += column_nm[idx] + "=" + values[idx];
		}
	}
	string where_part;
	if (whereClause.size() >= 1) {
		where_part += " WHERE ";
		for_each(whereClause.begin(), whereClause.end(),
			[&where_part, &whereClause](const string& clause) {
				if (clause == *whereClause.begin())
					where_part += clause;
				else
					where_part += " AND " + clause;
			}
		);
	}
	string query = "UPDATE " + tablenm + " SET " + set_part + where_part + ";";
	cout << query << endl;

	sqlite3_stmt* res;
	int rc = sqlite3_exec(db, query.c_str(), nullptr, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Update rows failed: %s\n", zErrMsg);
		return 0;
	}
	fprintf(stdout, "Update rows successfully\n");
	return 1;
}
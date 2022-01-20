#include "HTTPFilter.h"
#include "DBManager.h"

void db_test() {
	string filename = "test.db";
	vector<pair<string, string>> columns = {
		{"test", "VARCHAR(256)"}, {"test2", "INT NOT NULL"}
	};
	DBManager& instance = DBManager::instance(filename);

	if (!instance.check_table_exists("COMPANY")) {
		instance.create_table_with_name("COMPANY", columns);
	}

	string table_nm = "COMPANY";
	vector<string> column_nm = { "test", "test2" };
	vector<string> values = { "'test'", "100"};
	instance.insert(table_nm, column_nm, values);

	vector<string> whereClause = { "test='test'", "test2=100" };
	string target = "test2";
	cout << instance.select<char*>(table_nm, target, whereClause) << endl;
}

int main() {
	// argument parser code here
	// 
	// 
	// end

	// result = capture_traffic(time);
	// write_to_file(result);
	HTTPFilter* filter = new HTTPFilter();
	filter->capture_traffic(1000, "Ethernet");

	delete filter;
	// db_test();
	return 0;
}
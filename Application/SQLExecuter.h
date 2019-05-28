#pragma once

#include <winsqlite/winsqlite3.h>
#include <string>
#include <filesystem>
#include "Common.h"
#include "sharedContainers.h"

using namespace System::Collections;

namespace fs = std::filesystem;

/* TODO: make singleton*/
class sqlExecuter {
private:
	sqlite3 * db;
	

	Generic::List< Generic::List<String ^>^>^ executeSqlStatement(const std::wstring& sqlStatement) {
		sqlite3_stmt *stmt = nullptr;
		Generic::List< Generic::List<String ^>^>^ ret = nullptr;
		int sqlResultCode = SQLITE_OK;
		
		
		sqlResultCode = sqlite3_prepare16_v2(db, sqlStatement.c_str(), sqlStatement.size(), &stmt, NULL); //preparing the statement
		if (sqlResultCode != SQLITE_OK) { //prepare failed
			DBOUT("Failed to prepare sql statement: " << sqlStatement << std::endl << sqlite3_errstr(sqlResultCode) << std::endl);
			throw sqlException();
		}

		sqlResultCode = sqlite3_step(stmt);
		if (sqlResultCode == SQLITE_DONE) { // nothing returned from executing, succeded in running
			
		}
		else if (sqlResultCode == SQLITE_ROW) { // rows are ready to read (return values)
			while (sqlite3_column_text(stmt, 0))
			{
				for (int i = 0; i < 4; i++)
					result[i].push_back(std::string((char *)sqlite3_column_text(stmt2, i)));
				sqlite3_step(stmt2);
				counter++;
			}
		}
		else {
			DBOUT("Failed to run sql statement: " << sqlStatement << std::endl << sqlite3_errstr(sqlResultCode) << std::endl);
		}

		sqlite3_finalize(stmt);
		return ret;
	}

public:

	// FIXME: add checks to path, directory must exist
	void createDatabaseFile(const std::wstring& path) {
		int sqlite_error = SQLITE_OK;
		
		if ((sqlite_error = sqlite3_open16(path.c_str(), &db))) { // if erro occured
			DBOUT("Failed to open database file: " << sqlite3_errmsg(db) << std::endl);
			throw sqlite3_errmsg(db);
		}
	}
	// FIXME: add checks to db open
	void closeDatabase() {
		int sqlite_error = SQLITE_OK;
		if ((sqlite_error = sqlite3_close(db))) { // if erro occured
			DBOUT("Failed to close database file: " << sqlite3_errmsg(db) << std::endl);
			throw sqlite3_errmsg(db);
		}
	}
	
	Generic::List< Generic::List<String ^>^>^ getTableRows(const std::wstring& tableName) {
		std::wstring sqlStatement = std::wstring() + L"SELECT * from " + tableName + L";";
		return executeSqlStatement(sqlStatement);
	}

	// FIXME: add checks to db open
	/*
	bool createTable(const std::string& tableName, ) {

	}

	bool selectTable(const std::string& tableName, ) {

	}

	bool deleteTable(const std::string& tableName, ) {

	}

	bool insertRow(const std::string& tableName, ) {

	}

	bool insertRow(const std::string& tableName, ) {

	}
	*/

	sqlExecuter() : db(nullptr) {}
	~sqlExecuter() { if (db != nullptr) closeDatabase(); db = nullptr; }

};
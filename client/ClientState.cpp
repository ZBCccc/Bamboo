#include "ClientState.h"
#include <iostream>

using namespace std;

ClientState::ClientState()
{
    db_path = "./bamboo_clnt_state.db";
    sqlite3_open(db_path.c_str(), &db);

    sqlite3_exec(db, "PRAGMA synchronous = OFF;", 0, 0, 0);

    sqlite3_stmt *stmt_create_table;

    sqlite3_prepare_v2(db,
                       "CREATE TABLE IF NOT EXISTS State ("
                       "keyword TEXT PRIMARY KEY,"
                       "cnt INTEGER NOT NULL,"
                       "tk BLOB NOT NULL);",
                       -1,
                       &stmt_create_table,
                       nullptr);
    sqlite3_step(stmt_create_table);
    sqlite3_finalize(stmt_create_table);
}

ClientState::~ClientState()
{
    sqlite3_close(db);
    db = nullptr;
}

bool ClientState::Get(StateCell &out, const string &keyword)
{
    sqlite3_stmt *stmt_read_data;

    sqlite3_prepare_v2(db,
                       "SELECT cnt, tk FROM State WHERE keyword=?1;",
                       -1,
                       &stmt_read_data,
                       nullptr);
    sqlite3_bind_text(stmt_read_data, 1, keyword.c_str(), -1, nullptr);

    if (sqlite3_step(stmt_read_data) != SQLITE_ROW)
    {
        sqlite3_finalize(stmt_read_data);
        return false;
    }

    out.cntw = sqlite3_column_int(stmt_read_data, 0);
    out.tk.assign((const char *)sqlite3_column_blob(stmt_read_data, 1),
                  sqlite3_column_bytes(stmt_read_data, 1));

    sqlite3_finalize(stmt_read_data);
    return true;
}

void ClientState::Put(const StateCell &in, const string &keyword)
{
    sqlite3_stmt *stmt_update_data;

    sqlite3_prepare_v2(db,
                       "REPLACE INTO State (keyword, cnt, tk) VALUES (?1, ?2, ?3);",
                       -1,
                       &stmt_update_data,
                       nullptr);
    sqlite3_bind_text(stmt_update_data, 1, keyword.c_str(), -1, nullptr);
    sqlite3_bind_int(stmt_update_data, 2, in.cntw);
    sqlite3_bind_blob(stmt_update_data, 3, in.tk.c_str(), in.tk.size(), nullptr);
    sqlite3_step(stmt_update_data);
    sqlite3_finalize(stmt_update_data);
}

void ClientState::Clear()
{
    sqlite3_stmt *stmt_clear_data;

    sqlite3_prepare_v2(db,
                       "DELETE FROM State;",
                       -1,
                       &stmt_clear_data,
                       nullptr);
    sqlite3_step(stmt_clear_data);
    sqlite3_finalize(stmt_clear_data);
}

void ClientState::DumpData(const string &dname)
{
    sqlite3 *db_back;
    sqlite3_backup *sqlb;

    sqlite3_open(dname.c_str(), &db_back);
    sqlb = sqlite3_backup_init(db_back, "main", db, "main");
    sqlite3_errmsg(db_back);
    sqlite3_backup_step(sqlb, -1);
    sqlite3_backup_finish(sqlb);

    sqlite3_close(db_back);
}

void ClientState::LoadData(const string &dname)
{
    sqlite3 *db_back;
    sqlite3_backup *sqlb;

    sqlite3_open(dname.c_str(), &db_back);
    sqlb = sqlite3_backup_init(db, "main", db_back, "main");
    sqlite3_backup_step(sqlb, -1);
    sqlite3_backup_finish(sqlb);

    sqlite3_close(db_back);
}

void ClientState::GetKeywordsCnt(vector<int> &cnt)
{
    sqlite3_stmt *stmt_get_cnt;

    sqlite3_prepare_v2(db,
                       "SELECT cnt FROM State;",
                       -1,
                       &stmt_get_cnt,
                       nullptr);
    while (sqlite3_step(stmt_get_cnt) == SQLITE_ROW)
    {
        cnt.emplace_back(sqlite3_column_int(stmt_get_cnt, 0));
    }
}

#include <stdio.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <assert.h>
#include "el.h"
#include "db_pgclient.h"

using namespace std;
using namespace evt_loop;
using namespace db_api;

static void PrintResultset(DBResult* result) {
  for (int i = 0; i < result->RowCount(); ++i) {
    for (int j = 0; j < result->ColCount(); ++j) {
      cout << result->GetValue(i, j) << " ";
    }
    cout << endl;
  }
}

class PGClient_Test {
  public:
  PGClient_Test()
  {
    bool success = false;
#if 0
    map<string, string> conn_info;
    conn_info["user"] = "postgres";
    conn_info["dbname"] = "postgres";
    conn_info["password"] = "postgres";
    success = pg_client_.Connect(conn_info);
#else
    success = pg_client_.Connect("postgresql://postgres:postgres@localhost/postgres");
#endif
    if (!success) {
      cout << "connect failed: " << pg_client_.GetLastError().ToString() << endl;
    }
    assert(success);

    vector<SQLParameter> params;
    SQLParameter p1("100");
    SQLParameter p2("hello");
    params.push_back(p1);
    params.push_back(p2);

    using namespace std::placeholders;
    DBResultCallback cb = std::bind(&PGClient_Test::on_insert_result, this, _1, _2, _3);
    success = pg_client_.ExecuteSQLAsync("insert into test values ($1, $2)", params, cb, (void*)"test insert");
    assert(success);
    params.clear();

    DBResultCallback cb2 = std::bind(&PGClient_Test::on_select_result, this, _1, _2, _3);
    success = pg_client_.ExecuteSQLAsync("select * from test", cb2, (void*)"test select");
    assert(success);

    params.push_back(p1);
    DBResultCallback cb3 = std::bind(&PGClient_Test::on_delete_result, this, _1, _2, _3);
    success = pg_client_.ExecuteSQLAsync("delete from test where id=$1", params, cb3, (void*)"test delete");
    assert(success);
    params.clear();
  }

  protected:
  void on_insert_result(const DBError& error, DBResult* result, void* ctx) {
    bool success = !error.GetError();
    if (success) {
      printf("[%s] insert %d rows\n", (char*)ctx, result->AffectedRows());
      assert(result->AffectedRows() == 1);
    } else {
      cout << "[on_insert_result] error: " << error.ToString() << endl;
    }
    assert(success);
  }

  void on_select_result(const DBError& error, DBResult* result, void* ctx) {
    bool success = !error.GetError();
    if (success) {
      cout << (char*)ctx << endl;
      assert(result->ColCount() == 2);
      printf("[%s] select %d rows\n", (char*)ctx, result->AffectedRows());
      PrintResultset(result);
    } else {
      cout << "[on_select_result] error: " << error.ToString() << endl;
    }
    assert(success);
  }

  void on_delete_result(const DBError& error, DBResult* result, void* ctx) {
    bool success = !error.GetError();
    if (success) {
      printf("[%s] delete %d rows\n", (char*)ctx, result->AffectedRows());
      //assert(result->ColCount() == 2);
    } else {
      cout << "[on_delete_result] error: " << error.ToString() << endl;
    }
    assert(success);

    EV_Singleton->StopLoop();
  }


  private:
  PGClient pg_client_;
};

int main(int argc, char **argv) {
  PGClient_Test pg_client_test;

  EV_Singleton->StartLoop();

  return 0;
}


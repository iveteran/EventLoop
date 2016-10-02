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

    uint8_t array[16] = {0, 1, 2, 0xff, 0xa};

    SQLParameter p1(2147483647);
    SQLParameter p2("hello");
    SQLParameter p3((char*)array, sizeof(array), SQLParameter::BINARY);

    using namespace std::placeholders;
    DBResultCallback cb = std::bind(&PGClient_Test::on_insert_result, this, _1, _2, _3);
    success = pg_client_.ExecuteSQLAsync("insert into test values ($1, $2, $3)", cb, (void*)"test insert", 3, &p1, &p2, &p3);
    assert(success);

    DBResultCallback cb2 = std::bind(&PGClient_Test::on_select_result, this, _1, _2, _3);
    success = pg_client_.ExecuteSQLAsync("select * from test", cb2, (void*)"test select");
    assert(success);

    DBResultCallback cb3 = std::bind(&PGClient_Test::on_delete_result, this, _1, _2, _3);
    success = pg_client_.ExecuteSQLAsync("delete from test where id=$1", cb3, (void*)"test delete", 1, &p1);
    assert(success);
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
      assert(result->ColCount() == 3);
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


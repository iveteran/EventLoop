#ifndef _DB_PGCLIENT_H
#define _DB_PGCLIENT_H

#include <queue>
#include <libpq-fe.h>
#include "el.h"
#include "db_client.h"

using namespace evt_loop;

namespace db_api {

class DBConnection;
class PGClient;;

class PGResult: public DBResult
{
  friend class PGClient;
  public:
    ~PGResult();
    int AffectedRows() const;
    int RowCount() const;
    int ColCount() const;
    bool IsNull(int row, int column) const;
    const char* GetValue(int row, int column) const;
    string GetBytesValue(int row, int col) const;
    int GetIntValue(int row, int column) const;
    bool GetBoolValue(int row, int column) const;
    double GetDoubleValue(int row, int column) const;
    int GetColumnIndex(const char* column_name) const;
    const char* GetFieldname(int column) const;
    PGResult* Clone() const;
    DBConnection* GetDBConnection() const;    

  private:
    PGResult(PGClient* connection, PGresult *result);
    PGResult& operator=(const PGResult& rhs);
    PGResult(const PGResult& rhs);

  private:
    PGClient*   m_connection;
    PGresult*   m_result;
};

class PGClient: public DBConnection, public IOEvent
{
  public:
    PGClient(bool auto_connect = true);
    ~PGClient();

    bool Connect(const char* conn_uri);
    bool Connect(const str2strmap& connInfo);
    void Disconnect();
    bool RollbackTransaction();

    bool IsConnected();
    int  GetWaitingSQLCount();
    bool BeginTransaction(const DBResultCallback& cb, void* ctx);
    bool CommitTransaction(const DBResultCallback& cb, void* ctx);

    PGResult* ExecuteSQL(const char* sql, bool& success);
    PGResult* ExecuteSQL(const char* sql, const vector<SQLParameter>& params, bool& success);

    bool ExecuteSQLAsync(const char* sql, const DBResultCallback& cb, void* ctx);
    bool ExecuteSQLAsync(const char* sql, const vector<SQLParameter>& params, const DBResultCallback& cb, void* ctx);

    bool AddSubscribeChannel(const char* channelName, const SubscribeCallback& cb);
    bool RemoveSubscribeChannel(const char* channelName);

    const DBError& GetLastError() { return m_dberror; }

    void CancelQueue(bool invokeCallback);
    bool IsInTransactionBlock();
    bool CancelCurrentQuery();

  private:
    bool Connect_();
    void Reconnect();

    bool SendNextQueryAsync();
    PGResult* PollResultSet(bool& success);
    bool HandleSubscription();

    void SetLastError(const char* severity, const char* sqlstate,
        const char* message = NULL, const char* detail = NULL, const char* hint = NULL);

    void ReadBytes();
    void WriteBytes();
    void OnEvents(uint32_t events);  // implements IOEvent::OnEvents(uint32_t)

    void OnReconnectTimer(PeriodicTimer* timer);

  private:
    PGClient& operator=(const PGClient& rhs);
    PGClient(const PGClient& rhs);

  private:
    enum SQLCommand {
      CONNECT,
      BEGINTRANSACTION,
      COMMITTRANSACTION,
      ROLLBACKTRANSACTION,
      QUERY,
      SUBSCRIBE,
      UNSUBSCRIBE
    };

    struct QueryItem {
      QueryItem(const char* sqlStmt, const vector<SQLParameter>& params, const DBResultCallback& c, void* cx) :
        sqlQuery(sqlStmt), parameters(params), cb(c), ctx(cx)
      { }
      string ToString() const;

      string sqlQuery;
      vector<SQLParameter> parameters;
      DBResultCallback cb;
      void* ctx;
    };

    std::queue<QueryItem>  m_queryQueue;
    std::map<string, SubscribeCallback> m_subscribeMap;

  private:
    SQLCommand  m_last_cmd;
    string      m_conn_str;
    PGconn*     m_pgconn;
    bool        m_transactionStarted;
    DBError     m_dberror;
    bool        m_auto_reconnect;
    PeriodicTimer m_reconnect_timer;
};

} // namespace db_api

#endif  // _DB_PGCLIENT_H

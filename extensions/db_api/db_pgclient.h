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

    void Cleanup();
    void SetCallbacks(const DBCallbacksPtr& db_cbs);

    bool Init(const char* conn_uri, const DBCallbacksPtr& db_cbs, bool with_connect = true);
    bool Init(const str2strmap& conn_dict, const DBCallbacksPtr& db_cbs, bool with_connect = true);
    bool Connect();
    void Disconnect();
    bool RollbackTransaction();

    bool IsConnected();
    int  GetWaitingSQLCount();
    bool BeginTransaction(const DBResultCallback& cb, void* ctx);
    bool CommitTransaction(const DBResultCallback& cb, void* ctx);

    DBResult* ExecuteSQL(const char* sql, int pcount = 0, ...);
    bool ExecuteSQLAsync(const char* sql, const DBResultCallback& cb, void* ctx, int pcount = 0, ...);

    bool AddSubscribeChannel(const char* channelName, const SubscribeCallback& cb);
    bool RemoveSubscribeChannel(const char* channelName);

    const DBError& GetLastError() { return m_dberror; }

    void CancelQueue(bool invokeCallback);
    bool IsInTransactionBlock();
    bool CancelCurrentQuery();

    const string& GetConnectionStr() const { return m_conn_str; }

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
    void RemoveFDHandler();

    void OnReconnectTimer(TimerEvent* timer);
    void OnConnected();
    void OnClosed();
    void OnError(int errcode, const char* errstr);

  private:
    PGClient& operator=(const PGClient& rhs);
    PGClient(const PGClient& rhs);

  private:
    enum SQLCommand {
      UNKNOWN,
      CONNECT,
      BEGINTRANSACTION,
      COMMITTRANSACTION,
      ROLLBACKTRANSACTION,
      QUERY,
      SUBSCRIBE,
      UNSUBSCRIBE
    };

    struct QueryItem {
      QueryItem(const char* sql, const DBResultCallback& c, void* cx, const vector<SQLParameter>* p = NULL) :
        sqlQuery(sql), cb(c), ctx(cx), committed(false)
      {
        if (p) params = *p;
      }
      string ToString() const;

      string sqlQuery;
      vector<SQLParameter> params;
      DBResultCallback cb;
      void* ctx;
      bool committed;
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

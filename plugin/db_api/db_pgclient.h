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
  friend class    PGClient;
  public:
    ~PGResult();
    //bool IsOk() const;
    int AffectedRows() const;
    int RowCount() const;
    int ColCount() const;
    bool IsNull(int row, int column) const;
    const char* GetValue(int row, int column) const;
    const unsigned char* GetUnescapeValue(int row, int column, size_t* length) const;
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
    PGClient();
    ~PGClient();

    // The following 3 APIs are always blocking
    bool Connect(const map<string,string>& connInfo);
    void Disconnect();
    bool RollbackTransaction();

    bool IsConnected();
    int  GetWaitingSQLCount();
    bool BeginTransaction(const DBResultCallback& cb, void* ctx);
    bool CommitTransaction(const DBResultCallback& cb, void* ctx);
    //bool RollbackTransaction(const DBResultCallback& cb, void* ctx);

    PGResult* RunBlockingSQL(const char* sql, bool& success);
    PGResult* RunBlockingSQL(const char* sql, const vector<SQLParameter>& params, bool& success);

    bool AddNonBlockingSQL(const char* sql, const DBResultCallback& cb, void* ctx);
    bool AddNonBlockingSQL(const char* sql, const vector<SQLParameter>& params, const DBResultCallback& cb, void* ctx);

    bool AddSubscribeChannel(const char* channelName, const SubscribeCallback& cb);
    bool RemoveSubscribeChannel(const char* channelName);

    const DBError& GetLastError() { return m_dberror; }

    void CancelQueue(bool invokeCallbackFunction);
    bool IsInTransactionBlock();
    bool CancelCurrentQuery();

  private:
    void OnEvents(uint32_t events);  // implements IOEvent::OnEvents(uint32_t)
    void read();
    void write();

    void DummyRollbackCb(const DBError& err, DBResult* result, void* ctx);
    bool SendNextQueryRequestInNonBlocking();
    void SetLastError(const char* severity, const char* sqlstate,
        const char* message = NULL, const char* detail = NULL, const char* hint = NULL);
    PGResult* PollResultSet(bool& success);
    bool HandleSubscription();
    const char* ShowParamValues(int count, char* paramValues[]);

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
      string sqlQuery;
      vector<SQLParameter> parameters;
      DBResultCallback cb;
      void* ctx;
    };

    std::queue<QueryItem>  m_queryQueue;
    std::map<string, SubscribeCallback> m_subscribeMap;

  private:
    SQLCommand  m_last_cmd;
    PGconn*     m_pgconn;
    bool        m_transactionStarted;
    DBError     m_dberror;
};

} // namespace db_api

#endif  // _DB_PGCLIENT_H

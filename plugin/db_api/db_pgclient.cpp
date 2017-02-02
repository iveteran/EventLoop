#include <sstream>
#include <stdarg.h>
#include "db_pgclient.h"

namespace db_api {

// Error Messages used inside PGClient class
const char* INVALID_CONNECTION_STRING = "Invalid database connection string";
const char* NOCONNECTION = "No database connection is made yet!";
const char* CONNECTION_LOST = "Database connection is lost";
const char* ERROREXEC = "Error occurred in executing";
const char* CONNECTION_FAILED = "Connection to the database failed";
const char* NO_NONBLOCKINGCALL_REQUESTED   = "There is no non-blocking sql query requested";
const char* ALREADY_NONBLOCKINGCALL_REQUESTED   = "There is already another non-blocking sql query requested";
const char* INVALID_TRANSACTION = "There is an already running transaction";
const char* CANCELLED_REQUEST = "The request is cancelled due to rollback";

PGClient::PGClient(bool auto_reconnect)
  : m_last_cmd(UNKNOWN), m_pgconn(NULL),
  m_transactionStarted(false),
  m_auto_reconnect(auto_reconnect),
  m_reconnect_timer(std::bind(&PGClient::OnReconnectTimer, this, std::placeholders::_1))
{
  if (m_auto_reconnect) {
    timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    m_reconnect_timer.SetInterval(tv);
  }
}

PGClient::~PGClient()
{
  Disconnect();
  Cleanup();
}

void PGClient::Cleanup()
{
  if (m_transactionStarted) {
    m_transactionStarted = false;
  }

  while (!m_queryQueue.empty()) {
    m_queryQueue.pop();
  }
  m_subscribeMap.clear();
  m_transactionStarted = false;
}

void PGClient::SetCallbacks(const DBCallbacksPtr& db_cbs)
{
  m_db_cbs = db_cbs;
}
bool PGClient::Init(const char* conn_uri, const DBCallbacksPtr& db_cbs, bool with_connect)
{
  SetCallbacks(db_cbs);
  if (conn_uri && strncmp(conn_uri, "postgresql://", 13) != 0 &&
      strncmp(conn_uri, "postgres://", 11) != 0) {
    SetLastError("ERROR", "00000", INVALID_CONNECTION_STRING);
    return false;
  }
  if (m_conn_str != conn_uri)
    m_conn_str = conn_uri;

  return with_connect ? Connect() : true;
}
bool PGClient::Init(const str2strmap& conn_dict, const DBCallbacksPtr& db_cbs, bool with_connect)
{
  str2strmap::const_iterator it;
  for (it = conn_dict.begin(); it != conn_dict.end(); it++) {
    m_conn_str.append(it->first);
    m_conn_str.append("=");
    m_conn_str.append(it->second);
    m_conn_str.append(" ");
  }
  return Init(m_conn_str.c_str(), db_cbs, with_connect);
}

bool PGClient::Connect()
{
  bool success = Connect_();
  if (success) {
    OnConnected();
  } else {
    Reconnect();
  }
  return success;
}

bool PGClient::Connect_()
{
  printf("PGClient::Connect_ starts\n");
  bool success = false;

  if (m_pgconn == NULL) {
    printf("PGClient::Connect_ connection string: %s\n", m_conn_str.c_str());
    m_pgconn = PQconnectdb(m_conn_str.c_str());
  } else {
    PQreset(m_pgconn);
  }

  if (PQstatus(m_pgconn) != CONNECTION_OK) {
    const char* err_msg = PQerrorMessage(m_pgconn);
    SetLastError("ERROR", "08006", err_msg);
    printf("PGClient::Connect_ Connect to database(%s) failed: %s\n", m_conn_str.c_str(), SAFE_STRING(err_msg));
  } else {
    success = true;
    SetFD(PQsocket(m_pgconn));  // add fd to event loop
  }
  printf("PGClient::Connect_ end, success: %d\n", success);
  return  success;
}

void PGClient::Reconnect() {
  if (!IsConnected() && !m_reconnect_timer.IsRunning())
    m_reconnect_timer.Start();
}

void PGClient::Disconnect() {
  if (m_pgconn != NULL) {
    PQfinish(m_pgconn);
    m_pgconn = NULL;
    printf("PGClient::Disconnect PQfinish done\n");
  }
  RemoveFDHandler();
}

bool PGClient::IsConnected() {
  return m_pgconn && PQstatus(m_pgconn) == CONNECTION_OK;
}

int PGClient::GetWaitingSQLCount() {
  return m_queryQueue.size();
}

void PGClient::SetLastError( const char* severity, const char* sqlstate,
    const char* message, const char* detail, const char* hint) {
  m_dberror.SetError(true);
  m_dberror.SetSeverity(severity);
  m_dberror.SetSQLState(sqlstate);
  m_dberror.SetMessage(message);
  m_dberror.SetDetail(detail);
  m_dberror.SetHint(hint);
  if ((message == NULL || message[0] == '\0') && strcmp(sqlstate, "08006") == 0) {
    // Postgresql doesn't provide error message for connection loss (state: 08006
    m_dberror.SetMessage(CONNECTION_LOST);
  }
}

bool PGClient::BeginTransaction(const DBResultCallback& cb, void* ctx) {
  printf("PGClient::beginTransaction start\n");
  bool success = false;
  if (m_pgconn != NULL) {
    if (!m_transactionStarted) {
      m_last_cmd = BEGINTRANSACTION;
      success = ExecuteSQLAsync("begin transaction", cb, ctx);
      if (success) {
        m_transactionStarted = success;
      } else {
        // TODO: handle broken connection
      }
    } else {
      SetLastError("ERROR", "0B000", INVALID_TRANSACTION);
      printf("PGClient::beginTransaction there was an already running transaction in this connection.\n");
    }
  } else {
    SetLastError("ERROR", "08006", NOCONNECTION);
    printf("PGClient::beginTransaction there was no connection made yet.\n");
  }
  printf("PGClient::beginTransaction end %d\n", success);
  return  success;
}

bool PGClient::CommitTransaction(const DBResultCallback& cb, void* ctx) {
  printf("PGClient::commitTransaction start\n");
  bool success = false;
  if (m_pgconn != NULL) {
    m_last_cmd = COMMITTRANSACTION;
    success = ExecuteSQLAsync("commit transaction", cb, ctx);
  } else {
    //m_last_error = NOCONNECTION;
    SetLastError("ERROR", "08006", NOCONNECTION);
    printf("PGClient::commitTransaction there was no connection made yet.\n");
  }
  printf("PGClient::commitTransaction end %d\n", success);
  return  success;
}

// This function will be invoked when it runs into an error in the middle of transaction
void PGClient::CancelQueue(bool invokeCallbackFunction) {
  if (invokeCallbackFunction) {
    while (!m_queryQueue.empty()) {
      DBResultCallback& cb = m_queryQueue.front().cb;
      void* callerContext = m_queryQueue.front().ctx;
      m_queryQueue.pop();
      cb(m_dberror, NULL, callerContext);
    }
  } else {
    while (!m_queryQueue.empty()) {
      m_queryQueue.pop();
    }
  }
}

bool PGClient::CancelCurrentQuery() {
  printf("PGClient::cancelCurrentQuery start\n");
  bool success = false;
  if (m_pgconn != NULL) {
    char errbuf[256];
    PGcancel* cancel = PQgetCancel(m_pgconn);
    int tmpResult = PQcancel(cancel, errbuf, sizeof(errbuf));
    if (tmpResult == 1) {
      success = true;
    }
    PQfreeCancel(cancel);
  }
  printf("PGClient::cancelCurrentQuery end %d\n", success);
  return success;
}

bool PGClient::RollbackTransaction() {
  bool success = false;
  if (m_pgconn != NULL) {
    printf("PGClient::rollbackTransaction start\n");
    if (GetWaitingSQLCount() > 0) {
      CancelCurrentQuery();
    }

    DBResult* result = ExecuteSQL("rollback transaction");
    if (result != NULL) {
      success = true;
      delete result;
    }

    if (GetWaitingSQLCount() > 0) {
      SetLastError("ERROR", "57014", CANCELLED_REQUEST);
      CancelQueue(true);
    }
    m_transactionStarted = false;
    printf("PGClient::rollbackTransaction end %d\n", success);
  } else {
    SetLastError("ERROR", "08006", NOCONNECTION);
    printf("PGClient::rollbackTransaction there was no connection made yet.\n");
  }
  return  success;
}

DBResult* PGClient::ExecuteSQL(const char* sql, int pcount, ...) {
  printf("PGClient::ExecuteSQL start %s\n", SAFE_STRING(sql));
  bool success = false;
  PGResult* result = NULL;
  m_dberror.Clear();

  if (m_pgconn != NULL) {
    const char**  paramsValue = (const char**)NULL;
    int     paramsCount = pcount;
    int*    paramLengths = NULL;
    int*    paramFormats = NULL;

    if (paramsCount > 0) {
      paramsValue = new const char*[paramsCount];
      paramLengths = new int[paramsCount];
      paramFormats = new int[paramsCount];
      va_list vl;
      va_start(vl, pcount);
      for (int i = 0; i < paramsCount; i++) {
        SQLParameter* param = va_arg(vl, SQLParameter*);
        paramsValue[i] = param->value_.data();
        paramLengths[i] = param->value_.size();
        paramFormats[i] = param->format_;
      }
      va_end(vl);
    }

    PGresult* res = PQexecParams(m_pgconn,
        sql,
        paramsCount,
        NULL,
        paramsValue,
        paramLengths,
        paramFormats,
        0);
    if (res) {
      ExecStatusType status = PQresultStatus(res);
      if (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK) {
        result = new PGResult(this, res);
        success = true;
      } else {
        SetLastError(PQresultErrorField(res, PG_DIAG_SEVERITY),
            PQresultErrorField(res, PG_DIAG_SQLSTATE),
            PQresultErrorField(res, PG_DIAG_MESSAGE_PRIMARY),
            PQresultErrorField(res, PG_DIAG_MESSAGE_DETAIL),
            PQresultErrorField(res, PG_DIAG_MESSAGE_HINT));
        printf("PGClient::ExecuteSQL [%s].\n", SAFE_STRING(PQresultErrorMessage(res)));
      }
    } else {
      const char* err_msg = PQerrorMessage(m_pgconn);
      SetLastError("ERROR", "00000", err_msg);
      printf("PGClient::ExecuteSQL [%s].\n", SAFE_STRING(err_msg));
    }
    if (paramFormats != NULL) delete[] paramFormats;
    if (paramLengths != NULL) delete[] paramLengths;
    if (paramsValue != NULL) delete[] paramsValue;
  } else {
    SetLastError("ERROR", "08006", NOCONNECTION);
    printf("PGClient::ExecuteSQL there was no connection made yet.\n");
  }

  printf("PGClient::ExecuteSQL end %d\n", success);
  return result;
}

bool PGClient::SendNextQueryAsync()
{
  printf("PGClient::SendNextQueryAsync start\n");
  bool success = false;

  if (!m_queryQueue.empty()) {
    QueryItem& qitem = m_queryQueue.front();
    if (qitem.committed) return true;

    const char* sql = qitem.sqlQuery.c_str();
    const char**  paramsValue = (const char**)NULL;
    int* paramLengths = NULL;
    int* paramFormats = NULL;
    int  paramsCount = qitem.params.size();

    if (paramsCount > 0) {
      paramsValue = new const char*[paramsCount];
      paramLengths = new int[paramsCount];
      paramFormats = new int[paramsCount];
      for (int i = 0; i < paramsCount; i++) {
        paramsValue[i] = qitem.params[i].value_.data();
        paramLengths[i] = qitem.params[i].value_.size();
        paramFormats[i] = qitem.params[i].format_;
      }
    }
    printf("[PGClient::SendNextQueryAsync] Execute: %s\n", qitem.ToString().c_str());

    int nResult = PQsendQueryParams(m_pgconn,
        sql,
        paramsCount,
        NULL,
        paramsValue,
        paramLengths,
        paramFormats,
        0);
    if (nResult == 1) {
      m_last_cmd = QUERY;
      int flushResult = PQflush(m_pgconn);
      if (flushResult == 0) {
        // There is no data left in send queue
        DeleteWriteEvent();
        success = true;
      } else if (flushResult == 1) {
        // There is some data left in send queue
        AddWriteEvent();
        success = true;
      } else if (flushResult == -1) {
        const char* err_msg = PQerrorMessage(m_pgconn);
        SetLastError("ERROR", "08006", err_msg);
        printf("PGClient::SendNextQueryAsync error1:%s\n", SAFE_STRING(err_msg));
      }
    } else {
      const char* err_msg = PQerrorMessage(m_pgconn);
      SetLastError("ERROR", "08006", err_msg);
      printf("PGClient::SendNextQueryAsync error2:%s\n", SAFE_STRING(err_msg));
    }
    if (paramFormats != NULL) delete[] paramFormats;
    if (paramLengths != NULL) delete[] paramLengths;
    if (paramsValue != NULL) delete[] paramsValue;

    qitem.committed = true;
  } else {
    // The queue is empty. Nothing to request
    success = true;
  }

  printf("PGClient::SendNextQueryAsync end %d\n", success);
  return  success;
}

bool PGClient::ExecuteSQLAsync(const char* sql, const DBResultCallback& cb, void* ctx, int pcount, ...) {
  bool success = true;
  m_dberror.Clear();
  QueryItem qry_item(sql, cb, ctx);
  va_list vl;
  va_start(vl, pcount);
  for (int i=0; i<pcount; i++) {
    SQLParameter* val = va_arg(vl, SQLParameter*);
    qry_item.params.push_back(*val);
  }
  va_end(vl);
  m_queryQueue.push(qry_item);
  if (!m_queryQueue.empty()) {
  //if (m_queryQueue.size() == 1) {
    success = SendNextQueryAsync();
    if (!success) {
      m_queryQueue.pop();
      cb(m_dberror, NULL, ctx);
    }
  }

  return  success;
}

bool PGClient::IsInTransactionBlock() {
  bool success = false;
  if (m_pgconn && m_transactionStarted) {
    PGTransactionStatusType tstatus = PQtransactionStatus(m_pgconn);
    if (tstatus == PQTRANS_INTRANS || tstatus == PQTRANS_INERROR) {
      success = true;
    }
  }
  return success;
}

bool PGClient::HandleSubscription() {
  if (PQconsumeInput(m_pgconn) == 1) {
    PGnotify* pg_notify = NULL;
    while ((pg_notify = PQnotifies(m_pgconn)) != NULL) {
      string channelName = pg_notify->relname;
      string message = pg_notify->extra;
      printf("[PGClient::HandleSubscription] Invokes registered subscription callback function with [%s]\n", message.c_str());
      auto iter = m_subscribeMap.find(channelName);
      if (iter != m_subscribeMap.end()) {
        auto& cb = iter->second;
        DBError noerror;
        cb(noerror, channelName, message);
      }
      PQfreemem(pg_notify);
    }
    return true;
  } else {
    if (PQstatus(m_pgconn) != CONNECTION_OK) {
      const char* err_msg = PQerrorMessage(m_pgconn);
      SetLastError("ERROR", "08006", err_msg);

      for (auto iter = m_subscribeMap.begin(); iter != m_subscribeMap.end(); ++iter) {
        const string& chan_name = iter->first;
        auto& cb = iter->second;
        cb(m_dberror, chan_name, "");
      }
    }
    return false;
  }
}
PGResult* PGClient::PollResultSet(bool& success) {
  PGResult* rst = NULL;
  success = false;

  if (PQconsumeInput(m_pgconn) == 1 && PQisBusy(m_pgconn) == 0) {
    success = true;
    PGresult* result = PQgetResult(m_pgconn);
    if (result != NULL) {
      ExecStatusType status = PQresultStatus(result);
      if (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK) {
        // successful case
        rst = new PGResult(this, result);
      } else {
        SetLastError(PQresultErrorField(result, PG_DIAG_SEVERITY),
            PQresultErrorField(result, PG_DIAG_SQLSTATE),
            PQresultErrorField(result, PG_DIAG_MESSAGE_PRIMARY),
            PQresultErrorField(result, PG_DIAG_MESSAGE_DETAIL),
            PQresultErrorField(result, PG_DIAG_MESSAGE_HINT));
        PQclear(result);
        printf("PGClient::pollResultset received an error message: [%s]\n", SAFE_STRING(PQresultErrorMessage(result)));
      }
      while ((result = PQgetResult(m_pgconn)) != NULL) {
        PQclear(result);
      }
    } else {
      const char* err_msg = PQerrorMessage(m_pgconn);
      SetLastError("ERROR", "00000", err_msg);
      printf("PGClient::pollResultset postgresql returned empty resultset although it is supposed to return a resultset. [%s]\n",
          SAFE_STRING(err_msg));
    }
  } else {
    if (PQstatus(m_pgconn) != CONNECTION_OK) {
      const char* err_msg = PQerrorMessage(m_pgconn);
      SetLastError("ERROR", "08006", err_msg);
      printf("PGClient::pollResultset connection lost: [%s]\n", SAFE_STRING(err_msg));
    }
  }

  return  rst;
}

bool PGClient::AddSubscribeChannel(const char* channelName, const SubscribeCallback& cb) {
  printf("PGClient::AddSubscribeChannel start: channelName [%s]\n", SAFE_STRING(channelName));
  if (channelName == NULL) return false;
  bool success = false;
  stringstream ss;
  ss << "LISTEN " << channelName;
  string sql = ss.str();
  // Refer to addSubscribeChannel function for calling either blocking call or nonblocking call.
  DBResult* result = ExecuteSQL(sql.c_str());
  if (result != NULL) {
    m_last_cmd = SUBSCRIBE;
    delete result;

    m_subscribeMap[channelName] = cb;
    success = true;
  }
  printf("PGClient::addSubscribeChannel end: %d\n", success);
  return  success;
}

bool PGClient::RemoveSubscribeChannel(const char* channelName) {
  printf("PGClient::RemoveSubscribeChannel start: channelName [%s]\n", SAFE_STRING(channelName));
  if (channelName == NULL) return false;
  bool success = false;
  stringstream ss;
  ss << "UNLISTEN " << channelName;
  string sql = ss.str();
  DBResult* result = ExecuteSQL(sql.c_str());
  if (result != NULL) {
    m_last_cmd = UNSUBSCRIBE;
    delete result;

    m_subscribeMap.erase(channelName);
    success = true;
  }
  printf("PGClient::removeSubscribeChannel end: %d\n", success);
  return  success;
}

void PGClient::ReadBytes() {
  bool success = false;
  PGResult* rst = NULL;

  if (m_last_cmd == SQLCommand::UNKNOWN) {
    // receives EOF of tcp connection
    rst = PollResultSet(success);
    return;
  }

  switch (m_last_cmd) {
    case CONNECT:
      break;
    case SUBSCRIBE:
    case UNSUBSCRIBE:
      break;
    case BEGINTRANSACTION:
    case COMMITTRANSACTION:
    case ROLLBACKTRANSACTION:
    case QUERY:
      rst = PollResultSet(success);
      break;
    default:
      break;
  }

  if (!m_subscribeMap.empty()) {
    HandleSubscription();
  }

  if (success) {
    if (!m_queryQueue.empty()) {
      QueryItem& qitem = m_queryQueue.front();

      if (!m_dberror.GetError()) {
        if (strcasecmp("begin transaction", qitem.sqlQuery.c_str()) == 0) {
          this->m_transactionStarted = true;
        } else if (strcasecmp("commit transaction", qitem.sqlQuery.c_str()) == 0) {
          this->m_transactionStarted = false;
        }
      }

      qitem.cb(m_dberror, rst, qitem.ctx);
      m_queryQueue.pop();

      if (!m_queryQueue.empty()) {
        success = SendNextQueryAsync();
        if (!success) {
          //CancelQueue(true);
          QueryItem& qitem = m_queryQueue.front();
          qitem.cb(m_dberror, NULL, qitem.ctx);
          m_queryQueue.pop();
        }
      }
    } else {
      printf("[ERROR] There is no request item in the query queue for this reply.\n");
      printf("SQLState: %s, Message: %s, Detail: %s, Hint: %s\n",
          SAFE_STRING(m_dberror.GetSQLState()), SAFE_STRING(m_dberror.GetMessage()),
          SAFE_STRING(m_dberror.GetDetail()), SAFE_STRING(m_dberror.GetHint()));
    }
  }
  delete rst;
}

void PGClient::WriteBytes() {
  // printf("PGClient::WriteBytes callback function start\n");
  int flushResult = PQflush(m_pgconn);
  if (flushResult == 0) {
    // There is no data left in send queue
    DeleteWriteEvent();
  } else if (flushResult == 1) {
    // There is some data left in send queue
    AddWriteEvent();
  } else if (flushResult == -1) {
    const char* err_msg = PQerrorMessage(m_pgconn);
    SetLastError("ERROR", "08006", err_msg);
    printf("[PGClient::WriteBytes] callback function error in PQflush: %s\n", SAFE_STRING(err_msg));
  }
  // printf("PGClient::WriteBytes callback function end\n");
}

void PGClient::OnEvents(uint32_t events)
{
  //printf("[PGClient::OnEvents] events: %d\n", events);
  if (events & FileEvent::WRITE) {
    WriteBytes();
  }
  if (events & FileEvent::READ) {
    ReadBytes();
  }
  if (events & FileEvent::CLOSED || !IsConnected()) {
    OnClosed();
  }
  if (events & FileEvent::ERROR) {
    OnError(errno, strerror(errno));
  }
  if (m_dberror.GetError()) {
    OnError(-1, m_dberror.GetMessage());
  }
}

void PGClient::RemoveFDHandler()
{
  if (FD() > 0) {
    printf("PGClient::Disconnect removed database socket(fd: %d) from event handler\n", FD());
    EV_Singleton->DeleteEvent(this);
    SetFD(-1);
  }
}

void PGClient::OnReconnectTimer(TimerEvent* timer)
{
  printf("[PGClient::OnReconnectTimer] Timer tick %u\n", timer->GetInterval().Seconds());
  if (!IsConnected()) {  // if the connection is not created, then reconnect
    bool success = Connect_();
    if (success) {
      timer->Stop();
      OnConnected();
    } else {
      printf("[PGClient::OnReconnectTimer] Reconnect failed, retry %u seconds later...\n", timer->GetInterval().Seconds());
    }
  } else {
    timer->Stop();
  }
}

void PGClient::OnConnected()
{
  printf("[PGClient::OnConnected] connection created, fd: %d\n", FD());
  m_dberror.Clear();
  if (m_db_cbs) m_db_cbs->on_connected_cb(this);
}
void PGClient::OnClosed()
{
    printf("[PGClient::OnClosed] connection lost, fd: %d\n", FD());
    RemoveFDHandler();
    if (m_db_cbs) m_db_cbs->on_closed_cb(this);
    if (m_auto_reconnect) {
      Reconnect();
    }
}
void PGClient::OnError(int errcode, const char* errstr)
{
    printf("[PGClient::OnError] fd: %d, error code: %d, error string: %s\n", FD(), errcode, errstr);
    if (m_db_cbs) m_db_cbs->on_error_cb(this, errcode, errstr);
}

string PGClient::QueryItem::ToString() const
{
  stringstream ss;
  ss << "{ sql: " << sqlQuery << ", params: [";
  size_t params_size = params.size();
  for (size_t i=0; i<params_size; i++) {
    ss << "'" << params[i].ToString() << "'";
    if (i != params_size - 1)
      ss << ", ";
  }
  ss << "] }";
  return ss.str();
}

}  // namespace db_api

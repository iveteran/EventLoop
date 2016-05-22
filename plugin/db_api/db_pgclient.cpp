#include <sstream>
#include "db_pgclient.h"

namespace db_api {

// Error Messages used inside PGClient class
const   char*   NOCONNECTION = "No database connection is made yet!";
const   char*   CONNECTION_LOST = "Database connection is lost";
const   char*   ERROREXEC = "Error occurred in executing";
const   char*   CONNECTION_FAILED = "Connection to the database failed";
const   char*   NO_NONBLOCKINGCALL_REQUESTED   = "There is no non-blocking sql query requested";
const   char*   ALREADY_NONBLOCKINGCALL_REQUESTED   = "There is already another non-blocking sql query requested";
const   char*   INVALID_TRANSACTION = "There is an already running transaction";
const   char*   CANCELLED_REQUEST = "The request is cancelled due to rollback";

PGClient::PGClient()
  : m_pgconn(NULL), 
  m_transactionStarted(false)
{ }

PGClient::~PGClient() {
  Disconnect();
}

bool PGClient::Connect(const map<string,string>& connInfo) {
  printf("PGClient::connect starts\n");
  bool success = false;

  if (m_pgconn != NULL) {
    Disconnect();
  }

  string connectionString;
  map<string, string>::const_iterator it;
  for (it = connInfo.begin(); it != connInfo.end(); it++) {
    connectionString += it->first;
    connectionString += "=";
    connectionString += it->second;
    connectionString += " ";
  }

  printf("connection string: %s\n", connectionString.c_str());
  m_pgconn = PQconnectdb((const char*)connectionString.c_str());
  if (PQstatus(m_pgconn) != CONNECTION_OK) {
    const char* errMessage = PQerrorMessage(m_pgconn);
    SetLastError("ERROR", "08006", errMessage);
    printf("Connect to database(%s) failed: %s\n", connectionString.c_str(), SAFE_STRING(errMessage));
  } else {
    success = true;
    SetFD(PQsocket(m_pgconn));
  }
  printf("PGClient::connect end: %d\n", success);
  return  success;
}

void PGClient::Disconnect() {
  if (m_pgconn != NULL) {
    printf("PGClient::Disconnect start\n");
    if (m_transactionStarted) {
      m_transactionStarted = false;
    }
    EV_Singleton->DeleteEvent(this);
    SetFD(-1);
    printf("PGClient::Disconnect removed database socket from event handler\n");

    PQfinish(m_pgconn);
    m_pgconn = NULL;

    while (!m_queryQueue.empty()) m_queryQueue.pop();
    m_subscribeMap.clear();
    m_transactionStarted = false;
    printf("PGClient::Disconnect PQfinish done\n");
  }
  printf("PGClient::Disconnect end\n");
}

bool PGClient::IsConnected() {
  bool success = false;
  if (m_pgconn != NULL) {
    const char* sqlstate = m_dberror.GetSQLState();

    success = strcmp(sqlstate, "08006") &&   // connection_failure
      strcmp(sqlstate, "57000") &&   // operator_intervention
      strcmp(sqlstate, "57P01") &&   // admin_shutdown
      strcmp(sqlstate, "57P02") &&   // crash_shutdown
      strcmp(sqlstate, "57P03") &&   // cannot_connect_now
      strcmp(sqlstate, "57P03") &&   // cannot_connect_now
      strcmp(sqlstate, "53300") &&   // too_many_connections
      (PQstatus(m_pgconn) == CONNECTION_OK);
  }
  return success;
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

// beginTransaction is a nonblocking call
bool PGClient::BeginTransaction(const DBResultCallback& cb, void* ctx) {
  printf("PGClient::beginTransaction start\n");
  bool    success = false;
  if (m_pgconn != NULL) {
    if (!m_transactionStarted) {
      m_last_cmd = BEGINTRANSACTION;
      vector<SQLParameter>  params;
      success = AddNonBlockingSQL("begin transaction", params, cb, ctx);
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

// commitTransaction is a nonblocking call
bool PGClient::CommitTransaction(const DBResultCallback& cb, void* ctx) {
  printf("PGClient::commitTransaction start\n");
  bool    success = false;
  if (m_pgconn != NULL) {
    m_last_cmd = COMMITTRANSACTION;
    vector<SQLParameter>  params;
    success = AddNonBlockingSQL("commit transaction", params, cb, ctx);
  } else {
    //m_last_error = NOCONNECTION;
    SetLastError("ERROR", "08006", NOCONNECTION);
    printf("PGClient::commitTransaction there was no connection made yet.\n");
  }
  printf("PGClient::commitTransaction end %d\n", success);
  return  success;
}

bool PGClient::CancelCurrentQuery() {
  printf("PGClient::cancelCurrentQuery start\n");
  bool success = false;
  if (m_pgconn != NULL) {
    char    errbuf[256];
    PGcancel*   cancel = PQgetCancel(m_pgconn);
    int     tmpResult = PQcancel(cancel, errbuf, sizeof(errbuf));
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

    vector<SQLParameter>  params;
    DBResult*  rst = RunBlockingSQL("rollback transaction", params, success);
    if (rst != NULL) {
      success = true;
      delete rst;
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

PGResult* PGClient::RunBlockingSQL(const char* sql, bool& success) {
  vector<SQLParameter> params;
  return RunBlockingSQL(sql, params, success);
}

PGResult* PGClient::RunBlockingSQL(const char* sql, const vector<SQLParameter>& params, bool& success) {
  printf("PGClient::RunBlockingSQL start %s\n", SAFE_STRING(sql));
  PGResult*  result = 0;
  success = false;

  if (m_pgconn != NULL) {
    char**  paramsValue = (char**)NULL;
    int     paramsCount = params.size();
    int*    paramLengths = NULL;
    int*    paramFormats = NULL;

    if (paramsCount > 0) {
      paramsValue = new char*[paramsCount];
      paramLengths = new int[paramsCount];
      paramFormats = new int[paramsCount];
      for (int i = 0; i < paramsCount; i++) {
        paramsValue[i] = params[i].paramValue;
        paramLengths[i] = params[i].paramLength;
        paramFormats[i] = params[i].paramFormat;
      }
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
        printf("PGClient::RunBlockingSQL [%s].\n", SAFE_STRING(PQresultErrorMessage(res)));
      }
    } else {
      const char* errMessage = PQerrorMessage(m_pgconn);
      SetLastError("ERROR", "00000", errMessage);
      printf("PGClient::RunBlockingSQL [%s].\n", SAFE_STRING(errMessage));
    }
    if (paramFormats != NULL) delete[] paramFormats;
    if (paramLengths != NULL) delete[] paramLengths;
    if (paramsValue != NULL) delete[] paramsValue;
  } else {
    SetLastError("ERROR", "08006", NOCONNECTION);
    printf("PGClient::RunBlockingSQL there was no connection made yet.\n");
  }

  printf("PGClient::RunBlockingSQL end %d\n", success);
  return  result;
}

bool PGClient::SendNextQueryRequestInNonBlocking()
{
  printf("PGClient::sendNextQueryRequestInNonBlocking start\n");
  bool    success = false;

  if (m_queryQueue.size() > 0) {
    QueryItem qitem = m_queryQueue.front();
    const char* sql = qitem.sqlQuery.c_str();
    char**  paramsValue = (char**)NULL;
    int*    paramLengths = (int*)NULL;
    int*    paramFormats = (int*)NULL;
    int     paramsCount = qitem.parameters.size();

    if (paramsCount > 0) {
      paramsValue = new char*[paramsCount];
      paramLengths = new int[paramsCount];
      paramFormats = new int[paramsCount];
      for (int i = 0; i < paramsCount; i++) {
        paramsValue[i] = qitem.parameters[i].paramValue;
        paramLengths[i] = qitem.parameters[i].paramLength;
        paramFormats[i] = qitem.parameters[i].paramFormat;
      }
    }

    printf("PGClient::sendNextQueryRequestInNonBlocking execute sql:%s, params:[%s]\n",
        SAFE_STRING(sql), ShowParamValues(paramsCount, paramsValue));
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
        const char* errMessage = PQerrorMessage(m_pgconn);
        SetLastError("ERROR", "08006", errMessage);
        printf("PGClient::sendNextQueryRequestInNonBlocking error1:%s\n", SAFE_STRING(errMessage));
      }
    } else {
      const char* errMessage = PQerrorMessage(m_pgconn);
      SetLastError("ERROR", "08006", errMessage);
      printf("PGClient::sendNextQueryRequestInNonBlocking error2:%s\n", SAFE_STRING(errMessage));
    }
    if (paramFormats != NULL) delete[] paramFormats;
    if (paramLengths != NULL) delete[] paramLengths;
    if (paramsValue != NULL) delete[] paramsValue;
  } else {
    // The queue is empty. Nothing to request
    success = true;
  }

  printf("PGClient::sendNextQueryRequestInNonBlocking end %d\n", success);
  return  success;
}

bool PGClient::AddNonBlockingSQL(const char* sql, const DBResultCallback& cb, void* ctx) {
  vector<SQLParameter> dummy;
  return AddNonBlockingSQL(sql, dummy, cb, ctx);
}

bool PGClient::AddNonBlockingSQL(const char* sql, const vector<SQLParameter>& params, const DBResultCallback& cb, void* ctx) {
  bool success = true;
  m_queryQueue.push(QueryItem(sql, params, cb, ctx));
  if (m_queryQueue.size() == 1) {
    success = SendNextQueryRequestInNonBlocking();
    if (!success) {
      m_queryQueue.pop();
      cb(m_dberror, NULL, ctx);
    }
  }

  // printf("PGClient::AddNonBlockingSQL end %d\n", success);
  return  success;
}

void PGClient::OnEvents(uint32_t events)
{
  //printf("[PGClient::OnEvents] events: %d\n", events);
  if (events & IOEvent::WRITE) {
    write();
  }
  if (events & IOEvent::READ) {
    read();
  }
  if (events & IOEvent::ERROR) {
    OnError(errno, strerror(errno));
  }
}

void PGClient::write() {
  // printf("PGClient::write callback function start\n");
  int flushResult = PQflush(m_pgconn);
  if (flushResult == 0) {
    // There is no data left in send queue
    DeleteWriteEvent();
  } else if (flushResult == 1) {
    // There is some data left in send queue
    AddWriteEvent();
  } else if (flushResult == -1) {
    const char* errMessage = PQerrorMessage(m_pgconn);
    SetLastError("ERROR", "08006", errMessage);
    printf("PGClient::write callback function error in PQflush: %s\n", SAFE_STRING(errMessage));
  }
  // printf("PGClient::write callback function end\n");
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

void PGClient::DummyRollbackCb(const DBError& err, DBResult* rst, void* ctx) {
  (void)err;
  (void)rst;
  (void)ctx;
}

void PGClient::read() {
  bool success = false;
  PGResult* rst = NULL;

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

  if (m_subscribeMap.size() > 0) {
    HandleSubscription();
  }

  if (success) {
    if (!m_queryQueue.empty()) {
      QueryItem qitem = m_queryQueue.front();
      const char* sql = qitem.sqlQuery.c_str();
      DBResultCallback& cb = qitem.cb;
      void* callerContext = qitem.ctx;
      m_queryQueue.pop();

      if (!m_dberror.GetError()) {
        if (strcmp("begin transaction", sql) == 0) {
          this->m_transactionStarted = true;
        } else if (strcmp("commit transaction", sql) == 0) {
          this->m_transactionStarted = false;
        }
      }

      cb(m_dberror, rst, callerContext);

      if (!m_queryQueue.empty()) {
        success = SendNextQueryRequestInNonBlocking();
        if (!success) {
          CancelQueue(true);
        }
      }
    } else {
      printf("[ERROR] There is no request item in the query queue for this reply.\n");
      printf("SQLState: %s, Message: %s, Detail: %s, Hint: %s\n",
          SAFE_STRING(m_dberror.GetSQLState()), SAFE_STRING(m_dberror.GetMessage()),
          SAFE_STRING(m_dberror.GetDetail()), SAFE_STRING(m_dberror.GetHint()));
    }
  } else {
    if (!IsConnected()) {
      CancelQueue(true);
    }
  }
  delete rst;
}

bool PGClient::HandleSubscription() {
  if (PQconsumeInput(m_pgconn) == 1) {
    PGnotify*   tmpNotify = NULL;
    DBError    noerror;
    while ((tmpNotify = PQnotifies(m_pgconn)) != NULL) {
      string  x = tmpNotify->relname;
      string  message = tmpNotify->extra;
      printf("PGClient::read invokes registered subscription callback function with [%s]\n", message.c_str());
      m_subscribeMap[x](noerror, x, message);
      PQfreemem(tmpNotify);
    }
    return true;
  } else {
    if (PQstatus(m_pgconn) != CONNECTION_OK) {
      const char* errMessage = PQerrorMessage(m_pgconn);
      SetLastError("ERROR", "08006", errMessage);

      map<string, SubscribeCallback>::iterator it;
      for (it = m_subscribeMap.begin(); it != m_subscribeMap.end(); ++it) {
        it->second(m_dberror, it->first, "");
      }
    }
    return false;
  }
}
PGResult* PGClient::PollResultSet(bool& success) {
  PGResult* rst = NULL;
  success = false;

  PQconsumeInput(m_pgconn);
  if (PQisBusy(m_pgconn) == 0) {
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
        printf("PGClient::pollReseultset received an error message: [%s]\n", SAFE_STRING(PQresultErrorMessage(result)));
      }
      while ((result = PQgetResult(m_pgconn)) != NULL) {
        PQclear(result);
      }
    } else {
      const char* errMessage = PQerrorMessage(m_pgconn);
      SetLastError("ERROR", "00000", errMessage);
      printf("PGClient::pollResultset postgresql returned empty resultset although it is supposed to return a resultset. [%s]\n",
          SAFE_STRING(errMessage));
    }
  } else {
    if (PQstatus(m_pgconn) != CONNECTION_OK) {
      const char* errMessage = PQerrorMessage(m_pgconn);
      SetLastError("ERROR", "08006", errMessage);
      printf("PGClient::pollResultset connection lost: [%s]\n", SAFE_STRING(errMessage));
    }
  }

  return  rst;
}

bool PGClient::AddSubscribeChannel(const char*  channelName, const SubscribeCallback& cb) {
  printf("PGClient::addSubscribeChannel start: channelName [%s]\n", SAFE_STRING(channelName));
  bool success = false;
  stringstream ss;
  ss << "LISTEN " << channelName;
  string tmpSQL = ss.str();

  PGResult* tmpResult = RunBlockingSQL(tmpSQL.c_str(), success);
  if (tmpResult != NULL) {
    m_last_cmd = SUBSCRIBE;
    delete tmpResult;

    // Now add new subscibe callback function
    tmpSQL = channelName;
    m_subscribeMap[tmpSQL] = cb;
  }
  printf("PGClient::addSubscribeChannel end: %d\n", success);
  return  success;
}

bool PGClient::RemoveSubscribeChannel(const char*   channelName) {
  printf("PGClient::removeSubscribeChannel start: channelName [%s]\n", channelName);
  bool    success = false;

  stringstream ss;
  ss << "UNLISTEN " << channelName;
  string  tmpSQL = ss.str();
  // Refer to addSubscribeChannel function for calling either blocking call or nonblocking call.
  PGResult*  tmpResult = RunBlockingSQL(tmpSQL.c_str(), success);
  if (tmpResult != NULL) {
    m_last_cmd = UNSUBSCRIBE;
    delete tmpResult;

    // Now remove subscibe callback function regardless of success or failure
    tmpSQL = channelName;
    m_subscribeMap.erase(tmpSQL);
  }
  printf("PGClient::removeSubscribeChannel end: %d\n", success);
  return  success;
}

//
// For each parameter, the maximum value length will be 20. (Hard coded)
//
const char* PGClient::ShowParamValues(int count, char* paramValues[]) {
  const   int     BUFFER_SIZE = 80000;
  static  char    szReturnBuffer[BUFFER_SIZE + 1] = {0,};
  int     leftBufferSize = BUFFER_SIZE;
  int     tmpStrLen = 0;

  if (count > 0) {
    if (paramValues[0] != NULL) {
      if (leftBufferSize > (tmpStrLen = strlen(paramValues[0])+2)) {
        sprintf(szReturnBuffer, "'%s'", paramValues[0]);
        leftBufferSize -= tmpStrLen;
      } else {
        sprintf(szReturnBuffer, "'%.*s'", leftBufferSize-2, paramValues[0]);
        leftBufferSize = 0;
      }
    } else {
      strcpy(szReturnBuffer, "null");
      leftBufferSize -= 4;
    }

    for (int i = 1; i < count && leftBufferSize > 0; ++i) {
      if (paramValues[i] != NULL) {
        if (leftBufferSize > (tmpStrLen = strlen(paramValues[i])+3)) {
          sprintf(szReturnBuffer + strlen(szReturnBuffer), ",'%s'", paramValues[i]);
          leftBufferSize -= tmpStrLen;
        } else {
          sprintf(szReturnBuffer + strlen(szReturnBuffer), ",'%.*s'", leftBufferSize-3, paramValues[i]);
          szReturnBuffer[BUFFER_SIZE] = '\0';
          leftBufferSize = 0;
        }
      } else {
        if (leftBufferSize >= 5) {
          strcat(szReturnBuffer,",null");
          leftBufferSize -= 5;
        } else {
          sprintf(szReturnBuffer + strlen(szReturnBuffer), ",%.*s", leftBufferSize-1, "null");
          szReturnBuffer[BUFFER_SIZE] = '\0';
          leftBufferSize = 0;
        }
      }
    }
  } else {
    szReturnBuffer[0] = '\0';
  }

  return szReturnBuffer;
}

}  // namespace db_api

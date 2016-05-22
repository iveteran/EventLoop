#ifndef _DB_CLIENT_H
#define _DB_CLIENT_H

#include <string.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <iostream>
#include <functional>

using namespace std;

#define SAFE_STRING(X) ((X != NULL)?X:"")

namespace db_api {

class DBConnection;

class DBError
{
  public:
  DBError(bool error = false, const char* severity = NULL, const char* sqlstate = NULL,
      const char* message = NULL, const char* detail = NULL, const char* hint = NULL);
  DBError(const DBError&);
  bool GetError() const;
  const char* GetSeverity() const;
  const char* GetSQLState() const;
  const char* GetMessage() const;
  const char* GetDetail() const;
  const char* GetHint() const;
  void SetError(bool error);
  void SetSeverity(const char* severity);
  void SetSQLState(const char* sqlstate);
  void SetMessage(const char* message);
  void SetDetail(const char* detail);
  void SetHint(const char* hint);
  DBError& operator=(const DBError&);

  private:
  bool m_error;
  string m_severity;
  string m_sqlstate;
  string m_message;
  string m_detail;
  string m_hint;
};

struct SQLParameter
{
  SQLParameter(const char* value = NULL, int length = 0, int format = 0) :
    paramValue((char*)value), paramLength(length), paramFormat(format) {
      Initialize(value, length, format);
  }
  SQLParameter(int value) {
    Initialize(std::to_string(value).c_str(), 0, 0);
  }
  SQLParameter(const SQLParameter& right) {
    Initialize(right.paramValue, right.paramLength, right.paramFormat);
  }
  ~SQLParameter() {
    if (paramValue) delete[] paramValue;
  }

  bool operator==(const SQLParameter& right) const {
    if (paramLength == right.paramLength) {
      if (paramLength > 0) {
        return memcmp(paramValue, right.paramValue, paramLength) == 0;
      } else {
        return true;
      }
    } else {
      return  false;
    }
  }
  bool operator!=(const SQLParameter& right) const {
    return !(*this == right);
  }

  SQLParameter& operator=(const SQLParameter& right) {
    if (*this != right) {
      if (paramValue) delete[] paramValue;
      Initialize(right.paramValue, right.paramLength, right.paramFormat);
    } 
    return *this;
  }

  void Initialize(const char* value, int length, int format) {
    if (value != NULL) {
      if (length == 0 && format == 0) {
        length = strlen(value);
      }
      paramValue = new char[length + 1];
      memcpy(paramValue, value, length);
      paramValue[length] = '\0';
    } else {
      paramValue = NULL;
    }
    paramLength = length;
    paramFormat = format;
  }

  char* paramValue;
  int paramLength;
  int paramFormat;
};

class DBResult
{
  public:
  virtual ~DBResult() {}
  //virtual bool is_ok() const = 0;
  virtual int AffectedRows() const = 0;
  virtual int RowCount() const = 0;
  virtual int ColCount() const = 0;
  virtual bool IsNull(int row, int col) const = 0;
  virtual const char* GetValue(int row, int col) const = 0;
  virtual const unsigned char* GetUnescapeValue(int row, int col, size_t* length) const = 0;
  virtual int GetIntValue(int row, int col) const = 0;
  virtual bool GetBoolValue(int row, int col) const = 0;
  virtual double GetDoubleValue(int row, int col) const = 0;
  virtual int GetColumnIndex(const char* column_name) const = 0;
  virtual const char* GetFieldname(int col) const = 0;
  virtual DBResult* Clone() const = 0;
  virtual DBConnection *GetDBConnection() const = 0;
};

typedef std::function<void (const DBError&, DBResult*, void*)> DBResultCallback;
typedef std::function<void (const DBError&, const string&, const string&)>  SubscribeCallback;

class DBConnection
{
  public:
  virtual ~DBConnection() {}
  virtual bool Connect(const map<string, string>& connInfo) = 0;
  virtual void Disconnect () = 0;
  virtual bool IsConnected() = 0;
  virtual int GetWaitingSQLCount() = 0;
  virtual bool CancelCurrentQuery() = 0;
  virtual void CancelQueue(bool invokeCallbackFunction) = 0;
  virtual bool IsInTransactionBlock() = 0;
  virtual bool BeginTransaction(const DBResultCallback& cb, void *ctx) = 0;
  virtual bool CommitTransaction(const DBResultCallback& cb, void* ctx) = 0;
  virtual bool RollbackTransaction() = 0;
  virtual DBResult* RunBlockingSQL(const char* sql, bool& success) = 0;
  virtual DBResult* RunBlockingSQL(const char* sql, const vector<SQLParameter>& params, bool& success) = 0;
  virtual bool AddNonBlockingSQL(const char* sql, const DBResultCallback& cb, void* ctx) = 0;
  virtual bool AddNonBlockingSQL(const char* sql, const vector<SQLParameter>& params, const DBResultCallback& cb, void* ctx) = 0;
  virtual bool AddSubscribeChannel(const char* channelName, const SubscribeCallback& cb) = 0;
  virtual bool RemoveSubscribeChannel(const char* channelName) = 0;

  virtual const DBError&  GetLastError() = 0;
};

} // namespace db_api

#endif  // _DB_CLIENT_H

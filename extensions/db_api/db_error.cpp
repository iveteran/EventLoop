#include <sstream>
#include "db_client.h"

namespace db_api {

DBError::DBError(
    bool error,
    const char* severity,
    const char* sqlstate,
    const char* message,
    const char* detail,
    const char* hint)
  : m_error(error),
  m_severity(SAFE_STRING(severity)), 
  m_sqlstate(SAFE_STRING(sqlstate)),
  m_message(SAFE_STRING(message)),
  m_detail(SAFE_STRING(detail)),
  m_hint(SAFE_STRING(hint))
  { }

DBError::DBError(const DBError& right) {
  *this = right;
}

void DBError::Clear() {
  m_error = false;
}

bool DBError::GetError() const {
  return m_error;
}

void DBError::SetError(bool error) {
  m_error = error;
}

const char* DBError::GetSeverity() const {
  return m_severity.c_str();
}

void DBError::SetSeverity(const char* severity) {
  m_severity = SAFE_STRING(severity);
}

const char* DBError::GetSQLState() const {
  return m_sqlstate.c_str();
}

void DBError::SetSQLState(const char* sqlstate) {
  m_sqlstate = SAFE_STRING(sqlstate);
}

const char* DBError::GetMessage() const {
  return m_message.c_str();
}

void DBError::SetMessage(const char* message) {
  m_message = SAFE_STRING(message);
}

const char* DBError::GetDetail() const {
  return m_detail.c_str();
}

void DBError::SetDetail(const char* detail) {
  m_detail = SAFE_STRING(detail);
}

const char* DBError::GetHint() const {
  return m_hint.c_str();
}

void DBError::SetHint(const char* hint) {
  m_hint = SAFE_STRING(hint);
}

DBError& DBError::operator=(const DBError& right) {
  SetError(right.GetError());
  SetSeverity(right.GetSeverity());
  SetSQLState(right.GetSQLState());
  SetMessage(right.GetMessage());
  SetDetail(right.GetDetail());
  SetHint(right.GetHint());
  return *this;
}

string DBError::ToString() const {
  stringstream ss;
  ss << "{ error: " << m_error
    << ", Severity: " << m_severity
    << ", SQLState: " << m_sqlstate
    << ", Message: " << m_message
    << ", Detail: " << m_detail
    << ", Hint: " << m_hint
    << " }";
  return ss.str();
}

}  // namespace db_api

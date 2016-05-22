#include <ctype.h>
#include "db_pgclient.h"

namespace db_api {

PGResult::PGResult(PGClient* connection, PGresult* result) :
  m_connection(connection), m_result(result) {
}

PGResult::~PGResult() {
  if (m_result != 0) {
    PQclear(m_result);
    m_result = 0;
  }
}

int PGResult::AffectedRows() const {
  int result = 0;

  if (m_result != 0) {
    char*   tmpResult = PQcmdTuples(m_result);
    if (tmpResult != NULL) {
      result = atoi(tmpResult);
    }
  }

  return result;
}

int PGResult::RowCount() const {
  if (m_result != 0) {
    return PQntuples(m_result);
  }
  return 0;
}

int PGResult::ColCount() const {
  if (m_result != 0) {
    return PQnfields(m_result);
  }
  return 0;
}

bool PGResult::IsNull(int row, int column) const {
  bool    result = true;

  if (m_result != 0) {
    if (column < PQnfields(m_result)) {
      result = PQgetisnull(m_result, row, column);
    }
  }
  return result;
}

//
// get_value returns a string value for given element (row, column).
// Note that row and column start from 0
//
const char* PGResult::GetValue(int row, int column) const {
  const char* value = "";
  if (m_result != 0) {
    if (column < PQnfields(m_result)) {
      value = (const char*)PQgetvalue(m_result, row, column);
    }
  }
  return value;
}

// The caller must execute delete[] to avoid memory leak
const unsigned char* PGResult::GetUnescapeValue(int row, int column, size_t* length) const {
  unsigned char* value = NULL;

  if (m_result != 0) {
    if (column < PQnfields(m_result)) {
      unsigned char* tmpValue = PQunescapeBytea((const unsigned char*)PQgetvalue(m_result, row, column), length);
      value = new unsigned char[*length + 1];
      memcpy(value, tmpValue, *length);
      value[*length] = '\0';
      PQfreemem(tmpValue);
    }
  }
  return  (const unsigned char*)value;
}

int PGResult::GetIntValue(int row, int column) const {
  if (m_result != 0) {
    if (column < PQnfields(m_result)) {
      if (0 == PQfformat(m_result, column)) {
        return (int)strtol(PQgetvalue(m_result,row,column), (char **)NULL, 0);
      } else {
        if (PQgetlength(m_result, row, column) >= (int)sizeof(int)) {
          int *pval = (int*)PQgetvalue(m_result, row, column);
          if (pval) {
            return *pval;
          }
        }
      }
    }
  }

  return -1;
}

double PGResult::GetDoubleValue(int row, int column) const {
  if (m_result != 0) {
    if (column < PQnfields(m_result)) {
      if (0 == PQfformat(m_result, column)) {
        return strtod(PQgetvalue(m_result, row, column), (char**)NULL);
      }
    }
  }
  return 0;
}

bool PGResult::GetBoolValue(int row, int column) const {
  if (m_result != 0) {
    if (column < PQnfields(m_result)) {
      const char* val = PQgetvalue(m_result, row, column);
      return val && (tolower(val[0]) == 't');
    }
  }
  return false;
}

int PGResult::GetColumnIndex(const char* column_name) const {
  int col_index = -1;
  if (m_result != 0 && column_name != NULL) {
    col_index = PQfnumber(m_result, column_name);
  }
  return col_index;
}

const char* PGResult::GetFieldname(int column) const {
  const char* field_name = "";
  if (m_result != 0) {
    if (column < PQnfields(m_result)) {
      field_name = PQfname(m_result, column);
    }
  }
  return field_name;
}

PGResult*  PGResult::Clone() const {
  PGResult*  result = NULL;
  if (m_result != 0) {
    PGresult*   tmp = PQcopyResult(m_result, PG_COPYRES_TUPLES);
    result = new PGResult(m_connection, tmp);
  }
  return  result;
}

DBConnection*  PGResult::GetDBConnection() const {
  return m_connection;
}
}  // namespace db_api

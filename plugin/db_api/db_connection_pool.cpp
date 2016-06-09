#include <algorithm>
#include "db_connection_pool.h"
#include "db_client.h"

namespace db_api {

DBConnectionPool* DBConnectionPool::m_instance = NULL;

DBConnectionPool* DBConnectionPool::Create(const str2strmap& conn_info, size_t size, DBFactory::DBType type)
{
  if(!m_instance) {
    m_instance = new DBConnectionPool(conn_info, size, type);
  }
  return m_instance;
}

size_t DBConnectionPool::Resize(int delta)
{
  DBConnection* conn = 0;
  if(delta > 0) {
    for(size_t i = 0; i < static_cast<size_t>(delta); ++i) {
      if(m_size >= SZ_MAX)
        break;

      conn= DBFactory::CreateDBConnection(m_type);

      if(i < SZ_DFLT)
        conn->Connect(m_conn_info);
      m_pool.push_back(conn);
      m_shadow_pool.insert(conn);
      m_size++;
    }
  } else {
    for(int i = delta; i < 0; ++i) {
      if(m_size <= SZ_MIN)
        break;
      if(m_pool.size() == static_cast<size_t>(m_size)) {
        conn = m_pool.back();
        if(conn->IsConnected())
          conn->Disconnect();
        m_shadow_pool.erase(conn);
        delete conn;
        conn = 0;
        m_pool.pop_back();
      }
      m_size--;
    }
  }
  return m_size;
}

bool DBConnectionPool::Reset(size_t newSize)
{
  if(Resize(-m_size) != 0)
    return false;
  if(Resize(newSize) != newSize)
    return false;

  return true;
}

DBConnectionPool::DBConnectionPool(const str2strmap& conn_info, size_t size, DBFactory::DBType type):
  m_conn_info(conn_info), m_size(0), m_type(type)
{
  DBConnection* conn = 0;
  for(size_t i = 0; i < size; ++i) {
    if(m_size >= SZ_MAX)
      break;
    conn = DBFactory::CreateDBConnection(m_type);
    if(i < SZ_DFLT)
      conn->Connect(m_conn_info);
    m_pool.push_back(conn);
    m_shadow_pool.insert(conn);
    m_size++;
  }
}

DBConnectionPool::~DBConnectionPool()
{
  if(!m_pool.empty())
    m_pool.pop_front();

  if(!m_shared_pool.empty())
    m_shared_pool.clear();

  DBConnection* conn = 0;
  for(auto it = m_shadow_pool.begin(); it != m_shadow_pool.end(); ++it) {
    conn = *it;
    if(conn) {
      if(conn->IsConnected())
        conn->Disconnect();
      delete conn;
      conn = 0;
    }
  }
  m_shadow_pool.clear();
}

void DBConnectionPool::Destroy()
{
  delete m_instance;
}

DBConnection* DBConnectionPool::GetConnection(t_access access)
{
  if(SHR == access) {
    return GetShared();
  }
  else if(EXL == access) {
    return GetExclusive();
  }
  return 0;
}

DBConnection* DBConnectionPool::GetExclusive()
{
  DBConnection* conn = 0;

  if(!m_pool.empty()) {
    conn = m_pool.front();
    if(!conn->IsConnected()) {
      conn->Connect(m_conn_info);
    }
    m_pool.pop_front();
  }

  return conn;
}

DBConnection* DBConnectionPool::GetShared()
{
  DBConnection* conn = 0;
  if(AvailSlotsCount() > 0) {
    conn = GetExclusive();
    m_shared_pool.push_back(conn);
  } else if(m_shared_pool.size() > 0) {
    list<DBConnection*>::const_iterator it = std::min_element(m_shared_pool.begin(), m_shared_pool.end(), 
        DBConnectionPool::LeastShared);
    conn = *it;
    if (!conn->IsConnected()) {
      conn->Connect(m_conn_info);
    }
  }

  return conn;
}

bool DBConnectionPool::LeastShared(DBConnection* a, DBConnection* b)
{
  return a->GetWaitingSQLCount() < b->GetWaitingSQLCount();
}

DBConnection* DBConnectionPool::LookupConnection(DBConnection* conn)
{
  auto it = find(m_pool.begin(), m_pool.end(), conn);
  if(it == m_pool.end()) {
    return 0;
  } else {
    m_pool.erase(it);
    return conn;
  }
}

void DBConnectionPool::ReleaseConnection(DBConnection* conn)
{
  auto it = find(m_shared_pool.begin(), m_shared_pool.end(), conn);
  if(it != m_shared_pool.end()) {
    m_shared_pool.erase(it);
  }
  if(m_pool.size() < static_cast<size_t>(m_size)) {
    m_pool.push_back(conn);
  } else {
    if(conn) {
      if(conn->IsConnected()) {
        conn->Disconnect();
      }
      m_shadow_pool.erase(conn);
      delete conn;
    }
    conn = 0;
  }
}

} // namespace db_api

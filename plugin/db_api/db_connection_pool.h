#ifndef _DB_CONNECTION_POOL_H
#define _DB_CONNECTION_POOL_H

#include <string>
#include <list>
#include <map>
#include <set>
#include "db_pgclient.h"
#include "db_factory.h"

using std::string;
using std::set;
using std::map;
using std::list;

namespace db_api {

class DBConnectionPool
{
  public:
    enum t_access { EXL, SHR };

    static DBConnectionPool* Create(const str2strmap& conn_info,
        size_t size = DBConnectionPool::SZ_DFLT,
        DBFactory::DBType type = DBFactory::POSTGRESQL);
    static void Destroy();

    size_t Resize(int delta);
    bool Reset(size_t newSize);
    DBConnection* GetConnection(t_access access = EXL);	    
    DBConnection* LookupConnection(DBConnection* conn );
    void ReleaseConnection(DBConnection* conn);
    int TotalSlotsCount() const { return m_size; }
    int AvailSlotsCount() const { return m_pool.size(); }

    static bool LeastShared(DBConnection* a, DBConnection* b);

  private:
    static const size_t SZ_DFLT = 20;
    static const size_t SZ_MAX  = 200;
    static const size_t SZ_MIN  = 0;

    static DBConnectionPool* m_instance;

    DBConnectionPool(const str2strmap& conn_info, size_t size, DBFactory::DBType type);	
    DBConnectionPool(const DBConnectionPool&);
    DBConnectionPool& operator=(const DBConnectionPool&);	
    ~DBConnectionPool();

    DBConnection* GetExclusive();	    
    DBConnection* GetShared();	    

    list<DBConnection*>   m_pool;
    set<DBConnection*>    m_shadow_pool;
    list<DBConnection*>   m_shared_pool;
    const str2strmap      m_conn_info;
    size_t                m_size;
    DBFactory::DBType     m_type;
};

} // namespace db_api
#endif  // _DB_CONNECTION_POOL_H

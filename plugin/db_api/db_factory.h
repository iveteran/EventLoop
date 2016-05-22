#ifndef _DB_FACTORY_H
#define _DB_FACTORY_H

namespace   db_api {

class DBConnection;

class DBFactory
{
  public:
  enum DBType
  {
    UNDEFINED,
    POSTGRESQL,
    MYSQL,
    ORACLE
  };

  static DBConnection* CreateDBConnection(DBType type);
};

} // namespace db_api 
#endif  // _DB_FACTORY_H

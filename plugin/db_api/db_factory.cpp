#include "db_factory.h"
#include "db_pgclient.h"

namespace db_api {

DBConnection* DBFactory::CreateDBConnection(DBType type)
{
  switch (type) {
    case DBFactory::POSTGRESQL:
      return new PGClient();
    default:
      // Other databases are not supported yet
      return NULL;
  }
}

} // namespace db_api

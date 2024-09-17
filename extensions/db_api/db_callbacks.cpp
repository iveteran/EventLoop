#include "db_callbacks.h"
#include "core/logger.h"

using namespace evt_loop;

namespace db_api {

void DBCallbacks::EmptyReplyCb(DBConnection*, const DBResult*) {
    el_logger->debug("Empty DBConnection Result Callback\n");
}
void DBCallbacks::EmptyCmdSentCb(DBConnection*, const char* sql, const SQLParameter*) {
    el_logger->debug("Empty DBConnection Command Sent Callback\n");
}
void DBCallbacks::EmptyConnectedCb(DBConnection*) {
    el_logger->debug("Empty Connected Callback\n");
}
void DBCallbacks::EmptyClosedCb(DBConnection*) {
    el_logger->debug("Empty Connection Closed Callback\n");
}
void DBCallbacks::EmptyErrorCb(DBConnection*, int, const char*) {
    el_logger->debug("Empty Connection Error Callback\n");
}

}  // namespace db_api

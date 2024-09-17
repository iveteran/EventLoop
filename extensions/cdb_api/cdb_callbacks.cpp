#include "cdb_callbacks.h"
#include "core/logger.h"

using namespace evt_loop;

namespace cdb_api {

void CDBCallbacks::EmptyReplyCb(CDBClient*, const CDBReply*) {
    el_logger->debug("Empty CDBClient Reply Callback");
}
void CDBCallbacks::EmptyCmdSentCb(CDBClient*, const CDBCommand*) {
    el_logger->debug("Empty CDBClient Command Sent Callback");
}
void CDBCallbacks::EmptyConnectedCb(CDBClient*) {
    el_logger->debug("Empty Connected Callback");
}
void CDBCallbacks::EmptyClosedCb(CDBClient*) {
    el_logger->debug("Empty Connection Closed Callback");
}
void CDBCallbacks::EmptyErrorCb(CDBClient*, int, const char*) {
    el_logger->debug("Empty Connection Error Callback");
}

}  // namespace cdb_api

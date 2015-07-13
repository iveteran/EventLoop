#ifndef _TCP_CALLBACKS_H
#define _TCP_CALLBACKS_H

#include "callback.h"

namespace richinfo {

class BusinessTester;
class TcpConnection;

typedef Callback2<BusinessTester, TcpConnection*, const string* > OnMessageCallback;

}  // namespace richinfo

#endif  // _TCP_CALLBACKS_H

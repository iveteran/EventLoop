#include <stdio.h>
#include "tcp_client.h"

using namespace richinfo;

int main(int argc, char **argv) {
  TcpClient echoclient("localhost", 22222);
  //echoclient.SetOnMessageCb(OnMessageCallback(this, &BusinessTester::EchoClientMsgHandler));
  //echoclient.Connect();
  echoclient.Send("hello");

  //el.AddEvent(&echoclient);

  EV_Singleton->StartLoop();

  return 0;
}


#include <stdio.h>
#include "el.h"
#include "lua_eval.h"

using namespace evt_loop;
using namespace lua_eval;

int main(int argc, char **argv) {

  SignalHandler sh(SignalEvent::INT, [&](SignalHandler* sh, uint32_t signo) {
          printf("Shutdown\n");
          EV_Singleton->StopLoop();
          });

  LuaEval lua_eval_test("./lua_eval_test.lua", "enter");

  PeriodicTimer call_lua_timer(TimeVal(5, 0), [&](TimerEvent* timer) {
      string udata("china\0shenzhen", 14);
      string result;
      string errstr;
      int errcode = lua_eval_test.Enter(udata, result, errstr);
      printf("errcode: %d, errstr: %s, result data: %s\n", errcode, errstr.c_str(), result.c_str());
    });
  call_lua_timer.Start();

  EV_Singleton->StartLoop();

  return 0;
}


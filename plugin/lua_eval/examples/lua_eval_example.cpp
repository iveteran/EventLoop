#include <stdio.h>
#include <iostream>
#include "el.h"
#include "lua_eval.h"

using namespace evt_loop;
using namespace lua_eval;

void call_binary_example(LuaEval& lua_eval)
{
    string udata("china\0shenzhen", 14);
    string result;
    string errstr;
    int errcode = lua_eval.Invoke("example_binary", udata, result, errstr);
    printf("errcode: %d, errstr: %s, result data: %s\n", errcode, errstr.c_str(), result.c_str());
}

void call_array_example(LuaEval& lua_eval)
{
    std::vector<string> args;
    std::vector<string> rets;
    string errstr;
    args.push_back("foo");
    args.push_back("bar");
    int errcode = lua_eval.Invoke("example_array", args, rets, errstr);
    printf("errcode: %d, errstr: %s, result array size: %ld\n", errcode, errstr.c_str(), rets.size());
    cout << "[ ";
    for (const auto& value: rets)
        std::cout << value << ", ";
    std::cout << " ]" << endl;
}

void call_map_example(LuaEval& lua_eval)
{
    std::map<string, string> args;
    std::map<string, string> rets;
    string errstr;
    args["aa"] = "foo";
    args["bb"] = "bar";
    args["cc"] = "foobar";
    int errcode = lua_eval.Invoke("example_map", args, rets, errstr);
    printf("errcode: %d, errstr: %s, result map size: %ld\n", errcode, errstr.c_str(), rets.size());
    for (const auto& kv: rets)
        std::cout << kv.first << " => " << kv.second << endl;
}

int main(int argc, char **argv) {

  SignalHandler sh(SignalEvent::INT, [&](SignalHandler* sh, uint32_t signo) {
          printf("Shutdown\n");
          EV_Singleton->StopLoop();
          });

  LuaEval lua_eval_test("./lua_eval_test.lua");

  PeriodicTimer call_lua_timer(TimeVal(5, 0), [&](TimerEvent* timer) {
      cout << "--------------------" << endl;
      call_binary_example(lua_eval_test);
      cout << "--------------------" << endl;
      call_array_example(lua_eval_test);
      cout << "--------------------" << endl;
      call_map_example(lua_eval_test);
      cout << "--------------------" << endl;
    });

  call_lua_timer.Start();

  EV_Singleton->StartLoop();

  return 0;
}


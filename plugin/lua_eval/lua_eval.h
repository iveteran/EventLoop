#ifndef _LUA_EVAL_H
#define _LUA_EVAL_H

#include <string>
#include <vector>
#include <map>
#include <exception>
extern "C"  
{  
  #include <lua.h>  
  #include <lauxlib.h>  
  #include <lualib.h>  
}

using namespace std;

namespace lua_eval {

class LuaEvalException : public std::exception
{
  public:
  LuaEvalException(const char* errstr) : errstr_(errstr)
  {
  }

  const char* what() const noexcept
  {
    return errstr_.c_str();
  }

  private:
  string errstr_;
};

class LuaEval
{
  public:
    LuaEval(const char* lua_file);
    virtual ~LuaEval()
    {
      lua_close(lua_state_);
    }

    int Invoke(const char* func, const string& arg_udata, string& ret_udata, string& errstr);
    int Invoke(const char* func, const std::vector<string>& arg_tuple, std::vector<string>& ret_tuple, string& errstr);
    int Invoke(const char* func, const std::map<string, string>& args, std::map<string, string>& rets, string& errstr);

  private:
    string      lua_file_;
    lua_State*  lua_state_;
};

}  // namespace lua_eval

#endif // _LUA_EVAL_H

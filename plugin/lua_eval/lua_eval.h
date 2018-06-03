#ifndef _LUA_EVAL_H
#define _LUA_EVAL_H

#include <string>
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
    LuaEval(const char* lua_file, const char* enter_func);
    virtual ~LuaEval()
    {
      lua_close(lua_state_);
    }

    int Enter(const string& i_udata, string& o_udata, string& errstr);

  private:
    string      lua_file_;
    string      enter_func_;
    lua_State*  lua_state_;
};

}  // namespace lua_eval

#endif // _LUA_EVAL_H

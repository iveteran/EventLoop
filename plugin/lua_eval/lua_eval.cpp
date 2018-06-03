#include "lua_eval.h"

namespace lua_eval {

LuaEval::LuaEval(const char* lua_file, const char* enter_func) :
  lua_file_(lua_file), enter_func_(enter_func)
{
  lua_state_ = luaL_newstate();
  if (lua_state_ == NULL) {
    throw LuaEvalException("Create Lua state failed");
  }

  luaL_openlibs(lua_state_);

  int retval = luaL_dofile(lua_state_, lua_file);
  if (retval != 0) {
    throw LuaEvalException("Load Lua script failed");
  }
}

int LuaEval::Enter(const string& i_udata, string& o_udata, string& errstr)
{
  lua_getglobal(lua_state_, enter_func_.c_str());
  lua_pushlstring(lua_state_, i_udata.data(), i_udata.size());
  int retval = lua_pcall(lua_state_, 1, 1, 0);
  if (retval == 0) {
    bool is_string = lua_isstring(lua_state_, -1);
    if (is_string) {
      size_t len = 0;
      const char* result = lua_tolstring(lua_state_, -1, &len);
      printf(">>> result len: %ld\n", len);
      o_udata.assign(result, len);
    }
    return 0;
  } else {
    errstr = lua_tostring(lua_state_, -1);  
    return -1;
  }
}

}  // namespace lua_eval

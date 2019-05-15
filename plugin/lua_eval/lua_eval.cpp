#include "lua_eval.h"

namespace lua_eval {

LuaEval::LuaEval(const char* lua_file) :
  lua_file_(lua_file)
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

int LuaEval::Invoke(const char* func, const string& arg_udate, string& ret_udata, string& errstr)
{
  lua_getglobal(lua_state_, func);
  lua_pushlstring(lua_state_, arg_udate.data(), arg_udate.size());
  int retval = lua_pcall(lua_state_, 1, 1, 0);
  if (retval == 0) {
    bool is_string = lua_isstring(lua_state_, -1);
    if (is_string) {
      size_t len = 0;
      const char* result = lua_tolstring(lua_state_, -1, &len);
      //printf(">>> result len: %ld\n", len);
      ret_udata.assign(result, len);
    }
    lua_pop(lua_state_, 1);
    return 0;
  } else {
    errstr = lua_tostring(lua_state_, -1);  
    return -1;
  }
}

int LuaEval::Invoke(const char* func, const std::vector<string>& args, std::vector<string>& rets, string& errstr)
{
  lua_getglobal(lua_state_, func);
  lua_newtable(lua_state_);
  for (size_t idx = 0; idx < args.size(); idx++)
  {
    const int table_idx = idx + 1;
    string value = args[idx];
    lua_pushnumber(lua_state_, table_idx);
    lua_pushlstring(lua_state_, value.data(), value.size());
    lua_rawset(lua_state_, -3);
  }
  int retval = lua_pcall(lua_state_, 1, 1, 0);
  if (retval == 0) {
    bool is_table = lua_istable(lua_state_, -1);
    if (is_table) {
      lua_pushnil(lua_state_);
      while (lua_next(lua_state_, -2)) {
        size_t len = 0;
        const char* value = lua_tolstring(lua_state_, -1, &len);
        //printf("value : %s\n", value);
        rets.push_back(string(value, len));
        lua_pop(lua_state_, 1);
      }
    }
    lua_pop(lua_state_, 1);
    return 0;
  } else {
    errstr = lua_tostring(lua_state_, -1);
    return -1;
  }
}

int LuaEval::Invoke(const char* func, const std::map<string, string>& args, std::map<string, string>& rets, string& errstr)
{
  lua_getglobal(lua_state_, func);
  lua_newtable(lua_state_);
  for (auto iter = args.begin(); iter != args.end(); ++iter)
  {
    auto& key = iter->first;
    auto& value = iter->second;
    lua_pushstring(lua_state_, key.c_str());
    lua_pushlstring(lua_state_, value.data(), value.size());
    lua_rawset(lua_state_, -3);
  }
  int retval = lua_pcall(lua_state_, 1, 1, 0);
  if (retval == 0) {
    bool is_table = lua_istable(lua_state_, -1);
    if (is_table) {
      lua_pushnil(lua_state_);
      while (lua_next(lua_state_, -2)) {
        const char* key = lua_tostring(lua_state_, -2);
        size_t len = 0;
        const char* val = lua_tolstring(lua_state_, -1, &len);
        //printf("%s => %s\n", key, val);
        rets[key] = string(val, len);
        lua_pop(lua_state_, 1);
      }
    }
    lua_pop(lua_state_, 1);
    return 0;
  } else {
    errstr = lua_tostring(lua_state_, -1);
    return -1;
  }
}

}  // namespace lua_eval

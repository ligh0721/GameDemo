#ifndef __LUABINDINGFORCC_H__
#define __LUABINDINGFORCC_H__


class CUnitDrawForCC;

int luaModuleLoader4cc(lua_State *L);
bool luaL_loadscript4cc(lua_State *L, const char* name, string& err);

int unit4cc_ctor(lua_State* L);
int unit4cc_addBattleTip(lua_State* L);

CUnitDrawForCC* luaL_tospriteptr(lua_State* L, int idx = 1);
int sprite4cc_ctor(lua_State* L);
int sprite4cc_prepareFrame(lua_State* L);
int sprite4cc_prepareAnimation(lua_State* L);
int sprite4cc_setGeometry(lua_State* L);

int g_cclog(lua_State* L);
int g_loadTexture(lua_State* L);
int g_loadAnimation(lua_State* L);
int g_createUnit(lua_State* L);

int luaRegWorldFuncsForCC(lua_State* L, CWorld* pWorld);

int StatusShowPas_ctor(lua_State* L);


#endif  /* __LUABINDINGFORCC_H__ */



#include "pch.h"
#include "luaenv.h"
#include <boost/range/adaptor/reversed.hpp>

#include "scheduler.h"
#include "tasks.h"
#include "server.h"

extern Dispatcher g_dispatcher;
extern Scheduler g_scheduler;

extern std::condition_variable g_loaderSignal;

std::unordered_map<int64_t, DBResult_ptr> LuaBinder::results{};
int64_t LuaBinder::last_id = 1;
int64_t LuaBinder::last_result_id = 1;
String LuaBinder::lastError = String();

LuaBinder::LuaBinder()
{
	lua_state = luaL_newstate();
	luaL_openlibs(lua_state);
	register_funcs(lua_state);
}

void LuaBinder::reportError(const String& error)
{
	LuaBinder::setLastError(error);
	LuaBinder::reportLastError();
	g_loaderSignal.notify_all();
}

//get
String LuaBinder::getString(lua_State* L, int32_t arg)
{
	size_t len;
	const char* c_str = lua_tolstring(L, arg, &len);
	if (!c_str || len == 0) {
		return String();
	}
	return String(c_str, len);
}

// basic register // taken from forgotten server, since they are ready.
void LuaBinder::registerClass(lua_State* L, const std::string& name, const std::string& super, lua_CFunction __call/* = nullptr*/)
{
	// className = {}
	lua_newtable(L);
	lua_pushvalue(L, -1);
	lua_setglobal(L, name.c_str());
	int methods = lua_gettop(L);

	// methodsTable = {}
	lua_newtable(L);
	int methodsTable = lua_gettop(L);

	if (__call) {
		// className.__call = newFunction
		lua_pushcfunction(L, __call);
		lua_setfield(L, methodsTable, "__call");
	}

	uint32_t parents = 0;
	if (!super.empty()) {
		lua_getglobal(L, super.c_str());
		lua_rawgeti(L, -1, 'p');
		parents = LuaBinder::getNumber<uint32_t>(L, -1) + 1;
		lua_pop(L, 1);
		lua_setfield(L, methodsTable, "__index");
	}

	// setmetatable(className, methodsTable)
	lua_setmetatable(L, methods);

	// className.metatable = {}
	luaL_newmetatable(L, name.c_str());
	int metatable = lua_gettop(L);

	// className.metatable.__metatable = className
	lua_pushvalue(L, methods);
	lua_setfield(L, metatable, "__metatable");

	// className.metatable.__index = className
	lua_pushvalue(L, methods);
	lua_setfield(L, metatable, "__index");

	// className.metatable['h'] = hash
	lua_pushnumber(L, std::hash<std::string>()(name));
	lua_rawseti(L, metatable, 'h');

	// className.metatable['p'] = parents
	lua_pushnumber(L, parents);
	lua_rawseti(L, metatable, 'p');

	// className.metatable['t'] = type
	lua_pushnumber(L, ++LuaBinder::last_id);
	lua_rawseti(L, metatable, 't');

	// pop className, className.metatable
	lua_pop(L, 2);
}
void LuaBinder::registerTable(lua_State* L, const String& tableName)
{
	// _G[tableName] = {}
	lua_newtable(L);
	lua_setglobal(L, tableName.c_str());
}
void LuaBinder::registerMethod(lua_State* L, const String& globalName, const String& methodName, lua_CFunction func)
{
	// globalName.methodName = func
	lua_getglobal(L, globalName.c_str());
	lua_pushcfunction(L, func);
	lua_setfield(L, -2, methodName.c_str());

	// pop globalName
	lua_pop(L, 1);
}
void LuaBinder::registerMetaMethod(lua_State* L, const String& className, const String& methodName, lua_CFunction func)
{
	// className.metatable.methodName = func
	luaL_getmetatable(L, className.c_str());
	lua_pushcfunction(L, func);
	lua_setfield(L, -2, methodName.c_str());

	// pop className.metatable
	lua_pop(L, 1);
}
void LuaBinder::registerGlobalMethod(lua_State* L, const String& functionName, lua_CFunction func)
{
	// _G[functionName] = func
	lua_pushcfunction(L, func);
	lua_setglobal(L, functionName.c_str());
}
void LuaBinder::registerVariable(lua_State* L, const String& tableName, const String& name, const String& value)
{
	// tableName.name = value
	lua_getglobal(L, tableName.c_str());
	LuaBinder::setField(L, name.c_str(), value);

	// pop tableName
	lua_pop(L, 1);
}
String LuaBinder::popString(lua_State* L)
{
	if (lua_gettop(L) == 0) {
		return String();
	}

	String str(getString(L, -1));
	lua_pop(L, 1);
	return str;
}
bool LuaBinder::handleError(lua_State* L, int ret)
{
	if (ret != 0) {
		setLastError(LuaBinder::popString(L));
		return false;
	}
	return true;
}
bool LuaBinder::loadFile(const String& filename)
{
	return LuaBinder::handleError(lua_state, luaL_loadfile(lua_state, filename.c_str()));
}
bool LuaBinder::doFile(const String& filename)
{
	return LuaBinder::handleError(lua_state, luaL_dofile(lua_state, filename.c_str()));
}
bool LuaBinder::call()
{
	return LuaBinder::handleError(lua_state, lua_pcall(lua_state, 0, LUA_MULTRET, 0));
}

// Object
int LuaBinder::luaObjectConstruct(lua_State* L)
{
	lua_getfield(L, -2, "__name");
	String name = LuaBinder::getString(L, -1);
	lua_pop(L, 1);

	int args = lua_gettop(L);
	lua_getfield (L, -2, "init"); // -2 is the choice.
	lua_insert(L, 1);

	if (!LuaBinder::handleError(L, lua_pcall(L, args, 1, 0))) {
		LuaBinder::reportLastError();
	}

	LuaBinder::pushObject(L, new LuaObject());
	LuaBinder::setMetatable(L, -1, name);

	return 1;
}
int LuaBinder::luaObjectCompare(lua_State* L)
{ // obj1 = obj2
	LuaBinder::pushBoolean(L,
		LuaBinder::getObject(L, 1) == LuaBinder::getObject(L, 2));
	return 1;
}
int LuaBinder::luaObjectDelete(lua_State* L)
{
	LuaObject* obj = LuaBinder::getObject(L, 1);
	if (obj) {
		delete obj;
		obj = nullptr;
	}
	return 1;
}
int LuaBinder::luaObjectCreate(lua_State* L)
{
	// Object(name[, super = ""])
	String name = LuaBinder::getString(L, 1);
	String super = LuaBinder::getString(L, 2);

	LuaBinder::registerClass(L, name, super, &LuaBinder::luaObjectConstruct);
	LuaBinder::registerMetaMethod(L, name, "__eq", &LuaBinder::luaObjectCompare);
	LuaBinder::registerMetaMethod(L, name, "__gc", &LuaBinder::luaObjectDelete);
	LuaBinder::registerMethod(L, name, "delete", &LuaBinder::luaObjectDelete);

	// registering vars.
	LuaBinder::registerVariable(L, name, "__name", name);
	LuaBinder::registerVariable(L, name, "__super", super);

	// Object:init(...) -- construct.

	// Object:OnAdd(obj) objA(self) + objB(obj)
	// Object:OnSub(obj) objA(self) - objB(obj)

	return 1;
}

// Server
int LuaBinder::luaServerShutdown(lua_State* L)
{
	int error = LuaBinder::getNumber<int>(L, 1, EXIT_SUCCESS);
	Server::Shutdown(error);
	return 1;
}

// Database
int LuaBinder::luaDatabaseConnect(lua_State* L)
{
	// Database.connect(host, user, pass, db, port, socket)
	String host = LuaBinder::getString(L, 1);
	String user = LuaBinder::getString(L, 2);
	String pass = LuaBinder::getString(L, 3);
	String database = LuaBinder::getString(L, 4);
	int32_t port = LuaBinder::getNumber<int32_t>(L, 5);
	String socket = LuaBinder::getString(L, 6);

	Database* db = Database::getInstance();
	if (!db->connect(host, user, pass, database, port, socket)) {
		LuaBinder::reportError("Failed to connect to database.");
		LuaBinder::pushBoolean(L, false);
	} else {
		Database::isConnected = true;
		LuaBinder::pushBoolean(L, true);
	}
	return 1;
}
int LuaBinder::luaDatabaseStoreQuery(lua_State* L)
{
	// Database.storeQuery(query)
	if (!Database::isConnected) {
		lua_pushnil(L);
		return 1;
	}

	String query = LuaBinder::getString(L, 1);
	DBResult_ptr result = Database::getInstance()->storeQuery(query);
	if (!result) {
		lua_pushnil(L);
	} else {
		LuaBinder::pushDBResult(L, result);
	}

	return 1;
}
int LuaBinder::luaDatabaseExecuteQuery(lua_State* L)
{
	// Database.executeQuery(query)
	if (!Database::isConnected) {
		lua_pushnil(L);
		return 1;
	}

	String query = LuaBinder::getString(L, 1);
	LuaBinder::pushBoolean(L, Database::getInstance()->executeQuery(query));
	return 1;
}
int LuaBinder::luaDatabaseEscapeString(lua_State* L)
{
	// Database.escapeString(string)
	LuaBinder::pushString(L, Database::getInstance()->escapeString(getString(L, -1)));
	return 1;
}
int LuaBinder::luaDatabaseEscapeBlob(lua_State* L)
{
	// Database.escapeBlob(blob)
	uint32_t length = getNumber<uint32_t>(L, 2);
	pushString(L, Database::getInstance()->escapeBlob(getString(L, 1).c_str(), length));
	return 1;
}
int LuaBinder::luaDatabaseLastInsertId(lua_State* L)
{
	// Database.lastInsertId()
	lua_pushnumber(L, Database::getInstance()->getLastInsertId());
	return 1;
}
int LuaBinder::luaDatabaseConnected(lua_State* L)
{
	// Database.isConnected()
	LuaBinder::pushBoolean(L, Database::isConnected);
	return 1;
}
int LuaBinder::luaDatabaseTableExists(lua_State* L)
{
	// Database.tableExists(database, table)
	auto tableExists = [](String database, String tableName) {
		Database* db = Database::getInstance();

		std::ostringstream query;
		query << "SELECT `TABLE_NAME` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(database) << " AND `TABLE_NAME` = " << db->escapeString(tableName) << " LIMIT 1";
		return db->storeQuery(query.str()).get() != nullptr;
	};

	pushBoolean(L, tableExists(getString(L, 1), getString(L, 2)));
	return 1;
}

// DBResult
void LuaBinder::pushDBResult(lua_State* L, DBResult_ptr res)
{
	LuaResultObject* resObj = new LuaResultObject();
	resObj->id = ++last_result_id;
	results.insert(std::make_pair(resObj->id, res));

	LuaBinder::pushUserdata(L, resObj);
	LuaBinder::setMetatable(L, -1, "DBResult");
}
DBResult_ptr LuaBinder::getDBResult(lua_State* L)
{
	LuaResultObject* resObj = LuaBinder::getUserdata<LuaResultObject>(L, 1);
	if (!resObj) {
		return nullptr;
	}
	return results[resObj->id];
}
int LuaBinder::luaResultGetNumber(lua_State* L)
{
	DBResult_ptr res = LuaBinder::getDBResult(L);
	if (!res) {
		lua_pushnil(L);
		return 1;
	}

	String rowName = LuaBinder::getString(L, 2);
	LuaBinder::pushnumber(L, res->getNumber<int64_t>(rowName));
	return 1;
}
int LuaBinder::luaResultGetString(lua_State* L)
{
	DBResult_ptr res = LuaBinder::getDBResult(L);
	if (!res) {
		lua_pushnil(L);
		return 1;
	}

	String rowName = LuaBinder::getString(L, 2);
	LuaBinder::pushString(L, res->getString(rowName));
	return 1;
}
int LuaBinder::luaResultGetStream(lua_State* L)
{
	DBResult_ptr res = LuaBinder::getDBResult(L);
	if (!res) {
		pushBoolean(L, false);
		return 1;
	}

	unsigned long length;
	const char* stream = res->getStream(getString(L, 2), length);
	lua_pushlstring(L, stream, length);
	lua_pushnumber(L, length);
	return 2;
}
int LuaBinder::luaResultNext(lua_State* L)
{
	DBResult_ptr res = LuaBinder::getDBResult(L);
	if (!res) {
		pushBoolean(L, false);
		return 1;
	}

	pushBoolean(L, res->next());
	return 1;
}
int LuaBinder::luaResultFree(lua_State* L)
{
	LuaResultObject* resObj = LuaBinder::getUserdata<LuaResultObject>(L, 1);
	if (resObj) {
		results.erase(resObj->id);
	}

	return 1;
}

// Dispatcher
int LuaBinder::luaDispatcherStart(lua_State*)
{
	g_dispatcher.start();
	return 1;
}
int LuaBinder::luaDispatcherShutdown(lua_State*)
{
	g_dispatcher.shutdown();
	return 1;
}
int LuaBinder::luaDispatcherAddTask(lua_State* L)
{
	// Dispatcher.addTask(callable, ...)
	int parameters = lua_gettop(L);
	if (lua_isfunction(L, -parameters/*1*/)) {
		LuaEventObject* eventDesc = new LuaEventObject();
		for (int i = 0; i < parameters - 1; ++i) {
			eventDesc->parameters.push_back(luaL_ref(L, LUA_REGISTRYINDEX));
		}

		eventDesc->function = luaL_ref(L, LUA_REGISTRYINDEX);

		g_dispatcher.addTask(createTask(std::bind(&LuaBinder::executeLuaEvent, L, eventDesc)));
		LuaBinder::pushBoolean(L, true);
	} else {
		LuaBinder::reportError("[Error - Scheduler.addTask] Callable(#arg 1) must be function.");
		LuaBinder::pushBoolean(L, false);
	}

	return 1;
}

// Scheduler
int LuaBinder::luaSchedulerStart(lua_State*)
{
	g_scheduler.start();
	return 1;
}
int LuaBinder::luaSchedulerShutdown(lua_State*)
{
	g_scheduler.shutdown();
	return 1;
}
int LuaBinder::luaSchedulerAddEvent(lua_State* L)
{
	// Scheduler.addEvent(callable, delay, ...)
	int parameters = lua_gettop(L);
	if (lua_isfunction(L, -parameters/*1*/)) {
		LuaEventObject* eventDesc = new LuaEventObject();
		for (int i = 0; i < parameters - 2; ++i) {
			eventDesc->parameters.push_back(luaL_ref(L, LUA_REGISTRYINDEX));
		}

		uint32_t delay = std::max<uint32_t>(100, LuaBinder::getNumber<uint32_t>(L, 2));
		lua_pop(L, 1);

		eventDesc->function = luaL_ref(L, LUA_REGISTRYINDEX);
		LuaBinder::pushnumber(L, 
			g_scheduler.addEvent(createSchedulerTask(delay,
				std::bind(&LuaBinder::executeLuaEvent, L, eventDesc))));
	} else {
		LuaBinder::reportError("[Error - Scheduler.addTask] Callable(#arg 1) must be function.");
		LuaBinder::pushBoolean(L, false);
	}

	return 1;
}
int LuaBinder::luaSchedulerStopEvent(lua_State* L)
{
	// Scheduler.stopEvent(eventid)
	uint32_t eventId = LuaBinder::getNumber<uint32_t>(L, 1);
	g_scheduler.stopEvent(eventId);
	return 1;
}

// Executions
void LuaBinder::executeLuaEvent(lua_State* L, LuaEventObject* obj)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, obj->function);
	for (auto parameter : boost::adaptors::reverse(obj->parameters)) {
		lua_rawgeti(L, LUA_REGISTRYINDEX, parameter);
	}

	lua_insert(L, obj->function);
	lua_pcall(L, obj->parameters.size(), 1, 0);
}

// Registering All function into lua_state
void LuaBinder::register_funcs(lua_State* L)
{
	LuaBinder::registerGlobalMethod(L, "Object", &LuaBinder::luaObjectCreate);

	// Serer
	LuaBinder::registerTable(L, "Server");
	LuaBinder::registerMethod(L, "Server", "shutdown", &LuaBinder::luaServerShutdown);
	// Database
	LuaBinder::registerTable(L, "Database");
	LuaBinder::registerMethod(L, "Database", "connect", &LuaBinder::luaDatabaseConnect);
	LuaBinder::registerMethod(L, "Database", "storeQuery", &LuaBinder::luaDatabaseStoreQuery);
	LuaBinder::registerMethod(L, "Database", "executeQuery", &LuaBinder::luaDatabaseExecuteQuery);
	LuaBinder::registerMethod(L, "Database", "escapeString", &LuaBinder::luaDatabaseEscapeString);
	LuaBinder::registerMethod(L, "Database", "escapeBlob", &LuaBinder::luaDatabaseEscapeBlob);
	LuaBinder::registerMethod(L, "Database", "lastInsertId", &LuaBinder::luaDatabaseLastInsertId);
	LuaBinder::registerMethod(L, "Database", "isConnected", &LuaBinder::luaDatabaseConnected);
	LuaBinder::registerMethod(L, "Database", "tableExists", &LuaBinder::luaDatabaseTableExists);

	// Database Result
	LuaBinder::registerClass(L, "DBResult", "");
	LuaBinder::registerMethod(L, "DBResult", "getNumber", &LuaBinder::luaResultGetNumber);
	LuaBinder::registerMethod(L, "DBResult", "getString", &LuaBinder::luaResultGetString);
	LuaBinder::registerMethod(L, "DBResult", "getStream", &LuaBinder::luaResultGetStream);
	LuaBinder::registerMethod(L, "DBResult", "next", &LuaBinder::luaResultNext);
	LuaBinder::registerMethod(L, "DBResult", "free", &LuaBinder::luaResultFree);

	// Dispatcher
	LuaBinder::registerTable(L, "Dispatcher");
	LuaBinder::registerMethod(L, "Dispatcher", "start", &LuaBinder::luaDispatcherStart);
	LuaBinder::registerMethod(L, "Dispatcher", "shutdown", &LuaBinder::luaDispatcherShutdown);
	LuaBinder::registerMethod(L, "Dispatcher", "addTask", &LuaBinder::luaDispatcherAddTask);

	LuaBinder::registerTable(L, "Scheduler");
	LuaBinder::registerMethod(L, "Scheduler", "start", &LuaBinder::luaSchedulerStart);
	LuaBinder::registerMethod(L, "Scheduler", "shutdown", &LuaBinder::luaSchedulerShutdown);
	LuaBinder::registerMethod(L, "Scheduler", "addEvent", &LuaBinder::luaSchedulerAddEvent);
	LuaBinder::registerMethod(L, "Scheduler", "stopEvent", &LuaBinder::luaSchedulerStopEvent);
}

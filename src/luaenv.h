

#ifndef LUA_ENVIRONMENT
#define LUA_ENVIRONMENT

#include "database.h"

class LuaObject {};

class LuaResultObject : LuaObject {
	public:
		int64_t id;
};

class LuaEventObject : LuaObject, boost::noncopyable {
	public:
		int32_t function;
		std::list<int32_t> parameters;

		explicit LuaEventObject() :
			function(-1), parameters() {}
		~LuaEventObject() {}
};

class LuaBinder : private boost::noncopyable /* non-copyable */
{
	public:
		// constructor
		explicit LuaBinder();

		static void reportError(const String& error);

		static String getLastError() {
			return LuaBinder::lastError;
		}
		static void setLastError(String err) {
			LuaBinder::lastError = err;
		}
		static void reportLastError() {
			String err = getLastError();
			if (!err.empty()) {
				std::cout << getLastError() << std::endl;
				setLastError(String());
			}
		}

		// Userdata
		template<class T>
		static void pushUserdata(lua_State* L, T* value)
		{
			T** userdata = static_cast<T**>(lua_newuserdata(L, sizeof(T*)));
			*userdata = value;
		}

		// push
		static void pushObject(lua_State* L, LuaObject* _obj) {
			pushUserdata(L, _obj);
		}
		static void pushDBResult(lua_State* L, DBResult_ptr res);
		static void pushnumber(lua_State* L, LUA_NUMBER num) {
			lua_pushnumber(L, num);
		}
		static void pushString(lua_State* L, const String& value) {
			lua_pushlstring(L, value.c_str(), value.length());
		}
		static void pushBoolean(lua_State* L, bool value) {
			lua_pushboolean(L, value ? 1 : 0);
		}

		// set
		static void setMetatable(lua_State* L, int32_t index, String name) {
			luaL_getmetatable(L, name.c_str());
			lua_setmetatable(L, index - 1);
		}
		static int newMetatable(lua_State* L, String name) {
			return luaL_newmetatable(L, name.c_str());
		}
		static void register_funcs(lua_State* L);
		static void executeLuaEvent(lua_State* L, LuaEventObject* obj);
		static String popString(lua_State* L);
		static void setField(lua_State* L, String index, lua_Number value) {
			lua_pushnumber(L, value);
			lua_setfield(L, -2, index.c_str());
		}
		static void setField(lua_State* L, String index, const String& value) {
			pushString(L, value);
			lua_setfield(L, -2, index.c_str());
		}

		// get
		template<typename T>
		inline static typename std::enable_if<std::is_enum<T>::value, T>::type
			getNumber(lua_State* L, int32_t arg)
		{
			return static_cast<T>(static_cast<int64_t>(lua_tonumber(L, arg)));
		}
		template<typename T>
		inline static typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, T>::type
			getNumber(lua_State* L, int32_t arg)
		{
			return static_cast<T>(lua_tonumber(L, arg));
		}
		template<typename T>
		static T getNumber(lua_State *L, int32_t arg, T defaultValue)
		{
			const auto parameters = lua_gettop(L);
			if (parameters == 0 || arg > parameters) {
				return defaultValue;
			}
			return getNumber<T>(L, arg);
		}
		static String getString(lua_State* L, int32_t arg);
		static DBResult_ptr getDBResult(lua_State* L);

		bool loadFile(const String& filename);
		bool doFile(const String& filename);
		bool call();

		// userdata
		template<class T>
		static T* getUserdata(lua_State* L, int32_t arg)
		{
			T** userdata = getRawUserdata<T>(L, arg);
			if (!userdata) {
				return nullptr;
			}
			return *userdata;
		}
		template<class T>
		inline static T** getRawUserdata(lua_State* L, int32_t arg)
		{
			return static_cast<T**>(lua_touserdata(L, arg));
		}

		static LuaObject* getObject(lua_State* L, int32_t arg) {
			return getUserdata<LuaObject>(L, arg);
		}

		static bool getBoolean(lua_State* L, int32_t arg) {
			return lua_toboolean(L, arg) != 0;
		}
		static bool getBoolean(lua_State* L, int32_t arg, bool defaultValue) {
			const auto parameters = lua_gettop(L);
			if (parameters == 0 || arg > parameters) {
				return defaultValue;
			}
			return lua_toboolean(L, arg) != 0;
		}

		// functions
		static int luaObjectCreate(lua_State* L);
		static int luaObjectDelete(lua_State* L);
		static int luaObjectCompare(lua_State* L);
		static int luaObjectConstruct(lua_State* L);

		static int luaServerShutdown(lua_State* L);

		static int luaDatabaseConnect(lua_State* L);
		static int luaDatabaseStoreQuery(lua_State* L);
		static int luaDatabaseExecuteQuery(lua_State* L);
		static int luaDatabaseEscapeString(lua_State* L);
		static int luaDatabaseEscapeBlob(lua_State* L);
		static int luaDatabaseLastInsertId(lua_State* L);
		static int luaDatabaseConnected(lua_State* L);
		static int luaDatabaseTableExists(lua_State* L);

		static int luaResultGetNumber(lua_State* L);
		static int luaResultGetString(lua_State* L);
		static int luaResultGetStream(lua_State* L);
		static int luaResultNext(lua_State* L);
		static int luaResultFree(lua_State* L);

		static int luaDispatcherStart(lua_State*);
		static int luaDispatcherShutdown(lua_State*);
		static int luaDispatcherAddTask(lua_State* L);

		static int luaSchedulerStart(lua_State*);
		static int luaSchedulerShutdown(lua_State*);
		static int luaSchedulerAddEvent(lua_State* L);
		static int luaSchedulerStopEvent(lua_State* L);

	private:
		static void registerClass(lua_State* L, const std::string& className, const std::string& baseClass, lua_CFunction newFunction = nullptr);
		static void registerTable(lua_State* L, const String& tableName);
		static void registerMethod(lua_State* L, const String& className, const String& methodName, lua_CFunction func);
		static void registerMetaMethod(lua_State* L, const String& className, const String& methodName, lua_CFunction func);
		static void registerGlobalMethod(lua_State* L, const String& functionName, lua_CFunction func);
		static void registerVariable(lua_State* L, const String& tableName, const String& name, const String& value);

		static lua_CFunction dummy() {
			return lua_CFunction();
		}
		static bool handleError(lua_State* L, int ret);

		// Variables.
		static std::unordered_map<int64_t, DBResult_ptr> results;
		static int64_t last_result_id;
		static int64_t last_id;
		static String lastError;
		lua_State* lua_state; // lua_env_state
};

#endif
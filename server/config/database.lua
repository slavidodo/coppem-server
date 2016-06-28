Database.Information = {
	host = "127.0.0.1",
	user = "",
	password = "",
	database = "",
	port = 3306,
	socket = ""
}

if not Database.connect(Database.Information.host,
			Database.Information.user,
			Database.Information.password,
			Database.Information.database,
			Database.Information.port,
			Database.Information.socket
		) then
	--Server.shutdown()
end

db = Database

-- Database
---- Methods :
	-- Database.connect(host, user, password, database, port, socket) (bool [success])
	-- Database.storeQuery(query) (result [userdata])
	-- Database.executeQuery(query) (bool)
	-- Database.escapeString(str) (string [escaped string])
	-- Database.escapeBlob(blob) (bin [ready])
	-- Database.lastInsertId() (int [id])
	-- Database.isConnected() (bool [connected])
	-- Database.tableExists(table) (bool [exists])
	
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
	Server.shutdown(1)
end

db = Database
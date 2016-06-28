-- Database.
print("> Establishing Connection to database.")
dofile("config/database.lua")
print(">> Established connection successfully.")

-- Sample Database.
local result = db.storeQuery("SELECT * FROM `players` WHERE 1")
repeat
	print(result:getString("name"))
until not result:next()
result:free()

module:=dbserver

dbserver_TARGETS := bin/arb_db_server
dbserver_LIBADD := servercntrl db

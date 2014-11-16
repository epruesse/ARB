module:=dbserver

dbserver_TARGETS := bin/arb_db_server
dbserver_LIBADD := servercntrl aisc_com db 

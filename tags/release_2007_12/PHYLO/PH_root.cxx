#include "phylo.hxx"


extern AW_window *preset_window( AW_root *root );

char *AP_root::open(const char *db_server)
	{
	gb_main = GB_open(db_server,"rwt");
	if (!gb_main) return (char *)GB_get_error();
	::gb_main = this->gb_main;
	return 0;
}

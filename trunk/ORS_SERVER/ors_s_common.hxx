#define ROOT_USERPATH "/"
#define ORS_LIB_PATH "ors_lib/"

// Hash table of logged in users; hash key is dailypw
struct passwd_struct {
	char	*userpath;
	char	*remote_host;
	long	login_time;
	long	last_access_time;

};

// Hash table of buffered query requests; query# is key
struct query_struct {
	char	*dailypw;  // find query via # and check dailypw
	char	*query;
};

#ifndef GLOBAL_H
#include "aisc_global.h"
#endif

typedef struct struct_aisc_com {
	int		socket;
	int		message_type;
	char		*message;
	int		*message_queue;
	long		magic;
	const char	*error;
} aisc_com;

#define AISC_MAX_ATTR 4095
#define MAX_AISC_SET_GET 16
#define AISC_MAX_STRING_LEN 1024
#define AISC_MESSAGE_BUFFER_LEN ((AISC_MAX_STRING_LEN/4+3)*(16+2))

typedef struct struct_bytestring {
	char	*data;
	int	size;
} bytestring;

#define AISC_MAGIC_NUMBER 0

enum aisc_command_list {
	AISC_GET	= AISC_MAGIC_NUMBER	+ 0,
	AISC_SET	= AISC_MAGIC_NUMBER	+ 1,
	AISC_NSET	= AISC_MAGIC_NUMBER	+ 2,
	AISC_CREATE	= AISC_MAGIC_NUMBER	+ 3,
	AISC_FIND	= AISC_MAGIC_NUMBER	+ 4,
	AISC_COPY	= AISC_MAGIC_NUMBER	+ 5,
	AISC_DELETE	= AISC_MAGIC_NUMBER	+ 6,
	AISC_INIT	= AISC_MAGIC_NUMBER	+ 7,
	AISC_DEBUG_INFO	= AISC_MAGIC_NUMBER	+ 8,
	AISC_FORK_SERVER= AISC_MAGIC_NUMBER	+ 9
	};

enum aisc_client_command_list {
	AISC_CCOM_OK	= AISC_MAGIC_NUMBER	+ 0,
	AISC_CCOM_ERROR	= AISC_MAGIC_NUMBER	+ 1,
	AISC_CCOM_MESSAGE= AISC_MAGIC_NUMBER	+ 2
	};

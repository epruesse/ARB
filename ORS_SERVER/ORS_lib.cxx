#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>
#include <arbdb.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <crypt.h>
#include <ors_client.h>
#include <client.h>
#include <servercntrl.h>
#include <ors_lib.h>


/*****************************************************************************
  CRYPT A PASSWORD
							return strdup
*****************************************************************************/
char *ORS_crypt(char *password) {
	static char *crypted_pw1 = 0;
	delete crypted_pw1;
	static char *crypted_pw2 = 0;
	delete crypted_pw2;

	char *use_pw = password;

	static char *salt="XX";

	if (!password) use_pw="";

	sprintf(salt,"%2i",strlen(use_pw));

	char *temp = crypt(use_pw, salt);	// crypt first part
	crypted_pw1 = strdup(temp + 2);

	if (strlen(password) <= 8) 	use_pw="";
	else				use_pw = password + 8;

	temp = crypt(use_pw, salt);		// crypt second part
	crypted_pw2 = strdup(temp + 2);

	char *result=(char *)calloc(sizeof(char),strlen(crypted_pw1) + strlen(crypted_pw2) + 1);
	strcpy(result, crypted_pw1);
	strcpy(result + 8, crypted_pw2);
	return result;
}

/*****************************************************************************
  Contruct Error Message
*****************************************************************************/
char *ORS_export_error(char *templat, ...)
	{
        /* goes to header: __attribute__((format(printf, 1, 2))) */
	char buffer[10000];
	char *p = buffer;
	va_list	parg;
	// sprintf (buffer,"ORS ERROR: ");
	p += strlen(p);
	va_start(parg,templat);

	vsprintf(p,templat,parg);

	return strdup(buffer);
}

/*****************************************************************************
  Contruct String
*****************************************************************************/
char *ORS_sprintf(char *templat, ...)
	{
        /* goes to header: __attribute__((format(printf, 1, 2))) */
	char buffer[10000];
	char *p = buffer;
	va_list	parg;
	p += strlen(p);
	va_start(parg,templat);

	vsprintf(p,templat,parg);

	return strdup(buffer);
}


/*******************************************************************************
	Time & Date Strings
				type determines output format
				time = desired time/date value or 0
********************************************************************************/
char *ORS_time_and_date_string(int type, long time) {
	long current_time;

	if (!time) current_time = GB_time_of_day();
	else current_time = time;
	struct tm *tms = localtime(&current_time);
	static char buffer[256];

	switch(type) {
		case TIME_HM:	strftime( buffer, 255, "%H:%M", tms); break;
		case DATE_DMY:	strftime( buffer, 255, "%d.%m.%Y", tms); break;
		case DATE_TIME:	strftime( buffer, 255, "%d.%m.%Y - %H:%M", tms); break;
		default: buffer[0] = 0;
	}
	return buffer;
}

/*************************************************************
  return number of occurences of a seek character within str
**************************************************************/
int ORS_str_char_count(char *str, char seek) {
	int count;
	char *pos;

	if (!str) return 0;

	for (count = 0, pos = strchr(str,seek); pos; pos=strchr(pos+1,seek)) count++;
	return count;
}

/***********************************************************
  string case compare of str1 and str2 with length of str2
************************************************************/
int ORS_strncmp(char *str1, char *str2)
{
	if (!str2) return 1;
	return strncmp(str1,str2,strlen(str2));
}

/******************************************************
  compare str2 with the tail of str1 (length of str2)
*******************************************************/
int ORS_strncase_tail_cmp(char *str1, char *str2) {

	if (!str1 || !str2) return 1;

	return strncasecmp(str1 + strlen(str1) - strlen(str2), str2, strlen(str2));
}

/******************************************************
  trim: removes leading & trailing spaces and tabs
*******************************************************/
char *ORS_trim(char *str) {
	if (!str) return 0;
	while (*str) {
		if (*str == '\t' || *str == ' ') str++;
		else break;
	}
	if (!*str) return str;
	char *end=str + strlen(str) - 1;
	while (end > str) {
		if (*end == '\t' || *end == ' ' || *end == '\n') {
			*end=0;
			end--;
		}
		else break;
	}
	return str;
}

/******************************************************
  contains: contains a token (with seperator)
				returns true or false
*******************************************************/
int ORS_contains_word(char *buffer, char *word, char seperator) {
	char 	*pos = buffer;
	int	len;

	if (!word || !buffer) return 0;

	len = strlen(word);
	while (pos) {
		pos = strstr(pos, word);
		if (!pos) break;
		if ( (pos == buffer || *(pos-1) == seperator) && (*(pos+len) == seperator || *(pos+len) == 0 ) )
			return 1;  // found it!
		pos++;
	}
	return 0;
}

/******************************************************
  SEQ MATCHES TARGET: validate normalized sequences
				returns true or false
*******************************************************/
int ORS_seq_matches_target_seq(char *seq, char *target, int complemented) {
	char *spos = seq;
	char *tpos = target;

	if (!seq || !target) return 0;
	if (!*seq || !*target) return 0;
	if (strlen(seq) != strlen(target)) return 0;

	if (complemented) tpos = target + strlen(seq) - 1;

	while (*spos && *tpos) {
		switch(*spos) {
			case 'A': if (*tpos == 'T' || *tpos == 'U'); break;
			case 'C': if (*tpos == 'G'); break;
			case 'G': if (*tpos == 'C'); break;
			case 'T': if (*tpos == 'A'); break;
			case 'U': if (*tpos == 'A'); break;

			case 'M': if (*tpos == 'K'); break;
			case 'K': if (*tpos == 'M'); break;
			case 'R': if (*tpos == 'Y'); break;
			case 'Y': if (*tpos == 'R'); break;
			case 'S': if (*tpos == 'S'); break;
			case 'V': if (*tpos == 'B'); break;
			case 'B': if (*tpos == 'V'); break;
			case 'D': if (*tpos == 'H'); break;
			case 'H': if (*tpos == 'D'); break;
			case 'X': if (*tpos == 'X'); break;
			case 'N': if (*tpos == 'X'); break;
			case '.': if (*tpos == '.'); break;
			default: return 0;
		}
		spos++;
		if (complemented) tpos--;
			     else tpos++;
	}
	return 1;
}

/******************************************************
  CALC 4GC 2AT
				returns strdup()
*******************************************************/
char * ORS_calculate_4gc_2at(char *sequence) {
	static char buffer[10];

	int gc=0, at=0;

	if (!sequence || !*sequence) return strdup("");
	char *pos = sequence;
	while (*pos) {
		switch(*pos) {
			case 'G': gc++; break;
			case 'C': gc++; break;
			case 'A': at++; break;
			case 'T': at++; break;
			case 'U': at++; break;
			default: break;
		}
		pos++;

	}
	sprintf(buffer, "%i C", 4*gc + 2*at);
	return strdup(buffer);
}

/******************************************************
  CALC GC RATIO
				returns strdup()
*******************************************************/
char * ORS_calculate_gc_ratio(char *sequence) {
	static char buffer[10];

	double min=0, max=0;

	if (!sequence || !*sequence) return strdup("");

	// calculate minimum
	char *pos = sequence;
	while (*pos) {
		switch(*pos) {
			case 'G':
			case 'C':
			case 'S': min++; break;
			default: break;
		}
		pos++;

	}

	// calculate maximum
	pos = sequence;
	while (*pos) {
		switch(*pos) {
			case 'G':
			case 'C':
			case 'M':
			case 'R':
			case 'S':
			case 'Y':
			case 'K':
			case 'V':
			case 'H':
			case 'D':
			case 'B':
			case 'X':
			case 'N': max++; break;
			default: break;
		}
		pos++;

	}
	if (min == max)	sprintf(buffer, "%1g %%", min / (double) strlen(sequence) * 100);
	else		sprintf(buffer, "%4.1f %% - %4.1f %%", min / (double) strlen(sequence) * 100, max / (double) strlen(sequence) * 100);
	return strdup(buffer);
}

/******************************************************
  STRCMP: can compare NULL-pointers!
	NULL == NULL
	NULL == ""
				returns true or false
*******************************************************/
extern "C" int ORS_strcmp(char *str1, char *str2) {
	if (!str1 && !str2) return 0;
	if ((str1 && *str1 && !str2) || (!str1 && str2 && *str2)) return 1;
	if ((!str1 && !*str2) || (!*str1 && !str2) || (!*str1 && !*str2)) return 0;
	return strcmp(str1, str2);
}

/*****************************************************************************
  FIND NEXT char IN A LIST
				must be initialized with start=buffer, end=0
		returns 1 when found, otherwise 0 and changes start, end
*****************************************************************************/
int OC_find_next_chr(char **start, char **end, char chr) {
	//int first_time=0;

	if (*start != *end && *end != 0) {
		if (**end == 0) *start = *end;
		else		*start = *end + 1;
	} else if (*start == *end) {
		(*start)++;	// last field was empty
	}

	if (!start || !*start) return 0;
	*end = strchr(*start,chr);
	if (*start == *end && **end != 0) {	// empty field found
		return 1;
	}
	if (!*end) {
		*end = strchr(*start,0);
		if (*start == *end) return 0;	// last time was last field
		// *start = *end;
		return 1;	// last field
	}
	return 1;	// first or middle field
}

/*******************************************************
  IS PARENT OF: check whether user1 is father of user2
			return 1 if true, 0 else
********************************************************/
int ORS_is_parent_of(char *user1, char *user2) {

	if (!user1 || !*user1 || !user2 || !*user2) return 0;

	if (!strcmp(user1,ROOT_USERPATH)) return 1; // root ist everybody's father
	int len1=strlen(user1);
	if ( !strncmp(user1,user2,len1) && user2[len1] == '/') return 1;	// user1 is begin of user2 and user2 continues -> ok
	return 0;
}

/*******************************************************
  IS PARENT OR EQUAL: check whether user1 is father of user2; user1 = user2 is true
			return 1 if true, 0 else
********************************************************/
int ORS_is_parent_or_equal(char *user1, char *user2) {

	if (!user1 || !user2) return 0;
	if (!strcmp(user1, user2)) return 1;
	return ORS_is_parent_of(user1, user2);
}

/*******************************************************
  GET PARENT OF: calculate parent of userpath
				return strdup or NULL
********************************************************/
char * ORS_get_parent_of(char *userpath) {

	if (!strcmp(userpath,"/")) return 0;		// root has no father
	char *pos=strrchr(userpath,'/');
	if (!pos) return 0;				// no slash means: no father
	if (pos == userpath) return strdup("/");	// only one slash: root is father
	*pos=0;
	static char *u=0;
	delete u;
	u=strdup(userpath);
	*pos='/';
	return u;
}

/*******************************************************
  STR 2 UPPER: converse string to upper case
				return strdup or NULL
********************************************************/
char * ORS_str_to_upper(char *str1) {
	if (!str1 || !*str1) return strdup("");

	char *str2 = (char *)calloc(sizeof(char), strlen(str1) + 1);
	char *pos=str2;
	while (*pos) {
		*pos=toupper(*pos);
	}
	return str2;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include "convert.h"
#include "global.h"

#define SIZE	128
static const char	*mon[12] = {"JAN", "FEB", "MAR", "APR", "MAY",
		"JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

/* --------------------------------------------------------------------
*	Function genbank_date().
*		Convert the date to be in genbank date form.
*/
char
*genbank_date(other_date)
char	*other_date;
{
	char	*gdate, /*token[SIZE],*/ temp[SIZE];
	int	day, month, year;
	int	length /*, index=0*/;

	length = Lenstr(other_date);
	if(other_date[length-1]=='\n')	{
		length--;
		other_date[length]='\0';
	}

	if(length<=10&&length>=6)	{
		find_date(other_date, &month, &day, &year);
	} else	if(length>10)	{
		if(is_genbank_date(other_date))	{
			if(Lenstr(other_date)>11)
				other_date[11]='\0';
			gdate = Dupstr(other_date);
			return(gdate);
		} else find_date_long_form(other_date, &month, &day, &year);
	} else {
		sprintf(temp, "Unknown date format: %s, cannot convert.\n", 				other_date);
		warning(146, temp);
		return(Dupstr("\?\?-\?\?\?-\?\?\?\?"));
	}
	if(day<=0||month<=0||year<=0||day>31||month>12)	{
		sprintf(temp, "Wrong date format: %s\n", other_date);
		warning(147, temp);
		return(Dupstr("\?\?-\?\?\?-\?\?\?\?"));
	}
	if(day<10)
		sprintf(temp, "0%d-%s-19%d", day, mon[month-1], year);
	else sprintf(temp, "%d-%s-19%d", day, mon[month-1], year);
	gdate = (char*)Dupstr(temp);
	return(gdate);

}
/* -----------------------------------------------------------
*	Function find_date().
*		Find day, month, year from date string.
*/
void
find_date(date_string, month, day, year)
char	*date_string;
int	*month, *day, *year;
{
/* 	int	two_char(), Lenstr(); */
	int	index, indi, count, nums[3];
	char	token[20], determ;

	index = count = 0;
	nums[0]=nums[1]=nums[2]=0;
	determ=' ';
	if(two_char(date_string, '.'))	determ='.';
	else if(two_char(date_string, '/')) determ='/';
	else if(two_char(date_string, '-')) determ='-';
	if(determ!=' ')	{
		for(indi=0; indi<=Lenstr(date_string); indi++)	{
			if(date_string[indi]==determ
			||indi==Lenstr(date_string))	{
				token[index++]='\0';
				nums[count++] = atoi(token);
				index = 0;
			} else token[index++]=date_string[indi];
		}
		*month=nums[0];
		*day=nums[1];
		*year=(nums[2] % 100);
		return;
	}
}
/* ------------------------------------------------------------------
*	Function two_char().
*		Return true if string has two determinator char.
*/
int
two_char(string, determ)
char	*string, determ;
{
	int	len, indi;

	int	count = 0;

	for(indi=0, len=Lenstr(string); indi<len; indi++)
		if(string[indi]==determ)	count++;

	if(count==2)	return(1);
	else	return(0);
}
/* -----------------------------------------------------------------
*	Function find_date_long_term().
*		Find day, month, year in the long term date string
*			like day-of-week, month, day, time, year.
*/
void
find_date_long_form(date_string, month, day, year)
char	*date_string;
int	*month, *day, *year;
{
	int	indi, index, length, /*count,*/ nums[3], num;
	char	token[SIZE];

	nums[0]=nums[1]=nums[2]=0;
	length = Lenstr(date_string);
	for(indi=0, index=0; index<=length; index++)	{
		if(index==length||date_string[index]==' '
		||date_string[index]=='\0'||date_string[index]=='\n'
		||date_string[index]=='\t'||date_string[index]=='('
		||date_string[index]==')'||date_string[index]==',')	{
			if(indi==0) continue;	/* empty token */
			token[indi] = '\0';
			if((num=ismonth(token))>0&&nums[0]==0)
				nums[0] = num;
			else if((num=isdatenum(token))>0)	{
				if(nums[0]==0&&num<12)	nums[0]=num;
				else if(nums[1]==0&&num<31)	nums[1]=num;
				else if(nums[2]==0)	nums[2]=(num % 100);
			}
			indi=0;
		} else token[indi++]=date_string[index];
	}	/* for loop */
	*month=nums[0];
	*day=nums[1];
	*year=nums[2];
	return;
}
/* --------------------------------------------------------------------
*	Function ismonth().
*		Return true if the char string is one of 12 months.
*			Case insensitive.
*/
int
ismonth(string)
char	*string;
{
	int	num=0;

	if(Cmpcasestr(string, "JAN")==0)
		num=1;
	else	if(Cmpcasestr(string, "FEB")==0)
		num=2;
	else	if(Cmpcasestr(string, "MAR")==0)
		num=3;
	else	if(Cmpcasestr(string, "APR")==0)
		num=4;
	else	if(Cmpcasestr(string, "MAY")==0)
		num=5;
	else	if(Cmpcasestr(string, "JUN")==0)
		num=6;
	else	if(Cmpcasestr(string, "JUL")==0)
		num=7;
	else	if(Cmpcasestr(string, "AUG")==0)
		num=8;
	else	if(Cmpcasestr(string, "SEP")==0)
		num=9;
	else	if(Cmpcasestr(string, "OCT")==0)
		num=10;
	else	if(Cmpcasestr(string, "NOV")==0)
		num=11;
	else	if(Cmpcasestr(string, "DEC")==0)
		num=12;
	return(num);
}
/* ------------------------------------------------------------------
*	Function isdatenum().
*		Return number of day or year the string represents.
*			If not day or year, return 0.
*/
int
isdatenum(string)
char	*string;
{
	int	length, num, indi;

	length                              = Lenstr(string);
	if(length>4||length<1) return(0);
	for(indi=0, num=1; indi<length&&num==1; indi++) {
        if(!isdigit(string[indi]))  {
            num = 0;
        }
    }
	if(num == 1)	num = atoi(string);
	return(num);
}
/* ------------------------------------------------------------------
*	Function is_genbank_date().
*		Return true if it is genbank form of date, which is
*			day(2 digits)-MONTH(in letters)-year(4 digits).
*/
int
is_genbank_date(string)
char	*string;
{
/* 	int	Lenstr(); */

	if(Lenstr(string)>=11&&string[2]=='-'&&string[6]=='-') return(1);
	else return(0);
}
/* ----------------------------------------------------------------
*	Function today_date().
*		Get today's date.
*/
char
*today_date()	{
/* 	struct tm	*local_tm, *localtime(); */
	struct timeval	tp;
	struct timezone	tzp;
	char	line[SIZE], *return_value;

	(void)gettimeofday(&tp, &tzp);

	Cpystr(line, ctime(&(tp.tv_sec)));

	return_value = Dupstr(line);

	return(return_value);
}
/* ---------------------------------------------------------------
*	Function gcg_date().
*		Create gcg format of date.
*/
char
*gcg_date(date_string)
char	*date_string;
{
	static char	date[128], temp[128], time[128];
	int	day, year;
/* 	char *	Catstr(); */

	temp[0]='\0';
	date_string[7]='\0';
	if(Cmpstr("Jan", date_string+4)==1)
		Catstr(temp, "January");
	else if(Cmpstr("Feb", date_string+4)==1)
		Catstr(temp, "Feburary");
	else if(Cmpstr("Mar", date_string+4)==1)
		Catstr(temp, "March");
	else if(Cmpstr("Apr", date_string+4)==1)
		Catstr(temp, "April");
	else if(Cmpstr("May", date_string+4)==1)
		Catstr(temp, "May");
	else if(Cmpstr("Jun", date_string+4)==1)
		Catstr(temp, "June");
	else if(Cmpstr("Jul", date_string+4)==1)
		Catstr(temp, "July");
	else if(Cmpstr("Aug", date_string+4)==1)
		Catstr(temp, "August");
	else if(Cmpstr("Sep", date_string+4)==1)
		Catstr(temp, "September");
	else if(Cmpstr("Oct", date_string+4)==1)
		Catstr(temp, "October");
	else if(Cmpstr("Nov", date_string+4)==1)
		Catstr(temp, "Novemember");
	else if(Cmpstr("Dec", date_string+4)==1)
		Catstr(temp, "December");

	sscanf(date_string+8, "%d %s %d", &day, time, &year);
	sprintf(date, "%s %d, %d  %s", temp, day, year, time);

	return(date);
}

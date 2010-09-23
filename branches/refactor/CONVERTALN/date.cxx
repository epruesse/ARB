#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include "convert.h"
#include "global.h"

#define SIZE    128
static const char *mon[12] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

/* --------------------------------------------------------------------
 *   Function genbank_date().
 *       Convert the date to be in genbank date form.
 */
char *genbank_date(char *other_date) {
    char *gdate, temp[SIZE];
    int   day, month, year;
    int   length;

    length = Lenstr(other_date);
    if (other_date[length - 1] == '\n') {
        length--;
        other_date[length] = '\0';
    }

    if (length <= 10 && length >= 6) {
        find_date(other_date, &month, &day, &year);
    }
    else if (length > 10) {
        if (is_genbank_date(other_date)) {
            if (Lenstr(other_date) > 11)
                other_date[11] = '\0';
            gdate = Dupstr(other_date);
            return (gdate);
        }
        else
            find_date_long_form(other_date, &month, &day, &year);
    }
    else {
        sprintf(temp, "Unknown date format: %s, cannot convert.\n", other_date);
        warning(146, temp);
        return (Dupstr("\?\?-\?\?\?-\?\?\?\?"));
    }
    if (day <= 0 || month <= 0 || year <= 0 || day > 31 || month > 12) {
        sprintf(temp, "Wrong date format: %s\n", other_date);
        warning(147, temp);
        return (Dupstr("\?\?-\?\?\?-\?\?\?\?"));
    }
    if (day < 10)
        sprintf(temp, "0%d-%s-19%d", day, mon[month - 1], year);
    else
        sprintf(temp, "%d-%s-19%d", day, mon[month - 1], year);
    gdate = (char *)Dupstr(temp);
    return (gdate);

}

/* -----------------------------------------------------------
 *   Function find_date().
 *       Find day, month, year from date string.
 */
void find_date(const char *date_string, int *month, int *day, int *year) {
    int index, indi, count, nums[3];
    char token[20], determ;

    index = count = 0;
    nums[0] = nums[1] = nums[2] = 0;
    determ = ' ';

    if      (two_char(date_string, '.')) determ = '.';
    else if (two_char(date_string, '/')) determ = '/';
    else if (two_char(date_string, '-')) determ = '-';

    if (determ != ' ') {
        for (indi = 0; indi <= Lenstr(date_string); indi++) {
            if (date_string[indi] == determ || indi == Lenstr(date_string)) {
                token[index++] = '\0';
                nums[count++] = atoi(token);
                index = 0;
            }
            else
                token[index++] = date_string[indi];
        }
        *month = nums[0];
        *day   = nums[1];
        *year  = (nums[2] % 100);
        return;
    }
}

/* ------------------------------------------------------------------
 *   Function two_char().
 *       Return true if string has two determinator char.
 */
bool two_char(const char *str, char determ) {
    int count = 0;
    int len   = Lenstr(str);

    for (int indi = 0; indi < len; indi++)
        if (str[indi] == determ)
            count++;

    return count == 2;
}

/* -----------------------------------------------------------------
 *   Function find_date_long_term().
 *       Find day, month, year in the long term date string
 *           like day-of-week, month, day, time, year.
 */
void find_date_long_form(char *date_string, int *month, int *day, int *year) {
    int  indi, index, length, nums[3], num;
    char token[SIZE];

    nums[0] = nums[1] = nums[2] = 0;
    length = Lenstr(date_string);
    for (indi = 0, index = 0; index <= length; index++) {
        if (index == length || date_string[index] == ' '
            || date_string[index] == '\0' || date_string[index] == '\n'
            || date_string[index] == '\t' || date_string[index] == '(' || date_string[index] == ')' || date_string[index] == ',') {
            if (indi == 0)
                continue;       /* empty token */
            token[indi] = '\0';
            if ((num = ismonth(token)) > 0 && nums[0] == 0)
                nums[0] = num;
            else if ((num = isdatenum(token)) > 0) {
                if (nums[0] == 0 && num < 12)
                    nums[0] = num;
                else if (nums[1] == 0 && num < 31)
                    nums[1] = num;
                else if (nums[2] == 0)
                    nums[2] = (num % 100);
            }
            indi = 0;
        }
        else
            token[indi++] = date_string[index];
    }
    *month = nums[0];
    *day   = nums[1];
    *year  = nums[2];
    return;
}

/* --------------------------------------------------------------------
 *   Function ismonth().
 *       Return true if the char string is one of 12 months.
 *           Case insensitive.
 */
int ismonth(char *string) {
    int num = 0;

    // @@@ all conditions are wrong! (now it's obvious)
    if      (str_iequal(string, "JAN") == 0) num = 1;
    else if (str_iequal(string, "FEB") == 0) num = 2;
    else if (str_iequal(string, "MAR") == 0) num = 3;
    else if (str_iequal(string, "APR") == 0) num = 4;
    else if (str_iequal(string, "MAY") == 0) num = 5;
    else if (str_iequal(string, "JUN") == 0) num = 6;
    else if (str_iequal(string, "JUL") == 0) num = 7;
    else if (str_iequal(string, "AUG") == 0) num = 8;
    else if (str_iequal(string, "SEP") == 0) num = 9;
    else if (str_iequal(string, "OCT") == 0) num = 10;
    else if (str_iequal(string, "NOV") == 0) num = 11;
    else if (str_iequal(string, "DEC") == 0) num = 12;
    return (num);
}

/* ------------------------------------------------------------------
 *   Function isdatenum().
 *       Return number of day or year the string represents.
 *           If not day or year, return 0.
 */
int isdatenum(char *string) {
    int length, num, indi;

    length = Lenstr(string);
    if (length > 4 || length < 1)
        return (0);
    for (indi = 0, num = 1; indi < length && num == 1; indi++) {
        if (!isdigit(string[indi])) {
            num = 0;
        }
    }
    if (num == 1)
        num = atoi(string);
    return (num);
}

/* ------------------------------------------------------------------
 *   Function is_genbank_date().
 *       Return true if it is genbank form of date, which is
 *           day(2 digits)-MONTH(in letters)-year(4 digits).
 */
int is_genbank_date(char *string) {
    if (Lenstr(string) >= 11 && string[2] == '-' && string[6] == '-')
        return (1);
    else
        return (0);
}

/* ----------------------------------------------------------------
 *   Function today_date().
 *       Get today's date.
 */
char *today_date() {
    struct timeval  tp;
    struct timezone tzp;
    char            line[SIZE], *return_value;

    (void)gettimeofday(&tp, &tzp);

    Cpystr(line, ctime(&(tp.tv_sec)));

    return_value = Dupstr(line);

    return (return_value);
}

/* ---------------------------------------------------------------
 *   Function gcg_date().
 *       Create gcg format of date.
 */
char *gcg_date(char *date_string) {
    static char date[128];

    ca_assert(strlen(date_string) >= 8);

    char temp[128];
    temp[0] = '\0';

    ca_assert(date_string[7] == ' ');
    date_string[7] = '\0';

    if (str_equal("Jan", date_string + 4)) Catstr(temp, "January");
    else if (str_equal("Feb", date_string + 4)) Catstr(temp, "February");
    else if (str_equal("Mar", date_string + 4)) Catstr(temp, "March");
    else if (str_equal("Apr", date_string + 4)) Catstr(temp, "April");
    else if (str_equal("May", date_string + 4)) Catstr(temp, "May");
    else if (str_equal("Jun", date_string + 4)) Catstr(temp, "June");
    else if (str_equal("Jul", date_string + 4)) Catstr(temp, "July");
    else if (str_equal("Aug", date_string + 4)) Catstr(temp, "August");
    else if (str_equal("Sep", date_string + 4)) Catstr(temp, "September");
    else if (str_equal("Oct", date_string + 4)) Catstr(temp, "October");
    else if (str_equal("Nov", date_string + 4)) Catstr(temp, "November");
    else if (str_equal("Dec", date_string + 4)) Catstr(temp, "December");
    
    date_string[7] = ' ';

    char time[128];
    int  day, year;
    int  scanned = sscanf(date_string + 8, "%d %s %d", &day, time, &year);
    ca_assert(scanned == 3);

    sprintf(date, "%s %d, %d  %s", temp, day, year, time);

    return date;
}

// --------------------------------------------------------------------------------

#if (UNIT_TESTS == 1)
#include <test_unit.h>

#define TEST_ASSERT_DATE_IMPL(input,expect,ASSERTION,CONVERT)   \
    do {                                                        \
        char *dup_ = strdup(input);                             \
        ASSERTION(CONVERT(dup_), expect);                       \
        free(dup_);                                             \
    } while(0)


#define TEST_ASSERT_GENBANK_DATE(input,expect)         TEST_ASSERT_DATE_IMPL(input, expect, TEST_ASSERT_EQUAL, genbank_date)
#define TEST_ASSERT_GENBANK_DATE__BROKEN(input,expect) TEST_ASSERT_DATE_IMPL(input, expect, TEST_ASSERT_EQUAL__BROKEN, genbank_date)
#define TEST_ASSERT_GCG_DATE(input,expect)             TEST_ASSERT_DATE_IMPL(input, expect, TEST_ASSERT_EQUAL, gcg_date)
#define TEST_ASSERT_GCG_DATE__BROKEN(input,expect)     TEST_ASSERT_DATE_IMPL(input, expect, TEST_ASSERT_EQUAL__BROKEN, gcg_date)

#define TEST_ASSERT_FIND_ANYDATE(input,d,m,y,finder)    \
    do {                                                \
        char *dup_ = strdup(input);                     \
        int   day_,month_,year_;                        \
                                                        \
        finder(dup_, &month_, &day_, &year_);           \
        TEST_ASSERT_EQUAL(day_, d);                     \
        TEST_ASSERT_EQUAL(month_, m);                   \
        TEST_ASSERT_EQUAL(year_, y);                    \
        free(dup_);                                     \
    } while(0)

#define TEST_ASSERT_FIND_____DATE(input,d,m,y) TEST_ASSERT_FIND_ANYDATE(input,d,m,y,find_date)
#define TEST_ASSERT_FIND_LONGDATE(input,d,m,y) TEST_ASSERT_FIND_ANYDATE(input,d,m,y,find_date_long_form)

// #define TEST_ASSERT_FIND_DATE(str,d,m,y) TEST_ASSERT_FIND_DATE_IMPL(str,d,m,y,TEST_ASSERT_EQUAL)


void TEST_conv_date() {
    const char *error_date = "\?\?-\?\?\?-\?\?\?\?";

    // @@@ broken behavior (day completely broken, month contains day, year<100)
    TEST_ASSERT_FIND_____DATE("19-APR-1999", 0, 19, 99);
    TEST_ASSERT_FIND_____DATE("22-SEP-2010", 0, 22, 10);
    MISSING_TEST(correct behavior);
    // TEST_ASSERT_FIND_____DATE("19-APR-1999", 19, 4, 1999);
    // TEST_ASSERT_FIND_____DATE("22-SEP-2010", 22, 9, 2010);

    // @@@ broken behavior (month completely broken, year<100)
    TEST_ASSERT_FIND_LONGDATE("Mon Apr 19 25:46:19 CEST 1999", 19, 1, 99);
    TEST_ASSERT_FIND_LONGDATE("Wed Sep 22 19:46:25 CEST 2010", 22, 1, 10);
    MISSING_TEST(correct behavior);
    // TEST_ASSERT_FIND_LONGDATE("Mon Apr 19 25:46:19 CEST 1999", 19, 4, 1999);
    // TEST_ASSERT_FIND_LONGDATE("Wed Sep 22 19:46:25 CEST 2010", 22, 9, 2010);
    
    TEST_ASSERT_GENBANK_DATE("19 Apr 1999", error_date); // "Wrong date format"

    TEST_ASSERT_GENBANK_DATE("19-APR-1999", "19-APR-1999");
    TEST_ASSERT_GENBANK_DATE("22-SEP-2010", "22-SEP-2010");

    TEST_ASSERT_GENBANK_DATE__BROKEN("Wed Sep 22 19:46:25 CEST 2010", "22-SEP-2010");

    TEST_ASSERT_GCG_DATE("Mon Apr 19 25:46:19 1999", "April 19, 1999  25:46:19");
    TEST_ASSERT_GCG_DATE("Wed Sep 22 19:46:25 2010", "September 22, 2010  19:46:25");
    TEST_ASSERT(gcg_date(today_date())); // gcg_date is only used like this
}

#endif // UNIT_TESTS

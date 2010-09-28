#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include "convert.h"
#include "global.h"

#define SIZE 128 // default buffer size for generated dates

static const char *ERROR_DATE = "\?\?-\?\?\?-\?\?\?\?";

static const char *MON[12] = {
    "JAN", "FEB", "MAR",
    "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP",
    "OCT", "NOV", "DEC"
};


static const char *Month[12] = {
    "January", "February", "March",
    "April",   "May",      "June",
    "July",    "August",   "September",
    "October", "November", "December"
};



/* --------------------------------------------------------------------
 *   Function genbank_date().
 *       Convert the date to be in genbank date form.
 */
const char *genbank_date(const char *other_date) {
    const char *result;
    int         length = Lenstr(other_date);

    if (other_date[length - 1] == '\n') {
        char *dup     = Dupstr(other_date);
        dup[--length] = '\0';
        result        = genbank_date(dup);
        free(dup);
    }
    else {
        static char gdate[SIZE];
        gdate[0] = 0;

        int day, month, year;
        if (length <= 10 && length >= 6) {
            find_date(other_date, &month, &day, &year);
        }
        else if (length > 10) {
            if (is_genbank_date(other_date)) {
                strncpy(gdate, other_date, 11);
                gdate[11] = 0;
            }
            else {
                find_date_long_form(other_date, &month, &day, &year);
            }
        }
        else {
            warningf(146, "Unknown date format: %s, cannot convert.", other_date);
            strcpy(gdate, ERROR_DATE);
        }

        if (!gdate[0]) {
            if (day <= 0 || month <= 0 || year <= 0 || day > 31 || month > 12) {
                warningf(147, "Wrong date format: %s", other_date);
                strcpy(gdate, ERROR_DATE);
            }
            else {
                sprintf(gdate, "%02d-%s-19%d", day, MON[month - 1], year);
            }
        }

        ca_assert(gdate[0]);
        result = gdate;
    }
    return result;
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
    for (const char *d = strchr(str, determ); d; d = strchr(d+1, determ)) count++;
    return count;
}

/* -----------------------------------------------------------------
 *   Function find_date_long_term().
 *       Find day, month, year in the long term date string
 *           like day-of-week, month, day, time, year.
 */
void find_date_long_form(const char *date_string, int *month, int *day, int *year) {
    int nums[3] = { 0, 0, 0 };
    int length = Lenstr(date_string);

    for (int indi = 0, index = 0; index <= length; index++) {
        char token[SIZE];
        if ((index == length) || isspace(date_string[index]) || strchr("(),", date_string[index]) != 0) {
            if (indi == 0) continue; /* empty token */
            token[indi] = '\0';

            int num = ismonth(token);
            if (num>0 && nums[0] == 0) {
                nums[0] = num;
            }
            else if ((num = isdatenum(token)) > 0) {
                if (nums[0] == 0 && num < 12)           nums[0] = num;
                else if (nums[1] == 0 && num < 31)      nums[1] = num;
                else if (nums[2] == 0)                  nums[2] = (num % 100);
            }
            indi = 0;
        }
        else token[indi++] = date_string[index];
    }

    *month = nums[0];
    *day   = nums[1];
    *year  = nums[2];
}

/* --------------------------------------------------------------------
 *   Function ismonth().
 *       Return [1..12] if the char string is one of 12 months.
 *           Case insensitive.
 */
int ismonth(const char *str) {
    for (int i = 0; i<12; i++) {
        if (str_iequal(str, MON[i])) {
            return i+1;
        }
    }
    return 0;
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
int is_genbank_date(const char *str) {
    return Lenstr(str) >= 11 && str[2] == '-' && str[6] == '-';
}

/* ----------------------------------------------------------------
 *   Function today_date().
 *       Get today's date.
 */
const char *today_date() {
    static char line[SIZE] = "";
    if (!line[0]) {
        struct timeval  tp;
        struct timezone tzp;
        (void)gettimeofday(&tp, &tzp);

        Cpystr(line, ctime(&(tp.tv_sec)));

        int len = strlen(line);
        if (line[len-1] == '\n') {
            line[len-1] = 0;
        }
    }
    return line;
}

/* ---------------------------------------------------------------
 *   Function gcg_date().
 *       Create gcg format of date.
 */
const char *gcg_date(const char *input) {
    static char date[SIZE];

    ca_assert(strlen(input) >= 8);

    const int MONTH_POS = 4;
    const int MONTH_LEN = 3;
    const int DAY_POS = MONTH_POS+MONTH_LEN+1;

    const char *monthname = ""; 
    {
        char part[MONTH_LEN+1];
        memcpy(part, input+MONTH_POS, MONTH_LEN);
        part[MONTH_LEN] = 0;

        int month = ismonth(part);
        if (month) monthname = Month[month-1];
    }

    char time[SIZE];
    int  day, year;
    int  scanned = sscanf(input+DAY_POS, "%d %s %d", &day, time, &year);
    ca_assert(scanned == 3);

    sprintf(date, "%s %d, %d  %s", monthname, day, year, time);
    return date;
}

// --------------------------------------------------------------------------------

#if (UNIT_TESTS == 1)
#include <test_unit.h>

#define TEST_ASSERT_CONVERT(input,expect,CONVERT,ASSERTION) ASSERTION(CONVERT(input), expect);

#define TEST_ASSERT_GENBANK_DATE(input,expect)         TEST_ASSERT_CONVERT(input, expect, genbank_date, TEST_ASSERT_EQUAL)
#define TEST_ASSERT_GENBANK_DATE__BROKEN(input,expect) TEST_ASSERT_CONVERT(input, expect, genbank_date, TEST_ASSERT_EQUAL__BROKEN)
#define TEST_ASSERT_GCG_DATE(input,expect)             TEST_ASSERT_CONVERT(input, expect, gcg_date, TEST_ASSERT_EQUAL)
#define TEST_ASSERT_GCG_DATE__BROKEN(input,expect)     TEST_ASSERT_CONVERT(input, expect, gcg_date, TEST_ASSERT_EQUAL__BROKEN)

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
    TEST_ASSERT_EQUAL(ismonth("Apr"), 4);

    // @@@ broken behavior (day completely broken, month contains day, year<100)
    TEST_ASSERT_FIND_____DATE("19-APR-99", 0, 19, 99);
    TEST_ASSERT_FIND_____DATE("22-JUN-65", 0, 22, 65);
    TEST_ASSERT_FIND_____DATE("5-SEP-10", 0, 5, 10);
    TEST_ASSERT_FIND_____DATE("05-SEP-10", 0, 5, 10);

    TEST_ASSERT_FIND_____DATE("19-APR-1999", 0, 19, 99);
    TEST_ASSERT_FIND_____DATE("22-JUN-1965", 0, 22, 65); // test date b4 epoch
    TEST_ASSERT_FIND_____DATE("5-SEP-2010", 0, 5, 10);
    TEST_ASSERT_FIND_____DATE("05-SEP-2010", 0, 5, 10);

    MISSING_TEST(correct behavior);
    // TEST_ASSERT_FIND_____DATE("19-APR-1999", 19, 4, 1999);
    // TEST_ASSERT_FIND_____DATE("22-SEP-2010", 22, 9, 2010);

    // --------------------

    // @@@ broken behavior (month completely broken, year<100)
    TEST_ASSERT_FIND_LONGDATE("Mon Apr 19 25:46:19 CEST 99", 19, 4, 99);
    TEST_ASSERT_FIND_LONGDATE("Tue Jun 22 05:11:00 CEST 65", 22, 6, 65);
    TEST_ASSERT_FIND_LONGDATE("Wed Sep 5 19:46:25 CEST 10", 5, 9, 10);
    TEST_ASSERT_FIND_LONGDATE("Wed Sep 05 19:46:25 CEST 10", 5, 9, 10);

    TEST_ASSERT_FIND_LONGDATE("Mon Apr 19 25:46:19 CEST 1999", 19, 4, 99);
    TEST_ASSERT_FIND_LONGDATE("Tue Jun 22 05:11:00 CEST 1965", 22, 6, 65);
    TEST_ASSERT_FIND_LONGDATE("Wed Sep 5 19:46:25 CEST 2010", 5, 9, 10);
    TEST_ASSERT_FIND_LONGDATE("Wed Sep 05 19:46:25 CEST 2010", 5, 9, 10);
    
    MISSING_TEST(correct behavior);
    // TEST_ASSERT_FIND_LONGDATE("Mon Apr 19 25:46:19 CEST 1999", 19, 4, 1999);
    // TEST_ASSERT_FIND_LONGDATE("Wed Sep 22 19:46:25 CEST 2010", 22, 9, 2010);

    // --------------------
    
    TEST_ASSERT_GENBANK_DATE("19 Apr 1999", "19-APR-1999");
    TEST_ASSERT_GENBANK_DATE("19-APR-1999", "19-APR-1999");
    TEST_ASSERT_GENBANK_DATE("22-JUN-1965", "22-JUN-1965");
    TEST_ASSERT_GENBANK_DATE("5-SEP-2010", ERROR_DATE);
    TEST_ASSERT_GENBANK_DATE("05-SEP-2010", "05-SEP-2010");
    TEST_ASSERT_GENBANK_DATE("crap", ERROR_DATE);

    TEST_ASSERT_GENBANK_DATE        ("Mon Apr 19 25:46:19 CEST 1999", "19-APR-1999");
    TEST_ASSERT_GENBANK_DATE        ("Tue Jun 22 05:11:00 CEST 1965", "22-JUN-1965");
    TEST_ASSERT_GENBANK_DATE__BROKEN("Wed Sep 5 19:46:25 CEST 2010",  "05-SEP-2010");
    TEST_ASSERT_GENBANK_DATE__BROKEN("Wed Sep 05 19:46:25 CEST 2010", "05-SEP-2010");

    // --------------------

    TEST_ASSERT_GCG_DATE("Mon Apr 19 25:46:19 99", "April 19, 99  25:46:19");

    TEST_ASSERT_GCG_DATE("Mon Apr 19 25:46:19 1999", "April 19, 1999  25:46:19");
    TEST_ASSERT_GCG_DATE("Tue Jun 22 05:11:00 1965", "June 22, 1965  05:11:00");
    TEST_ASSERT_GCG_DATE("Wed Sep 5 19:46:25 2010", "September 5, 2010  19:46:25");
    TEST_ASSERT_GCG_DATE("Wed Sep 05 19:46:25 2010", "September 5, 2010  19:46:25");

    TEST_ASSERT(gcg_date(today_date())); // currently gcg_date is only used like this
}

#endif // UNIT_TESTS

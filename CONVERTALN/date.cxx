#include "fun.h"
#include "global.h"

#include <time.h>
#include <sys/time.h>

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

static unsigned char days_in_month[12+1] = {
    0xFF, 
    31, 29, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

inline bool two_char(const char *str, char determ) {
    // Return true if Str has two determinator char.
    int count = 0;
    for (const char *d = strchr(str, determ); d; d = strchr(d+1, determ)) count++;
    return count;
}

inline int ismonth(const char *str) {
    // Return [1..12] if the char Str is one of 12 months. Case insensitive.
    for (int i = 0; i<12; i++) {
        if (str_iequal(str, MON[i])) {
            return i+1;
        }
    }
    return 0;
}


__ATTR__USERESULT static bool find_date(const char *date_string, int *month, int *day, int *year) {
    // Find day, month, year from date Str.
    char determ                                 = ' ';
    if      (two_char(date_string, '.')) determ = '.';
    else if (two_char(date_string, '/')) determ = '/';
    else if (two_char(date_string, '-')) determ = '-';

    if (determ == ' ') return false;

    char token[20];
    int nums[3] = { 0, 0, 0 };
    int count = 0;
    int index = 0;

    int len = str0len(date_string);
    for (int indi = 0; indi <= len; indi++) {
        if (date_string[indi] == determ || indi == len) {
            token[index++] = '\0';
            if (count == 1) {
                nums[count++] = ismonth(token);
            }
            else {
                nums[count++] = atoi(token);
            }
            index = 0;
        }
        else {
            token[index++] = date_string[indi];
        }
    }
    *day   = nums[0];
    *month = nums[1];
    *year  = nums[2];

    return true;
}

static int isdatenum(char *Str) {
    // Return number of day or year the Str represents.
    // If not day or year, return 0.
    int length, num, indi;

    length = str0len(Str);
    if (length > 4 || length < 1)
        return (0);
    for (indi = 0, num = 1; indi < length && num == 1; indi++) {
        if (!isdigit(Str[indi])) {
            num = 0;
        }
    }
    if (num == 1)
        num = atoi(Str);
    return (num);
}

class SetOnce {
    int  num_;
    bool set_;
    bool is_set() const { return set_; }
public:
    SetOnce() : num_(-1), set_(false) {}

    bool operator!() const { return !set_; }

    int value() const { ca_assert(is_set()); return num_; }
    void set(int val) { ca_assert(!is_set()); num_ = val; set_ = true; }
    void replace(int val) { ca_assert(is_set()); num_ = val; }
};

__ATTR__USERESULT static bool find_date_long_form(const char *date_string, int *monthPtr, int *dayPtr, int *yearPtr) {
    // Find day, month, year in the long term date Str like day-of-week, month, day, time, year.

    int     length = str0len(date_string);
    SetOnce day, month, year;

    char token[SIZE];
    for (int indi = 0, index = 0; index <= length; index++) {
        if ((index == length) || isspace(date_string[index]) || strchr("(),", date_string[index]) != 0) {
            if (indi == 0) continue; // empty token
            token[indi] = '\0';

            int num = ismonth(token);
            if (num>0) {
                if (!month) month.set(num);
                else if (!day) {
                    day.set(month.value()); // day has been misinterpreted as month
                    month.replace(num);
                }
            }
            else if ((num = isdatenum(token)) > 0) {
                if      (!month && num <= 12) { month.set(num); }
                else if (!day   && num <= 31) { day.set(num); }
                else if (!year)               { year.set(num); }
            }
            indi = 0;
        }
        else token[indi++] = date_string[index];
    }

    if (!day || !month || !year ||
        day.value()>days_in_month[month.value()]) return false;

    *monthPtr = month.value();
    *dayPtr   = day.value();
    *yearPtr  = year.value();

    return true;
}

inline bool is_genbank_date(const char *str) {
    // Return true if it is genbank form of date,
    // which is day(2 digits)-MONTH(in letters)-year(4 digits).
    return str0len(str) >= 11 && str[2] == '-' && str[6] == '-';
}

const char *genbank_date(const char *other_date) {
    // Convert the date to be in genbank date form.
    const char *result;
    int         length = str0len(other_date);

    if (other_date[length - 1] == '\n') {
        char *dup     = nulldup(other_date);
        dup[--length] = '\0';
        result        = genbank_date(dup);
        free(dup);
    }
    else {
        static char gdate[SIZE];
        gdate[0] = 0;

        int  day, month, year;
        bool ok = false;
        if (length > 10) {
            if (is_genbank_date(other_date)) {
                strncpy(gdate, other_date, 11);
                gdate[11] = 0;
                ok        = true;
            }
            else ok = find_date_long_form(other_date, &month, &day, &year);
        }

        if (!ok) ok = find_date(other_date, &month, &day, &year);

        if (!ok) {
            warningf(146, "Unknown date format: %s, cannot convert.", other_date);
            strcpy(gdate, ERROR_DATE);
        }

        if (!gdate[0]) {
            if (day <= 0 || month <= 0 || year <= 0 || month > 12 || day > days_in_month[month]) {
                warningf(147, "Wrong date format: %s", other_date);
                strcpy(gdate, ERROR_DATE);
            }
            else {
                if (year<100) year += 1900;
                sprintf(gdate, "%02d-%s-%d", day, MON[month - 1], year);
            }
        }

        ca_assert(gdate[0]);
        result = gdate;
    }
    return result;
}

const char *today_date() {
    // Get today's date.
    static char line[SIZE] = "";
    if (!line[0]) {
        struct timeval  tp;
        struct timezone tzp;
        (void)gettimeofday(&tp, &tzp);

        strcpy(line, ctime(&(tp.tv_sec)));

        int len = strlen(line);
        if (line[len-1] == '\n') {
            line[len-1] = 0;
        }
    }
    return line;
}

const char *gcg_date(const char *input) {
    // Create gcg format of date.
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
    IF_ASSERTION_USED(int scanned = )
        sscanf(input+DAY_POS, "%d %s %d", &day, time, &year);
    ca_assert(scanned == 3);

    sprintf(date, "%s %d, %d  %s", monthname, day, year, time);
    return date;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

#define TEST_EXPECT_CONVERT(input,expect,CONVERT,ASSERTION) ASSERTION(CONVERT(input), expect);

#define TEST_EXPECT_GENBANK_DATE(input,expect)         TEST_EXPECT_CONVERT(input, expect, genbank_date, TEST_EXPECT_EQUAL)
#define TEST_EXPECT_GENBANK_DATE__BROKEN(input,expect) TEST_EXPECT_CONVERT(input, expect, genbank_date, TEST_EXPECT_EQUAL__BROKEN)
#define TEST_EXPECT_GCG_DATE(input,expect)             TEST_EXPECT_CONVERT(input, expect, gcg_date, TEST_EXPECT_EQUAL)
#define TEST_EXPECT_GCG_DATE__BROKEN(input,expect)     TEST_EXPECT_CONVERT(input, expect, gcg_date, TEST_EXPECT_EQUAL__BROKEN)

#define TEST_EXPECT_INVALID_ANYDATE(input,finder)                       \
    do {                                                                \
        int   day_, month_, year_;                                      \
        ASSERT_RESULT(bool, false,                                      \
                      finder(input, &month_, &day_, &year_));           \
    } while(0)

#define TEST_EXPECT_INVALID_LONGDATE(input) TEST_EXPECT_INVALID_ANYDATE(input, find_date_long_form)

#define TEST_EXPECT_FIND_ANYDATE(input,d,m,y,finder) \
    do {                                                        \
        char *dup_ = strdup(input);                             \
        int   day_, month_, year_;                              \
        TEST_EXPECT(finder(dup_, &month_, &day_, &year_));      \
        TEST_EXPECT_EQUAL(day_, d);                             \
        TEST_EXPECT_EQUAL(month_, m);                           \
        TEST_EXPECT_EQUAL(year_, y);                            \
        free(dup_);                                             \
    } while (0)

#define TEST_EXPECT_FIND_____DATE(input,d,m,y) TEST_EXPECT_FIND_ANYDATE(input, d, m, y, find_date)
#define TEST_EXPECT_FIND_LONGDATE(input,d,m,y) TEST_EXPECT_FIND_ANYDATE(input, d, m, y, find_date_long_form)

// #define TEST_EXPECT_FIND_DATE(str,d,m,y) TEST_EXPECT_FIND_DATE_IMPL(str,d,m,y,TEST_EXPECT_EQUAL)

void TEST_BASIC_conv_date() {
    TEST_EXPECT_EQUAL(ismonth("Apr"), 4);

    TEST_EXPECT_FIND_____DATE("19-APR-99", 19, 4, 99);
    TEST_EXPECT_FIND_____DATE("22-JUN-65", 22, 6, 65);
    TEST_EXPECT_FIND_____DATE("5-SEP-10",   5, 9, 10);
    TEST_EXPECT_FIND_____DATE("05-SEP-10",  5, 9, 10);

    TEST_EXPECT_FIND_____DATE("19-APR-1999", 19, 4, 1999);
    TEST_EXPECT_FIND_____DATE("22-JUN-1965", 22, 6, 1965); // test date b4 epoch
    TEST_EXPECT_FIND_____DATE("5-SEP-2010",   5, 9, 2010);
    TEST_EXPECT_FIND_____DATE("05-SEP-2010",  5, 9, 2010);

    // --------------------

    TEST_EXPECT_FIND_LONGDATE("05 Sep 2010",  5, 9, 2010);
    TEST_EXPECT_FIND_LONGDATE("Sep, 05 2010",  5, 9, 2010);
    TEST_EXPECT_FIND_LONGDATE("Sep 05 2010",  5, 9, 2010);

    TEST_EXPECT_FIND_LONGDATE("Mon Apr 19 25:46:19 CEST 99", 19, 4, 99);
    TEST_EXPECT_FIND_LONGDATE("Tue Jun 22 05:11:00 CEST 65", 22, 6, 65);
    TEST_EXPECT_FIND_LONGDATE("Wed Sep 5 19:46:25 CEST 10",   5, 9, 10);
    TEST_EXPECT_FIND_LONGDATE("Wed Sep 05 19:46:25 CEST 10",  5, 9, 10);

    TEST_EXPECT_FIND_LONGDATE("Mon Apr 19 25:46:19 CEST 1999", 19, 4, 1999);
    TEST_EXPECT_FIND_LONGDATE("Tue Jun 22 05:11:00 CEST 1965", 22, 6, 1965);
    TEST_EXPECT_FIND_LONGDATE("Wed Sep 5 19:46:25 CEST 2010",   5, 9, 2010);
    TEST_EXPECT_FIND_LONGDATE("Wed Sep 05 19:46:25 CEST 2010",  5, 9, 2010);
    TEST_EXPECT_FIND_LONGDATE("Wed Sep 05 19:46:25 2010",  5, 9, 2010);
    
    TEST_EXPECT_FIND_LONGDATE("Sun Oct 31 08:37:14 2010",  31, 10, 2010);

    // --------------------

    TEST_EXPECT_GENBANK_DATE("19 Apr 1999", "19-APR-1999");
    TEST_EXPECT_GENBANK_DATE("19-APR-1999", "19-APR-1999");
    TEST_EXPECT_GENBANK_DATE("22-JUN-1965", "22-JUN-1965");
    TEST_EXPECT_GENBANK_DATE("5-SEP-2010", "05-SEP-2010");
    TEST_EXPECT_GENBANK_DATE("05-SEP-2010", "05-SEP-2010");
    TEST_EXPECT_GENBANK_DATE("crap", ERROR_DATE);

    TEST_EXPECT_GENBANK_DATE("Mon Apr 19 25:46:19 CEST 1999", "19-APR-1999");
    TEST_EXPECT_GENBANK_DATE("Tue Jun 22 05:11:00 CEST 1965", "22-JUN-1965");
    TEST_EXPECT_GENBANK_DATE("Wed Sep 5 19:46:25 CEST 2010",  "05-SEP-2010");
    TEST_EXPECT_GENBANK_DATE("Wed Sep 05 19:46:25 CEST 2010", "05-SEP-2010");
    TEST_EXPECT_GENBANK_DATE("Wed Sep 31 19:46:25 CEST 2010", ERROR_DATE);

    TEST_EXPECT_GENBANK_DATE("Sun Oct 31 08:37:14 2010", "31-OCT-2010");
    TEST_EXPECT_GENBANK_DATE("Sun 10 31 08:37:14 2010", "31-OCT-2010");
    TEST_EXPECT_GENBANK_DATE("Sun 31 10 08:37:14 2010", "31-OCT-2010");
    TEST_EXPECT_GENBANK_DATE("Sun Oct 32 08:37:14 2010", ERROR_DATE);
    
    TEST_EXPECT_GENBANK_DATE("Fri Dec 31 08:37:14 2010", "31-DEC-2010");
    TEST_EXPECT_GENBANK_DATE("Fri 12 31 08:37:14 2010", "31-DEC-2010");
    TEST_EXPECT_GENBANK_DATE("Fri 31 12 08:37:14 2010", "31-DEC-2010");
    TEST_EXPECT_GENBANK_DATE("Fri 13 31 08:37:14 2010", ERROR_DATE);
    TEST_EXPECT_GENBANK_DATE("Fri 31 13 08:37:14 2010", ERROR_DATE);

    TEST_EXPECT_GENBANK_DATE("Tue Feb 28 08:37:14 2011", "28-FEB-2011");
    TEST_EXPECT_GENBANK_DATE("Tue Feb 29 08:37:14 2011", "29-FEB-2011"); // existence not checked
    TEST_EXPECT_GENBANK_DATE("Tue Feb 30 08:37:14 2011", ERROR_DATE);    // existence not checked

    TEST_EXPECT_DIFFERENT(genbank_date(today_date()), ERROR_DATE);

    // --------------------

    TEST_EXPECT_GCG_DATE("Mon Apr 19 25:46:19 99", "April 19, 99  25:46:19");

    TEST_EXPECT_GCG_DATE("Mon Apr 19 25:46:19 1999", "April 19, 1999  25:46:19");
    TEST_EXPECT_GCG_DATE("Tue Jun 22 05:11:00 1965", "June 22, 1965  05:11:00");
    TEST_EXPECT_GCG_DATE("Wed Sep 5 19:46:25 2010",  "September 5, 2010  19:46:25");
    TEST_EXPECT_GCG_DATE("Wed Sep 05 19:46:25 2010", "September 5, 2010  19:46:25");

    TEST_REJECT_NULL(gcg_date(today_date())); // currently gcg_date is only used like this
}

#endif // UNIT_TESTS

#ifndef ARBDB_H
#include <arbdb.h>
#endif

#include "aw_window.hxx"
#include "ctype.h"
// --------------------------------------------------------------------------------
//                                 Hotkey Checking (DEBUG)
// --------------------------------------------------------------------------------


#ifdef DEBUG

#define MAX_DEEP_TO_TEST       10
#define MAX_MENU_ITEMS_TO_TEST 50

static char *TD_menu_name = 0;
static char  TD_mnemonics[MAX_DEEP_TO_TEST][MAX_MENU_ITEMS_TO_TEST];
static int   TD_topics[MAX_DEEP_TO_TEST];

struct SearchPossibilities { //: public Noncopyable {
    char *menu_topic;
    SearchPossibilities *next;

    SearchPossibilities(const char* menu_topic_, SearchPossibilities *next_) {
        menu_topic = strdup(menu_topic_);
        next = next_;
    }

    ~SearchPossibilities() {
        free(menu_topic);
        delete next;
    }
};

typedef SearchPossibilities *SearchPossibilitiesPtr;
static SearchPossibilitiesPtr TD_poss[MAX_DEEP_TO_TEST] = { 0 };

inline void addToPoss(int menu_deep, const char *topic_name) {
    TD_poss[menu_deep] = new SearchPossibilities(topic_name, TD_poss[menu_deep]);
}

inline char oppositeCase(char c) {
    return isupper(c) ? tolower(c) : toupper(c);
}

static void strcpy_overlapping(char *dest, char *src) {
    int src_len = strlen(src);
    memmove(dest, src, src_len+1);
}

static const char *possible_mnemonics(int menu_deep, const char *topic_name) {
    int t;
    static char *unused;

    freedup(unused, topic_name);

    for (t = 0; unused[t]; ++t) {
        bool remove = false;
        if (!isalnum(unused[t])) { // remove useless chars
            remove = true;
        }
        else {
            char *dup = strchr(unused, unused[t]);
            if (dup && (dup-unused)<t) { // remove duplicated chars
                remove = true;
            }
            else {
                dup = strchr(unused, oppositeCase(unused[t]));
                if (dup && (dup-unused)<t) { // char is duplicated with opposite case
                    dup[0] = toupper(dup[0]); // prefer upper case
                    remove = true;
                }
            }
        }
        if (remove) {
            strcpy_overlapping(unused+t, unused+t+1);
            --t;
        }
    }

    int topics = TD_topics[menu_deep];
    for (t = 0; t<topics; ++t) {
        char c = TD_mnemonics[menu_deep][t]; // upper case!
        char *u = strchr(unused, c);
        if (u)
            strcpy_overlapping(u, u+1); // remove char
        u = strchr(unused, tolower(c));
        if (u)
            strcpy_overlapping(u, u+1); // remove char
    }

    return unused;
}

static void printPossibilities(int menu_deep) {
    SearchPossibilities *sp = TD_poss[menu_deep];
    while (sp) {
        const char *poss = possible_mnemonics(menu_deep, sp->menu_topic);
        fprintf(stderr, "          - Possibilities for '%s': '%s'\n", sp->menu_topic, poss);

        sp = sp->next;
    }

    delete TD_poss[menu_deep];
    TD_poss[menu_deep] = 0;
}

static int menu_deep_check = 0;

static void test_duplicate_mnemonics(int menu_deep, const char *topic_name, const char *mnemonic) {
    if (mnemonic && mnemonic[0] != 0) {
        if (mnemonic[1]) { // longer than 1 char -> wrong
            fprintf(stderr, "Warning: Hotkey '%s' is too long; only 1 character allowed (%s|%s)\n", mnemonic, TD_menu_name, topic_name);
        }
        if (topic_name[0] == '#') { // graphical menu
            if (mnemonic[0]) {
                fprintf(stderr, "Warning: Hotkey '%s' is useless for graphical menu entry (%s|%s)\n", mnemonic, TD_menu_name, topic_name);
            }
        }
        else {
            if (strchr(topic_name, mnemonic[0])) {  // occurs in menu text
                int topics = TD_topics[menu_deep];
                int t;
                char hotkey = toupper(mnemonic[0]); // store hotkeys case-less (case does not matter when pressing the hotkey)

                TD_mnemonics[menu_deep][topics] = hotkey;

                for (t=0; t<topics; t++) {
                    if (TD_mnemonics[menu_deep][t]==hotkey) {
                        fprintf(stderr, "Warning: Hotkey '%c' used twice (%s|%s)\n", hotkey, TD_menu_name, topic_name);
                        addToPoss(menu_deep, topic_name);
                        break;
                    }
                }

                TD_topics[menu_deep] = topics+1;
            }
            else {
                fprintf(stderr, "Warning: Hotkey '%c' is useless; does not occur in text (%s|%s)\n", mnemonic[0], TD_menu_name, topic_name);
                addToPoss(menu_deep, topic_name);
            }
        }
    }
#if defined(DEVEL_RALF)
    else {
        if (topic_name[0] != '#') { // not a graphical menu
            fprintf(stderr, "Warning: Missing hotkey for (%s|%s)\n", TD_menu_name, topic_name);
            addToPoss(menu_deep, topic_name);
        }
    }
#endif // DEVEL_RALF
}

static void open_test_duplicate_mnemonics(int menu_deep, const char *sub_menu_name, const char *mnemonic) {
    aw_assert(menu_deep == menu_deep_check+1);
    menu_deep_check = menu_deep;

    int len = strlen(TD_menu_name)+1+strlen(sub_menu_name)+1;
    char *buf = (char*)malloc(len);

    memset(buf, 0, len);
    sprintf(buf, "%s|%s", TD_menu_name, sub_menu_name);

    test_duplicate_mnemonics(menu_deep-1, sub_menu_name, mnemonic);

    freeset(TD_menu_name, buf);
    TD_poss[menu_deep] = 0;
}

static void close_test_duplicate_mnemonics(int menu_deep) {
    aw_assert(menu_deep == menu_deep_check);
    menu_deep_check = menu_deep-1;

    printPossibilities(menu_deep);
    TD_topics[menu_deep] = 0;

    aw_assert(TD_menu_name);
    // otherwise no menu was opened

    char *slash = strrchr(TD_menu_name, '|');
    if (slash) {
        slash[0] = 0;
    }
    else {
        TD_menu_name[0] = 0;
    }
}

static void init_duplicate_mnemonic() {
    int i;

    if (TD_menu_name) close_test_duplicate_mnemonics(1); // close last menu
    freedup(TD_menu_name, "");

    for (i=0; i<MAX_DEEP_TO_TEST; i++) {
        TD_topics[i] = 0;
    }
    aw_assert(menu_deep_check == 0);
}
static void exit_duplicate_mnemonic() {
    close_test_duplicate_mnemonics(1); // close last menu
    aw_assert(TD_menu_name);
    freenull(TD_menu_name);
    aw_assert(menu_deep_check == 0);
}

#endif // DEBUG

#ifndef RDP_INFO_H
#define RDP_INFO_H

#ifndef GLOBAL_H
#include "global.h"
#endif
#ifndef FUN_H
#include "fun.h"
#endif

// -------------------------------
//      RDP-defined comments (Embl+GenBank)

struct OrgInfo : virtual Noncopyable {
    char *source;
    char *cultcoll;
    char *formname;
    char *nickname;
    char *commname;
    char *hostorg;

    OrgInfo() {
        source   = no_content();
        cultcoll = no_content();
        formname = no_content();
        nickname = no_content();
        commname = no_content();
        hostorg  = no_content();
    }
    ~OrgInfo() {
        freenull(source);
        freenull(cultcoll);
        freenull(formname);
        freenull(nickname);
        freenull(commname);
        freenull(hostorg);
    }

    void set_content_from(const OrgInfo& other) {
        copy_content(source, other.source);
        copy_content(cultcoll, other.cultcoll);
        copy_content(formname, other.formname);
        copy_content(nickname, other.nickname);
        copy_content(commname, other.commname);
        copy_content(hostorg, other.hostorg);
    }

    bool exists() const {
        return (has_content(source) ||
                has_content(cultcoll) ||
                has_content(formname) ||
                has_content(nickname) ||
                has_content(commname) ||
                has_content(hostorg));
    }
};

struct SeqInfo : virtual Noncopyable {
    char comp3;  // yes or no, y/n
    char comp5;  // yes or no, y/n

    char *RDPid;
    char *gbkentry;
    char *methods;

    SeqInfo() {
        comp3    = ' ';
        comp5    = ' ';
        RDPid    = no_content();
        gbkentry = no_content();
        methods  = no_content();
    }
    ~SeqInfo() {
        freenull(RDPid);
        freenull(gbkentry);
        freenull(methods);
    }
    void set_content_from(const SeqInfo& other) {
        comp3 = other.comp3;
        comp5 = other.comp5;
        copy_content(RDPid, other.RDPid);
        copy_content(gbkentry, other.gbkentry);
        copy_content(methods, other.methods);
    }
    bool exists() const {
        return
            comp3 != ' ' ||
            comp5 != ' ' ||
            has_content(RDPid)    ||
            has_content(gbkentry) ||
            has_content(methods);
    }
};

struct RDP_comments : virtual Noncopyable {
    OrgInfo  orginf;
    SeqInfo  seqinf;
    char    *others;

    RDP_comments() : others(NULL) {}
    ~RDP_comments() { freenull(others); }

    void set_content_from(const RDP_comments& other) {
        orginf.set_content_from(other.orginf);
        seqinf.set_content_from(other.seqinf);
        copy_content(others, other.others);
    }
    bool exists() const { return orginf.exists() || seqinf.exists() || has_content(others); }
};

#else
#error rdp_info.h included twice
#endif // RDP_INFO_H

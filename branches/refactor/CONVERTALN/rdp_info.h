#ifndef RDP_INFO_H
#define RDP_INFO_H

// -------------------------------
//      RDP-defined comments (Embl+GenBank)

struct OrgInfo {
    bool  exists;
    char *source;
    char *cultcoll;
    char *formname;
    char *nickname;
    char *commname;
    char *hostorg;

    OrgInfo() {
        exists = false;
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
    OrgInfo(const OrgInfo& other) {
        exists   = other.exists;
        source   = nulldup(other.source);
        cultcoll = nulldup(other.cultcoll);
        formname = nulldup(other.formname);
        nickname = nulldup(other.nickname);
        commname = nulldup(other.commname);
        hostorg  = nulldup(other.hostorg);
    }
    DECLARE_ASSIGNMENT_OPERATOR(OrgInfo);
};

struct SeqInfo {
    bool exists;
    char comp3;  // yes or no, y/n
    char comp5;  // yes or no, y/n

    char *RDPid;
    char *gbkentry;
    char *methods;

    SeqInfo() {
        exists   = false;
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
    SeqInfo(const SeqInfo& other) {
        exists   = other.exists;
        comp3    = other.comp3;
        comp5    = other.comp5;
        RDPid    = nulldup(other.RDPid);
        gbkentry = nulldup(other.gbkentry);
        methods  = nulldup(other.methods);
    }
    DECLARE_ASSIGNMENT_OPERATOR(SeqInfo);
};

struct RDP_comments {
    OrgInfo  orginf;
    SeqInfo  seqinf;
    char    *others;

    RDP_comments() : others(NULL) {}
    RDP_comments(const RDP_comments& from)
        : orginf(from.orginf),
          seqinf(from.seqinf),
          others(nulldup(from.others)) {}
    ~RDP_comments() { freenull(others); }
    DECLARE_ASSIGNMENT_OPERATOR(RDP_comments);
};



#else
#error rdp_info.h included twice
#endif // RDP_INFO_H

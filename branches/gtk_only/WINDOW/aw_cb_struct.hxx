typedef void (*AW_cb_struct_guard)();

/**
 * A list of callback functions.
 */
class AW_cb : virtual Noncopyable {
    WindowCallback cb;

    AW_cb *next;

    static AW_cb_struct_guard guard_before;
    static AW_cb_struct_guard guard_after;
    static AW_postcb_cb       postcb; // called after each cb triggered via interface

public:
    // private (read-only):
    AW_window  *pop_up_window; // only if cb == AW_POPUP: stores result of window creator
    AW_window  *aw;            // parent window
    const char *help_text;
    char       *id;

    // real public section:
    AW_cb(AW_window  *awi,
          AW_CB       g,
          AW_CL       cd1i       = 0,
          AW_CL       cd2i       = 0,
          const char *help_texti = 0,
          AW_cb      *next       = 0);

    AW_cb(AW_window             *awi,
          const WindowCallback&  wcb,
          const char            *help_texti = 0,
          AW_cb                 *next       = 0);

    void run_callbacks();                           // runs the whole list
    bool contains(AW_CB g);                         // test if contained in list
    bool is_equal(const AW_cb& other) const;

    int compare(const AW_cb& other) const { return cb<other.cb ? -1 : (other.cb<cb ? 1 : 0); }

#if defined(ASSERTION_USED)
    // these functions should not be used in production code
    AW_CL get_cd1() const { return cb.inspect_CD1(); }
    AW_CL get_cd2() const { return cb.inspect_CD2(); }
#endif // ASSERTION_USED

    static void set_AW_cb_guards(AW_cb_struct_guard before, AW_cb_struct_guard after) {
        guard_before = before;
        guard_after  = after;
    }
    static void set_AW_postcb_cb(AW_postcb_cb postcb_cb) {
        postcb = postcb_cb;
    }

    static void useraction_init() {
        if (guard_before) guard_before();
    }
    static void useraction_done(AW_window *aw) {
        if (guard_after) guard_after();
        if (postcb) postcb(aw);
    }
};

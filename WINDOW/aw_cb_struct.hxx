typedef void (*AW_cb_struct_guard)();

/**
 * A list of callback functions.
 */
class AW_cb_struct {
    AW_CL         cd1; //callback parameter 2
    AW_CL         cd2; //callback parameter 3
    AW_cb_struct *next;

    static AW_cb_struct_guard guard_before;
    static AW_cb_struct_guard guard_after;
    static AW_postcb_cb       postcb; // called after each cb triggered via interface

public:
    // private (read-only):
    AW_window  *pop_up_window; //callback parameter 1
    AW_CB       f; //the callback function
    AW_window  *aw;
    const char *help_text;
    char       *id;

    // real public section:
    AW_cb_struct(AW_window    *awi,
                 AW_CB         g,
                 AW_CL         cd1i       = 0,
                 AW_CL         cd2i       = 0,
                 const char   *help_texti = 0,
                 AW_cb_struct *next       = 0);

    void run_callback();                            // runs the whole list
    bool contains(AW_CB g);                         // test if contained in list
    bool is_equal(const AW_cb_struct& other) const;

    AW_CL get_cd1() const { return cd1; }
    AW_CL get_cd2() const { return cd2; }

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

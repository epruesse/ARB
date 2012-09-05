// ============================================================== //
//                                                                //
//   File      : NT_branchAnalysis.cxx                            //
//   Purpose   :                                                  //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2012   //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#include <TreeCallbacks.hxx>
#include <aw_awar.hxx>
#include <awt_canvas.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

#define nt_assert(bed) arb_assert(bed)

#define AWAR_BRANCH_ANALYSIS     "branch_analysis"
#define AWAR_BRANCH_ANALYSIS_TMP "tmp/" AWAR_BRANCH_ANALYSIS

#define AWAR_BA_MIN_REL_DIFF AWAR_BRANCH_ANALYSIS "/min_rel_diff"
#define AWAR_BA_MIN_ABS_DIFF AWAR_BRANCH_ANALYSIS "/min_abs_diff"
#define AWAR_BA_MIN_DEPTH    AWAR_BRANCH_ANALYSIS "/min_depth"
#define AWAR_BA_MIN_ROOTDIST AWAR_BRANCH_ANALYSIS "/min_rootdist"
#define AWAR_BA_DEGENERATION AWAR_BRANCH_ANALYSIS "/degeneration"

class BranchWindow : virtual Noncopyable {
    BranchWindow     *next;
    AWT_canvas       *ntw;
    char             *suffix;
    AW_awar          *awar_info;
    AW_window_simple *aws;

    static BranchWindow *head;

    static char *get_suffix(const char *tree_awar_name) { // suffix depends on parent window
        const unsigned  LEN    = 15;
        const char     *EXPECT = "focus/tree_name";

        nt_assert(strlen(EXPECT)                      == LEN);
        nt_assert(memcmp(tree_awar_name, EXPECT, LEN) == 0);

        return strdup(tree_awar_name+LEN);
    }

    const char *local_awar_name (const char *prefix, const char *name) { return GBS_global_string("%s%s/%s", prefix, suffix, name); }

    void create_awars(AW_root *aw_root);
    void create_window(AW_root *aw_root);

public:
    static BranchWindow *find_existing(AWT_canvas *ntw) {
        BranchWindow *bw = head;
        while (bw && bw->ntw != ntw) bw = bw->next;
        return bw;
    }

    BranchWindow(AW_root *aw_root, AWT_canvas *ntw_)
        : next(head),
          ntw(ntw_),
          suffix(get_suffix(ntw->user_awar))
    {
        create_awars(aw_root);
        create_window(aw_root);
        head = this;
    }

    ~BranchWindow() {
        free(suffix);
    }

    AW_window *get_window() const { return aws; }

private:
    AW_root *get_awroot() const { return get_window()->get_root(); }
    AWT_canvas *get_canvas() const { return ntw; }
    AP_tree *get_tree() const { return AWT_TREE(ntw)->get_root_node(); }
    GBDATA *get_gbmain() const { return get_canvas()->gb_main; }
    AW_awar *awar(const char *name) { return get_awroot()->awar(name); }

    void postmark_action() const {
        get_tree()->compute_tree(get_gbmain());
        get_canvas()->refresh();
    }

public:
    void set_info(const char *msg) const { awar_info->write_string(msg); }
    void unmark_all() const { NT_mark_all_cb(get_window(), (AW_CL)get_canvas(), 0); }

    void markDegeneratedBranches() {
        GB_transaction ta(get_gbmain());
        double         degeneration_factor = awar(AWAR_BA_DEGENERATION)->read_float();

        unmark_all();
        set_info(get_tree()->mark_degenerated_branches(degeneration_factor));
        postmark_action();
    }
    void markDeepLeafs() {
        GB_transaction ta(get_gbmain());

        int    min_depth    = awar(AWAR_BA_MIN_DEPTH)->read_int();
        double min_rootdist = awar(AWAR_BA_MIN_ROOTDIST)->read_float();

        unmark_all();
        set_info(get_tree()->mark_deep_leafs(min_depth, min_rootdist));
        postmark_action();
    }
    void markLongBranches() {
        GB_transaction ta(get_gbmain());
        float          min_rel_diff = awar(AWAR_BA_MIN_REL_DIFF)->read_float()/100.0;
        float          min_abs_diff = awar(AWAR_BA_MIN_ABS_DIFF)->read_float();

        unmark_all();
        set_info(get_tree()->mark_long_branches(min_rel_diff, min_abs_diff));
        postmark_action();
    }
};

BranchWindow *BranchWindow::head = NULL;

// --------------------------------------------------------------------------------

static void mark_long_branches_cb       (AW_window *, AW_CL cl_bw) {((BranchWindow*)cl_bw)->markLongBranches(); }
static void mark_deep_leafs_cb          (AW_window *, AW_CL cl_bw) {((BranchWindow*)cl_bw)->markDeepLeafs(); }
static void mark_degenerated_branches_cb(AW_window *, AW_CL cl_bw) {((BranchWindow*)cl_bw)->markDegeneratedBranches(); }
static void unmark_branches_cb          (AW_window *, AW_CL cl_bw) {((BranchWindow*)cl_bw)->unmark_all(); }

static void distance_analysis_cb(AW_window *, AW_CL cl_bw) {
    BranchWindow *bw = (BranchWindow*)cl_bw;
    // @@@ impl missing
}
static void tree_changed_cb(AW_root*, AW_CL cl_bw) {
    BranchWindow *bw = (BranchWindow*)cl_bw;
    bw->set_info("<tree has changed>");
}

void BranchWindow::create_awars(AW_root *aw_root) {
    awar_info = aw_root->awar_string(local_awar_name(AWAR_BRANCH_ANALYSIS_TMP, "info"), "<No analysis performed yet>");
    aw_root->awar(ntw->user_awar)->add_callback(tree_changed_cb, (AW_CL)this);

    if (!head || head == this) {
        aw_root->awar_float(AWAR_BA_MIN_REL_DIFF, 75);
        aw_root->awar_float(AWAR_BA_MIN_ABS_DIFF, 0.01);

        aw_root->awar_int(AWAR_BA_MIN_DEPTH, 0);
        aw_root->awar_float(AWAR_BA_MIN_ROOTDIST, 0.9);

        aw_root->awar_float(AWAR_BA_DEGENERATION, 30);
    }
}

void BranchWindow::create_window(AW_root *aw_root) {
    aws = new AW_window_simple;

    aws->init(aw_root, GBS_global_string("BRANCH_ANALYSIS_%s", suffix), "Branch analysis");
    aws->load_xfig("ad_branch.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"branch_analysis.hlp");
    aws->create_button("HELP", "HELP", "H");

    aws->at("sel");
    aws->create_button(0, ntw->user_awar, 0, "+");

    aws->at("info");
    aws->create_text_field(awar_info->awar_name);

    aws->button_length(28);
    
    aws->sens_mask(AWM_DISABLED);
    aws->at("dist_analyse");
    aws->callback(distance_analysis_cb, (AW_CL)this);
    aws->create_button("ANALYSE", "Analyse distances in tree");
    aws->sens_mask(AWM_ALL);

    aws->at("unmark");
    aws->callback(unmark_branches_cb, (AW_CL)this);
    aws->create_button("UNMARK", "Unmark all species");

    const int WIDTH = 10;

    aws->at("mark_long");
    aws->callback(mark_long_branches_cb, (AW_CL)this);
    aws->create_button("MARK_LONG", "Mark long branches");

    aws->at("min_rel"); aws->create_input_field(AWAR_BA_MIN_REL_DIFF, WIDTH);
    aws->at("min_abs"); aws->create_input_field(AWAR_BA_MIN_ABS_DIFF, WIDTH);


    aws->at("mark_deep");
    aws->callback(mark_deep_leafs_cb, (AW_CL)this);
    aws->create_button("MARK_DEEP", "Mark deep leafs");

    aws->at("tree_depth");   aws->create_input_field(AWAR_BA_MIN_DEPTH, WIDTH);
    aws->at("branch_depth"); aws->create_input_field(AWAR_BA_MIN_ROOTDIST, WIDTH);

    aws->at("mark_degen");
    aws->callback(mark_degenerated_branches_cb, (AW_CL)this);
    aws->create_button("MARK_DEGENERATED", "Mark degenerated branches");

    aws->at("degen"); aws->create_input_field(AWAR_BA_DEGENERATION, WIDTH);
}

AW_window *NT_open_branch_analysis_window(AW_root *aw_root, AW_CL cl_ntw) {
    AWT_canvas   *ntw = (AWT_canvas *)cl_ntw;
    BranchWindow *bw  = BranchWindow::find_existing(ntw);
    if (!bw) bw       = new BranchWindow(aw_root, ntw);
    nt_assert(bw);
    return bw->get_window();
}


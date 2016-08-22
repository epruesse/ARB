// =============================================================== //
//                                                                 //
//   File      : PARS_dtree.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "pars_dtree.hxx"
#include "pars_main.hxx"
#include "pars_debug.hxx"
#include "ap_tree_nlen.hxx"

#include <AP_seq_dna.hxx>
#include <AP_seq_protein.hxx>
#include <AP_filter.hxx>

#include <ColumnStat.hxx>
#include <awt_sel_boxes.hxx>
#include <awt_filter.hxx>

#include <gui_aliview.hxx>

#include <aw_preset.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>
#include <aw_question.hxx>

static void AWT_graphic_parsimony_root_changed(void *cd, AP_tree *old, AP_tree *newroot) {
    AWT_graphic_tree *agt = (AWT_graphic_tree*)cd;

    if (old == agt->displayed_root) agt->displayed_root = newroot;
}

static AliView *pars_generate_aliview(WeightedFilter *pars_weighted_filter) {
    GBDATA *gb_main = pars_weighted_filter->get_gb_main();
    char *ali_name;
    {
        GB_transaction ta(gb_main);
        ali_name = GBT_read_string(gb_main, AWAR_ALIGNMENT);
    }
    GB_ERROR  error   = NULL;
    AliView  *aliview = pars_weighted_filter->create_aliview(ali_name, error);
    if (!aliview) aw_popup_exit(error);
    free(ali_name);
    return aliview;
}

void PARS_tree_init(AWT_graphic_tree *agt) {
    ap_assert(agt->get_root_node());
    ap_assert(agt == ap_main->get_tree_root());

    GB_transaction ta(GLOBAL_gb_main);

    const char *use     = ap_main->get_aliname();
    long        ali_len = GBT_get_alignment_len(GLOBAL_gb_main, use);
    if (ali_len <= 1) {
        aw_popup_exit("No valid alignment selected! Try again");
    }

    agt->tree_static->set_root_changed_callback(AWT_graphic_parsimony_root_changed, agt);
}

static double funktion_quadratisch(double wert, double *param_list, int param_anz) {
    if (param_anz != 3) {
        ap_assert(0); // wrong number of parameters
        return 0;
    }
    return wert * wert * param_list[0] + wert * param_list[1] + param_list[2];
}


void ArbParsimony::kernighan_optimize_tree(AP_tree *at) {
    GB_push_transaction(GLOBAL_gb_main);

    long prevCombineCount = AP_sequence::combine_count();

    AP_FLOAT       pars_start = get_root_node()->costs();
    const AP_FLOAT pars_org   = pars_start;

    int rek_deep_max = *GBT_read_int(GLOBAL_gb_main, "genetic/kh/maxdepth");

    AP_KL_FLAG funktype = (AP_KL_FLAG)*GBT_read_int(GLOBAL_gb_main, "genetic/kh/function_type");

    int param_anz;
    double param_list[3];
    double f_startx, f_maxy, f_maxx, f_max_deep;
    f_max_deep = (double)rek_deep_max;                   //             x2
    f_startx = (double)*GBT_read_int(GLOBAL_gb_main, "genetic/kh/dynamic/start");
    f_maxy = (double)*GBT_read_int(GLOBAL_gb_main, "genetic/kh/dynamic/maxy");
    f_maxx = (double)*GBT_read_int(GLOBAL_gb_main, "genetic/kh/dynamic/maxx");

    double (*funktion)(double wert, double *param_list, int param_anz);
    switch (funktype) {
        default:
        case AP_QUADRAT_START:
            funktion = funktion_quadratisch;
            param_anz = 3;
            param_list[2] = f_startx;
            param_list[0] = (f_startx - f_maxy) / (f_maxx * f_maxx);
            param_list[1] = -2.0 * param_list[0] * f_maxx;
            break;
        case AP_QUADRAT_MAX:    // parameter liste fuer quadratische gleichung (y =ax^2 +bx +c)
            funktion = funktion_quadratisch;
            param_anz = 3;
            param_list[0] =  - f_maxy / ((f_max_deep -  f_maxx) * (f_max_deep - f_maxx));
            param_list[1] =  -2.0 * param_list[0] * f_maxx;
            param_list[2] =  f_maxy  + param_list[0] * f_maxx * f_maxx;
            break;
    }


    AP_KL_FLAG searchflag=(AP_KL_FLAG)0;
    if (*GBT_read_int(GLOBAL_gb_main, "genetic/kh/dynamic/enable")) {
        searchflag = AP_DYNAMIK;
    }
    if (*GBT_read_int(GLOBAL_gb_main, "genetic/kh/static/enable")) {
        searchflag = (AP_KL_FLAG)(searchflag|AP_STATIC);
    }

    int rek_breite[8];
    rek_breite[0] = *GBT_read_int(GLOBAL_gb_main, "genetic/kh/static/depth0");
    rek_breite[1] = *GBT_read_int(GLOBAL_gb_main, "genetic/kh/static/depth1");
    rek_breite[2] = *GBT_read_int(GLOBAL_gb_main, "genetic/kh/static/depth2");
    rek_breite[3] = *GBT_read_int(GLOBAL_gb_main, "genetic/kh/static/depth3");
    rek_breite[4] = *GBT_read_int(GLOBAL_gb_main, "genetic/kh/static/depth4");
    int rek_breite_anz = 5;

    int       anzahl = (int)(*GBT_read_float(GLOBAL_gb_main, "genetic/kh/nodes")*at->count_leafs());
    AP_tree **list   = at->getRandomNodes(anzahl);
    
    arb_progress progress(anzahl);

    progress.subtitle(GBS_global_string("Old Parsimony: %.1f", pars_start));

    GB_pop_transaction(GLOBAL_gb_main);

    for (int i=0; i<anzahl && !progress.aborted(); i++) {
        AP_tree_nlen *tree_elem = DOWNCAST(AP_tree_nlen*, list[i]); // @@@ pass 'at' as AP_tree_nlen

        bool in_folded_group = tree_elem->gr.hidden ||
            (tree_elem->father && tree_elem->get_father()->gr.hidden);

        if (!in_folded_group) {
            bool better_tree_found = false;
            ap_main->push();
            display_clear(funktion, param_list, param_anz, (int)pars_start, (int)rek_deep_max);

            tree_elem->kernighan_rek(0,
                                     rek_breite, rek_breite_anz, rek_deep_max,
                                     funktion, param_list, param_anz,
                                     pars_start,  pars_start, pars_org,
                                     searchflag, &better_tree_found);

            if (better_tree_found) {
                ap_main->clear();
                pars_start =  get_root_node()->costs();
                progress.subtitle(GBS_global_string("New parsimony: %.1f (gain: %.1f)", pars_start, pars_org-pars_start));
            }
            else {
                ap_main->pop();
            }
        }
        progress.inc();
    }
    delete list;
    printf("Combines: %li\n", AP_sequence::combine_count()-prevCombineCount);
}



void ArbParsimony::optimize_tree(AP_tree *at, arb_progress& progress) {
    AP_tree        *oldrootleft  = get_root_node()->get_leftson();
    AP_tree        *oldrootright = get_root_node()->get_rightson();
    const AP_FLOAT  org_pars     = get_root_node()->costs();
    AP_FLOAT        prev_pars    = org_pars;

    progress.subtitle(GBS_global_string("Old parsimony: %.1f", org_pars));

    while (!progress.aborted()) {
        AP_FLOAT nni_pars = DOWNCAST(AP_tree_nlen*, at)->nn_interchange_rek(-1, AP_BL_NNI_ONLY, false);

        if (nni_pars == prev_pars) { // NNI did not reduce costs -> kern-lin
            kernighan_optimize_tree(at);
            AP_FLOAT ker_pars = get_root_node()->costs();
            if (ker_pars == prev_pars) break; // kern-lin did not improve tree -> done
            prev_pars = ker_pars;
        }
        else {
            prev_pars = nni_pars;
        }
        progress.subtitle(GBS_global_string("New parsimony: %.1f (gain: %.1f)", prev_pars, org_pars-prev_pars));
    }

    if (oldrootleft->father == oldrootright) oldrootleft->set_root();
    else oldrootright->set_root();

    get_root_node()->costs();
}

AWT_graphic_parsimony::AWT_graphic_parsimony(ArbParsimony& parsimony_, GBDATA *gb_main_, AD_map_viewer_cb map_viewer_cb_)
    : AWT_graphic_tree(parsimony_.get_awroot(), gb_main_, map_viewer_cb_),
      parsimony(parsimony_)
{}

void ArbParsimony::generate_tree(WeightedFilter *pars_weighted_filter) {
    AliView     *aliview   = pars_generate_aliview(pars_weighted_filter);
    AP_sequence *seq_templ = 0;

    GBDATA *gb_main = aliview->get_gb_main();
    {
        GB_transaction ta(gb_main);
        bool           is_aa = GBT_is_alignment_protein(gb_main, aliview->get_aliname());

        if (is_aa) seq_templ = new AP_sequence_protein(aliview);
        else seq_templ       = new AP_sequence_parsimony(aliview);
    }

    tree = new AWT_graphic_parsimony(*this, aliview->get_gb_main(), PARS_map_viewer);
    tree->init(new AP_TreeNlenNodeFactory, aliview, seq_templ, true, false);
    ap_main->set_tree_root(tree);
}

AW_gc_manager AWT_graphic_parsimony::init_devices(AW_window *aww, AW_device *device, AWT_canvas* ntw) {
    AW_init_color_group_defaults("arb_pars");

    AW_gc_manager gc_manager =
        AW_manage_GC(aww,
                     ntw->get_gc_base_name(),
                     device, AWT_GC_CURSOR, AWT_GC_MAX, /* AWT_GC_CURSOR+7, */ AW_GCM_DATA_AREA,
                     makeWindowCallback(AWT_resize_cb, ntw),
                     true,      // uses color groups
                     "#AAAA55",

                     // Important note :
                     // Many gc indices are shared between ABR_NTREE and ARB_PARSIMONY
                     // e.g. the tree drawing routines use same gc's for drawing both trees
                     // (check AWT_dtree.cxx AWT_graphic_tree::init_devices)

                     "Cursor$#FFFFFF",
                     "Branch remarks$#DBE994",
                     "+-Bootstrap$#DBE994",    "-B.(limited)$white",
                     "--unused$#ff0000",
                     "Marked$#FFFF00",
                     "Some marked$#eeee88",
                     "Not marked$black",
                     "Zombies etc.$#cc5924",

                     "--unused", "--unused", // these reserve the numbers which are used for probe colors in ARB_NTREE
                     "--unused", "--unused", // (this is necessary because ARB_PARS and ARB_NTREE use the same tree painting routines)
                     "--unused", "--unused",
                     "--unused", "--unused",

                     NULL);
    return gc_manager;
}

void AWT_graphic_parsimony::show(AW_device *device) {
    AP_tree_nlen *root_node = parsimony.get_root_node();
    AW_awar      *awar_pars = aw_root->awar(AWAR_PARSIMONY);
    AW_awar      *awar_best = aw_root->awar(AWAR_BEST_PARSIMONY);

    long parsval = root_node ? root_node->costs() : 0;
    awar_pars->write_int(parsval);

    long best_pars = awar_best->read_int();
    if (parsval < best_pars || 0==best_pars) awar_best->write_int(parsval);

    AWT_graphic_tree::show(device);
}

void AWT_graphic_parsimony::handle_command(AW_device *device, AWT_graphic_event& event) {
    ClickedTarget clicked(this, event.best_click());
    bool          recalc_branchlengths_on_structure_change = true;

    switch (event.cmd()) {
        // @@@ something is designed completely wrong here!
        // why do all commands close TA and reopen when done?

        case AWT_MODE_NNI:
            if (event.type()==AW_Mouse_Press) {
                GB_pop_transaction(gb_main);
                switch (event.button()) {
                    case AW_BUTTON_LEFT: {
                        if (clicked.node()) {
                            arb_progress  progress("NNI optimize subtree");
                            AP_tree_nlen *atn = DOWNCAST(AP_tree_nlen*, clicked.node());
                            atn->nn_interchange_rek(-1, AP_BL_NNI_ONLY, false);
                            exports.save = 1;
                            ASSERT_VALID_TREE(get_root_node());
                        }
                        break;
                    }
                    case AW_BUTTON_RIGHT: {
                        arb_progress progress("NNI optimize tree");
                        long         prevCombineCount = AP_sequence::combine_count();
                        
                        AP_tree_nlen *atn = DOWNCAST(AP_tree_nlen*, get_root_node());
                        atn->nn_interchange_rek(-1, AP_BL_NNI_ONLY, false);
                        printf("Combines: %li\n", AP_sequence::combine_count()-prevCombineCount);

                        exports.save = 1;
                        ASSERT_VALID_TREE(get_root_node());
                        break;
                    }

                    default: break;
                }
                GB_begin_transaction(gb_main);
            }
            break;
        case AWT_MODE_KERNINGHAN:
            if (event.type()==AW_Mouse_Press) {
                GB_pop_transaction(gb_main);
                switch (event.button()) {
                    case AW_BUTTON_LEFT:
                        if (clicked.node()) {
                            arb_progress  progress("Kernighan-Lin optimize subtree");
                            parsimony.kernighan_optimize_tree(clicked.node());
                            this->exports.save = 1;
                            ASSERT_VALID_TREE(get_root_node());
                        }
                        break;
                    case AW_BUTTON_RIGHT: {
                        arb_progress progress("Kernighan-Lin optimize tree");
                        parsimony.kernighan_optimize_tree(get_root_node());
                        this->exports.save = 1;
                        ASSERT_VALID_TREE(get_root_node());
                        break;
                    }
                    default: break;
                }
                GB_begin_transaction(gb_main);
            }
            break;
        case AWT_MODE_OPTIMIZE:
            if (event.type()==AW_Mouse_Press) {
                GB_pop_transaction(gb_main);
                switch (event.button()) {
                    case AW_BUTTON_LEFT:
                        if (clicked.node()) {
                            arb_progress  progress("Optimizing subtree");
                            parsimony.optimize_tree(clicked.node(), progress);
                            this->exports.save = 1;
                            ASSERT_VALID_TREE(get_root_node());
                        }
                        break;
                    case AW_BUTTON_RIGHT: {
                        arb_progress progress("Optimizing tree");
                        
                        parsimony.optimize_tree(get_root_node(), progress);
                        this->exports.save = 1;
                        ASSERT_VALID_TREE(get_root_node());
                        break;
                    }
                    default: break;
                }
                GB_begin_transaction(gb_main);
            }
            break;

        default:
            recalc_branchlengths_on_structure_change = false;
            // fall-through (modes listed below trigger branchlength calculation)
        case AWT_MODE_MOVE:
            AWT_graphic_tree::handle_command(device, event);
            break;
    }

    if (exports.save == 1 && recalc_branchlengths_on_structure_change) {
        arb_progress progress("Recalculating branch lengths");
        rootEdge()->calc_branchlengths();
        reorder_tree(BIG_BRANCHES_TO_TOP); // beautify after recalc_branch_lengths
    }
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <arb_diff.h>
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void fake_AW_init_color_groups();

struct fake_agt : public AWT_graphic_tree, virtual Noncopyable {
    AP_sequence_parsimony *templ;

    fake_agt()
        : AWT_graphic_tree(NULL, GLOBAL_gb_main, NULL),
          templ(NULL)
    {
    }
    ~fake_agt() {
        delete templ;
    }
    void init(AliView *aliview) {
        fake_AW_init_color_groups(); // acts like no species has a color
        delete templ;
        templ = aliview->has_data() ? new AP_sequence_parsimony(aliview) : NULL;
        AWT_graphic_tree::init(new AP_TreeNlenNodeFactory, aliview, templ, true, false);
    }
};

class PARSIMONY_testenv : virtual Noncopyable {
    GB_shell  shell;
    AP_main   apMain;
    fake_agt *agt;

    void common_init(const char *dbname) {
        GLOBAL_gb_main = NULL;
        apMain.open(dbname);

        TEST_EXPECT_NULL(ap_main);
        ap_main = &apMain;

        agt = new fake_agt;
        apMain.set_tree_root(agt);
    }

public:
    PARSIMONY_testenv(const char *dbname) {
        common_init(dbname);
        agt->init(new AliView(GLOBAL_gb_main));
    }
    PARSIMONY_testenv(const char *dbname, const char *aliName) {
        common_init(dbname);
        GB_transaction ta(GLOBAL_gb_main);
        size_t aliLength = GBT_get_alignment_len(GLOBAL_gb_main, aliName);

        AP_filter filter(aliLength);
        if (!filter.is_invalid()) {
            AP_weights weights(&filter);
            agt->init(new AliView(GLOBAL_gb_main, filter, weights, aliName));
        }
    }
    ~PARSIMONY_testenv() {
        TEST_EXPECT_EQUAL(ap_main, &apMain);
        ap_main = NULL;

        delete agt;
        GB_close(GLOBAL_gb_main);
        GLOBAL_gb_main = NULL;
    }

    GB_ERROR load_tree(const char *tree_name) {
        GB_transaction ta(GLOBAL_gb_main); // @@@ do inside AWT_graphic_tree::load?
        return agt->load(GLOBAL_gb_main, tree_name, 0, 0);
    }
    AP_tree_nlen *tree_root() { return apMain.get_root_node(); }

    void push() { apMain.push(); }
    void pop() { apMain.pop(); }
};

void TEST_tree_modifications() {
    PARSIMONY_testenv env("TEST_trees.arb");
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_test"));

    {
        AP_tree_nlen *root = env.tree_root();
        TEST_REJECT_NULL(root);

        AP_tree_edge::initialize(root);   // builds edges
        TEST_ASSERT_VALID_TREE(root);

        root->compute_tree();

        // first check initial state:
        {
            AP_tree_members& root_info = root->gr;

            TEST_EXPECT_EQUAL(root_info.grouped,             0);
            TEST_EXPECT_EQUAL(root_info.hidden,              0);
            TEST_EXPECT_EQUAL(root_info.has_marked_children, 1);
            TEST_EXPECT_EQUAL(root_info.leaf_sum,            15);

            TEST_EXPECT_SIMILAR(root_info.max_tree_depth, 1.624975, 0.000001);
            TEST_EXPECT_SIMILAR(root_info.min_tree_depth, 0.341681, 0.000001);

            GB_transaction ta(GLOBAL_gb_main);
            GBT_mark_all(GLOBAL_gb_main, 0); // unmark all species
            root->compute_tree();
            TEST_EXPECT_EQUAL(root_info.has_marked_children, 0);
        }


#define B1_TOP "(((((CloTyro3:1.046,CloTyro4:0.061):0.026,CloTyro2:0.017):0.017,CloTyrob:0.009):0.274,CloInnoc:0.371):0.057,CloBifer:0.388):0.124"
#define B1_BOT "(CloBifer:0.388,(CloInnoc:0.371,(CloTyrob:0.009,(CloTyro2:0.017,(CloTyro3:1.046,CloTyro4:0.061):0.026):0.017):0.274):0.057):0.124"
#define B2_TOP "(((CloButy2:0.009,CloButyr:0.000):0.564,CloCarni:0.120):0.010,CloPaste:0.179):0.131"
#define B2_BOT "(CloPaste:0.179,(CloCarni:0.120,(CloButy2:0.009,CloButyr:0.000):0.564):0.010):0.131"


#define B3_LEFT_TOP_SONS "(((CorAquat:0.084,CurCitre:0.058):0.103,CorGluta:0.522):0.053,CelBiazo:0.059)"
#define B3_TOP_SONS      B3_LEFT_TOP_SONS ":0.207,CytAquat:0.711"
#define B3_TOP_SONS_CCR  "((CorAquat:0.187,CorGluta:0.522):0.053,CelBiazo:0.059):0.207,CytAquat:0.711" // CCR = CurCitre removed
#define B3_TOP           "(" B3_TOP_SONS "):0.081"
#define B3_BOT           "(CytAquat:0.711,(CelBiazo:0.059,(CorGluta:0.522,(CorAquat:0.084,CurCitre:0.058):0.103):0.053):0.207):0.081"


        const char *top_topo    = "((" B1_TOP "," B2_TOP "):0.081," B3_TOP ");";
        const char *edge_topo   = "((" B1_TOP "," B2_BOT "):0.081," B3_BOT ");";
        const char *bottom_topo = "(" B3_BOT ",(" B2_BOT "," B1_BOT "):0.081);";

        const char *radial_topo  =
            "(((CloPaste:0.179,((CloButy2:0.009,CloButyr:0.000):0.564,CloCarni:0.120):0.010):0.131,"
            "((CloInnoc:0.371,((CloTyro2:0.017,(CloTyro3:1.046,CloTyro4:0.061):0.026):0.017,CloTyrob:0.009):0.274):0.057,CloBifer:0.388):0.124):0.081,"
            "((CelBiazo:0.059,((CorAquat:0.084,CurCitre:0.058):0.103,CorGluta:0.522):0.053):0.207,CytAquat:0.711):0.081);";
        const char *radial_topo2 =
            "(((CloBifer:0.388,(CloInnoc:0.371,(((CloTyro3:1.046,CloTyro4:0.061):0.026,CloTyro2:0.017):0.017,CloTyrob:0.009):0.274):0.057):0.124," B2_TOP "):0.081,"
            "(CytAquat:0.711," B3_LEFT_TOP_SONS ":0.207):0.081);";

        // expect that no mode reproduces another mode:
        TEST_EXPECT_DIFFERENT(top_topo,    edge_topo);
        TEST_EXPECT_DIFFERENT(top_topo,    bottom_topo);
        TEST_EXPECT_DIFFERENT(top_topo,    radial_topo);
        TEST_EXPECT_DIFFERENT(top_topo,    radial_topo2);
        TEST_EXPECT_DIFFERENT(edge_topo,   bottom_topo);
        TEST_EXPECT_DIFFERENT(edge_topo,   radial_topo);
        TEST_EXPECT_DIFFERENT(edge_topo,   radial_topo2);
        TEST_EXPECT_DIFFERENT(bottom_topo, radial_topo);
        TEST_EXPECT_DIFFERENT(bottom_topo, radial_topo2);
        TEST_EXPECT_DIFFERENT(radial_topo, radial_topo2);

        env.push(); // 1st stack level (=top_topo)

        TEST_ASSERT_VALID_TREE(root);

        TEST_EXPECT_NEWICK(nLENGTH, root, top_topo);
        // test reorder_tree:
        root->reorder_tree(BIG_BRANCHES_TO_EDGE);     TEST_EXPECT_NEWICK(nLENGTH, root, edge_topo);    env.push(); // 2nd stack level (=edge_topo)
        root->reorder_tree(BIG_BRANCHES_TO_BOTTOM);   TEST_EXPECT_NEWICK(nLENGTH, root, bottom_topo);  env.push(); // 3rd stack level (=bottom_topo)
        root->reorder_tree(BIG_BRANCHES_TO_CENTER);   TEST_EXPECT_NEWICK(nLENGTH, root, radial_topo);
        root->reorder_tree(BIG_BRANCHES_ALTERNATING); TEST_EXPECT_NEWICK(nLENGTH, root, radial_topo2);
        root->reorder_tree(BIG_BRANCHES_TO_TOP);      TEST_EXPECT_NEWICK(nLENGTH, root, top_topo);

        TEST_ASSERT_VALID_TREE(root);

        // test set root:
        AP_tree_nlen *CloTyrob = root->findLeafNamed("CloTyrob");
        TEST_REJECT_NULL(CloTyrob);

        ARB_edge rootEdge(root->get_leftson(), root->get_rightson());
        CloTyrob->set_root();

        TEST_ASSERT_VALID_TREE(root);

        const char *rootAtCloTyrob_topo =
            "(CloTyrob:0.004,"
            "(((CloTyro3:1.046,CloTyro4:0.061):0.026,CloTyro2:0.017):0.017,"
            "((((" B3_TOP_SONS "):0.162," B2_TOP "):0.124,CloBifer:0.388):0.057,CloInnoc:0.371):0.274):0.004);";

        TEST_EXPECT_NEWICK(nLENGTH, root, rootAtCloTyrob_topo);
        env.push(); // 4th stack level (=rootAtCloTyrob_topo)

        TEST_ASSERT_VALID_TREE(root);

        AP_tree_nlen *CelBiazoFather = root->findLeafNamed("CelBiazo")->get_father();
        TEST_REJECT_NULL(CelBiazoFather);
        CelBiazoFather->set_root();

        const char *rootAtCelBiazoFather_topo = "(" B3_LEFT_TOP_SONS ":0.104,((" B1_TOP "," B2_TOP "):0.162,CytAquat:0.711):0.104);";
        TEST_EXPECT_NEWICK(nLENGTH, root, rootAtCelBiazoFather_topo);

        TEST_ASSERT_VALID_TREE(root);

        ARB_edge oldRootEdge(rootEdge.source(), rootEdge.dest());
        DOWNCAST(AP_tree_nlen*,oldRootEdge.son())->set_root();

        const char *rootSetBack_topo = top_topo;
        TEST_EXPECT_NEWICK(nLENGTH, root, rootSetBack_topo);
        env.push(); // 5th stack level (=rootSetBack_topo)

        TEST_ASSERT_VALID_TREE(root);

        // test remove:
        AP_tree_nlen *CurCitre = root->findLeafNamed("CurCitre");
        TEST_REJECT_NULL(CurCitre);
        TEST_REJECT_NULL(CurCitre->get_father());

        CurCitre->remove();
        const char *CurCitre_removed_topo = "((" B1_TOP "," B2_TOP "):0.081,(" B3_TOP_SONS_CCR "):0.081);";
        // ------------------------------------------------------------------- ^^^ = B3_TOP_SONS minus CurCitre
        TEST_EXPECT_NEWICK(nLENGTH, root, CurCitre_removed_topo);

        TEST_ASSERT_VALID_TREE(root);
        TEST_ASSERT_VALID_TREE(CurCitre);

        TEST_EXPECT_EQUAL(root->gr.leaf_sum, 15); // out of date
        root->compute_tree();
        TEST_EXPECT_EQUAL(root->gr.leaf_sum, 14);

        env.push(); // 6th stack level (=CurCitre_removed_topo)

        TEST_ASSERT_VALID_TREE(root);

        // test insert:
        AP_tree_nlen *CloCarni = root->findLeafNamed("CloCarni");
        TEST_REJECT_NULL(CloCarni);
        CurCitre->insert(CloCarni); // this creates two extra edges (not destroyed by destroy() below) and one extra node

        const char *CurCitre_inserted_topo = "((" B1_TOP ",(((CloButy2:0.009,CloButyr:0.000):0.564,(CurCitre:0.060,CloCarni:0.060):0.060):0.010,CloPaste:0.179):0.131):0.081,(" B3_TOP_SONS_CCR "):0.081);";
        TEST_EXPECT_NEWICK(nLENGTH, root, CurCitre_inserted_topo);

        AP_tree_nlen *node_del_manually  = CurCitre->get_father();
        AP_tree_edge *edge1_del_manually = CurCitre->edgeTo(node_del_manually);
        AP_tree_edge *edge2_del_manually = CurCitre->get_brother()->edgeTo(node_del_manually);

        TEST_ASSERT_VALID_TREE(root);

        // now check pops:
        env.pop(); TEST_EXPECT_NEWICK(nLENGTH, root, CurCitre_removed_topo);
        env.pop(); TEST_EXPECT_NEWICK(nLENGTH, root, rootSetBack_topo);
        env.pop(); TEST_EXPECT_NEWICK(nLENGTH, root, rootAtCloTyrob_topo);
        env.pop(); TEST_EXPECT_NEWICK(nLENGTH, root, bottom_topo);
        env.pop(); TEST_EXPECT_NEWICK(nLENGTH, root, edge_topo);
        env.pop(); TEST_EXPECT_NEWICK(nLENGTH, root, top_topo);

        TEST_ASSERT_VALID_TREE(root);

        AP_tree_edge::destroy(root);

        // delete memory allocated by insert() above and lost due to pop()s
        delete edge1_del_manually;
        delete edge2_del_manually;

        node_del_manually->forget_origin();
        node_del_manually->father   = NULL;
        node_del_manually->leftson  = NULL;
        node_del_manually->rightson = NULL;
        delete node_del_manually;
    }
}

// @@@ Tests wanted:
// - NNI
// - tree optimize
// - ...

void TEST_calc_bootstraps() {
    PARSIMONY_testenv env("TEST_trees.arb", "ali_5s");
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_test"));

    const char *bs_origi_topo = "(((((((CloTyro3,CloTyro4)'40%',CloTyro2)'0%',CloTyrob)'97%',CloInnoc)'0%',CloBifer)'53%',(((CloButy2,CloButyr)'100%',CloCarni)'33%',CloPaste)'97%')'100%',((((CorAquat,CurCitre)'100%',CorGluta)'17%',CelBiazo)'40%',CytAquat)'100%');";
    const char *bs_limit_topo = "(((((((CloTyro3,CloTyro4)'87%',CloTyro2)'0%',CloTyrob)'100%',CloInnoc)'87%',CloBifer)'83%',(((CloButy2,CloButyr)'99%',CloCarni)'17%',CloPaste)'56%')'61%',((((CorAquat,CurCitre)'78%',CorGluta)'0%',CelBiazo)'59%',CytAquat)'61%');";
    const char *bs_estim_topo = "(((((((CloTyro3,CloTyro4)'75%',CloTyro2)'0%',CloTyrob)'100%',CloInnoc)'75%',CloBifer)'78%',(((CloButy2,CloButyr)'99%',CloCarni)'13%',CloPaste)'32%')'53%',((((CorAquat,CurCitre)'74%',CorGluta)'0%',CelBiazo)'56%',CytAquat)'53%');";

    {
        AP_tree_nlen *root = env.tree_root();
        TEST_REJECT_NULL(root);

        AP_tree_edge::initialize(root);   // builds edges
        TEST_EXPECT_EQUAL(root, rootNode()); // need tree-access via global 'ap_main' (too much code is based on that)

        AP_tree_edge *root_edge = rootEdge();
        TEST_REJECT_NULL(root_edge);

        root->reorder_tree(BIG_BRANCHES_TO_TOP); TEST_EXPECT_NEWICK(nREMARK, root, bs_origi_topo);

        root_edge->nni_rek(-1, false, AP_BL_MODE(AP_BL_BL_ONLY|AP_BL_BOOTSTRAP_LIMIT),    NULL); root->reorder_tree(BIG_BRANCHES_TO_TOP); TEST_EXPECT_NEWICK(nREMARK, root, bs_limit_topo);
        root_edge->nni_rek(-1, false, AP_BL_MODE(AP_BL_BL_ONLY|AP_BL_BOOTSTRAP_ESTIMATE), NULL); root->reorder_tree(BIG_BRANCHES_TO_TOP); TEST_EXPECT_NEWICK(nREMARK, root, bs_estim_topo);

        TEST_EXPECT_EQUAL(env.tree_root(), root);
        AP_tree_edge::destroy(root);
    }
}

void TEST_tree_remove_add_all() {
    // reproduces crash as described in #527
    PARSIMONY_testenv env("TEST_trees.arb", "ali_5s");
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_nj"));

    const int     LEAFS     = 6;
    AP_tree_nlen *leaf[LEAFS];
    const char *name[LEAFS] = {
        "CloButy2",
        "CloButyr",
        "CytAquat",
        "CorAquat",
        "CurCitre",
        "CorGluta",
    };

    AP_tree_nlen *root = env.tree_root();
    TEST_REJECT_NULL(root);

    for (int i = 0; i<LEAFS; ++i) {
        leaf[i] = root->findLeafNamed(name[i]);
        TEST_REJECT_NULL(leaf[i]);
    }

    AP_tree_edge::initialize(root);   // builds edges
    TEST_EXPECT_EQUAL(root, rootNode()); // need tree-access via global 'ap_main' (too much code is based on that)

    AP_tree_root *troot = leaf[0]->get_tree_root();
    TEST_REJECT_NULL(troot);

    // Note: following loop leaks father nodes and edges
    for (int i = 0; i<LEAFS-1; ++i) { // removing the second to last leaf, "removes" both remaining leafs
        TEST_ASSERT_VALID_TREE(root);
        leaf[i]->remove();
        TEST_ASSERT_VALID_TREE(leaf[i]);
    }
    leaf[LEAFS-1]->father = NULL; // correct final leaf (not removed regularily)

    leaf[0]->initial_insert(leaf[1], troot);
    for (int i = 2; i<LEAFS; ++i) {
        TEST_ASSERT_VALID_TREE(leaf[i-1]);
        TEST_ASSERT_VALID_TREE(leaf[i]);
        leaf[i]->insert(leaf[i-1]);
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

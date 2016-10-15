// =============================================================== //
//                                                                 //
//   File      : AWT_TreeAwars.cxx                                 //
//   Purpose   : tree awar registry                                //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in January 2014   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "awt_TreeAwars.hxx"

#include <arbdbt.h>
#include <ad_cb.h>

#include <arb_global_defs.h>

#include <aw_awar.hxx>
#include <aw_root.hxx>
#include <aw_global_awars.hxx>

#include <set>
#include <cstddef>

#define awt_assert(cond) arb_assert(cond)

/*! bind a callback to a tree-awar and the associated tree
 * i.e. callback is triggered if awar changes to different tree
 * or if tree itself changes.
 *
 * The awar has to be type string and has to contain a tree name
 */
class BoundTreeAwarCallback : virtual Noncopyable {
    mutable AW_awar  *awar;
    GBDATA           *gb_tree;
    TreeAwarCallback  cb;
    mutable bool      triggerOnDataChange;

public:
    BoundTreeAwarCallback(AW_awar *awar_, const TreeAwarCallback& cb_, bool triggerIfTreeDataChanges);
    ~BoundTreeAwarCallback();

    void bind_tree_callback();
    void call(bool treeDataChanged) { cb(awar, treeDataChanged); }

    void rename_if(const char *oldname, const char *newname) const {
        if (strcmp(awar->read_char_pntr(), oldname) == 0) {
            awar->write_string(newname);
        }
    }

    bool is_less_than(const BoundTreeAwarCallback& other) const {
        ptrdiff_t diff = (const char*)awar - (const char*)other.awar;
        return diff<0;
    }

    void tree_lost() { gb_tree = NULL; }
};

typedef SmartPtr<BoundTreeAwarCallback> BoundTreeAwarCallbackPtr;

inline bool operator<(const BoundTreeAwarCallbackPtr& bc1, const BoundTreeAwarCallbackPtr& bc2) {
    return bc1->is_less_than(*bc2);
}

typedef std::set<BoundTreeAwarCallbackPtr> BoundTreeAwarCallbacks;

/*! registry for TreeAwars
 */
class TreeAwarRegistry : virtual Noncopyable {
    GBDATA                 *gb_main;
    BoundTreeAwarCallbacks  callbacks;

public:
    TreeAwarRegistry(GBDATA *gb_main_);

    static SmartPtr<TreeAwarRegistry> SINGLETON;

    GBDATA *get_gb_main() { return gb_main; }
    void add(BoundTreeAwarCallbackPtr bcb) { callbacks.insert(bcb); }

    void tree_renamed(const char *oldname, const char *newname);
};

SmartPtr<TreeAwarRegistry> TreeAwarRegistry::SINGLETON;

static void destroy_TreeAwarRegistry(GBDATA*,void*) {
    TreeAwarRegistry::SINGLETON.SetNull();
}

// --------------------------
//      BoundTreeAwarCallback

static void TreeDataChanged_cb(UNFIXED, BoundTreeAwarCallback *tac, GB_CB_TYPE cbtype) {
    if (cbtype == GB_CB_DELETE) {
        tac->tree_lost();
    }
    tac->call(true);
}
static void TreeAwarChanged_cb(UNFIXED, BoundTreeAwarCallback *tac) {
    tac->bind_tree_callback();
    tac->call(false);
}

void BoundTreeAwarCallback::bind_tree_callback() {
    if (triggerOnDataChange) {
        DatabaseCallback dbcb = makeDatabaseCallback(TreeDataChanged_cb, this);
        {
            GBDATA         *gb_main = TreeAwarRegistry::SINGLETON->get_gb_main();
            GB_transaction  ta(gb_main);
            if (gb_tree) {
                GB_remove_callback(gb_tree, GB_CB_CHANGED_OR_DELETED, dbcb);
                gb_tree = NULL;
            }

            char *treename = awar->read_string();
            gb_tree        = GBT_find_tree(gb_main, treename);

            if (gb_tree) GB_add_callback(gb_tree, GB_CB_CHANGED_OR_DELETED, dbcb);
            free(treename);
        }
    }
}

BoundTreeAwarCallback::BoundTreeAwarCallback(AW_awar *awar_, const TreeAwarCallback& cb_, bool triggerIfTreeDataChanges)
    : awar(awar_),
      gb_tree(NULL),
      cb(cb_),
      triggerOnDataChange(triggerIfTreeDataChanges)
{
    awar->add_callback(makeRootCallback(TreeAwarChanged_cb, this));
    bind_tree_callback();
}
BoundTreeAwarCallback::~BoundTreeAwarCallback() {
    awar->remove_callback(makeRootCallback(TreeAwarChanged_cb, this));

    // remove DB callback w/o triggering it:
    char *old = awar->read_string();
    awar->write_string("");
    bind_tree_callback();
    awar->write_string(old);
    free(old);
}

// --------------------------
//      TreeAwarRegistry

static void tree_renamed_cb(AW_root *aw_root) {
    AW_awar *awar_renamed = aw_root->awar(AWAR_ARB_TREE_RENAMED);
    char    *name_change  = awar_renamed->read_string();

    if (name_change[0]) {
        char     *equal   = strchr(name_change, '=');
        GB_ERROR  error   = NULL;
        if (!equal) error = "Expected '=' missing";
        else {
            equal[0] = 0;

            const char *oldname = name_change;
            const char *newname = equal+1;
            TreeAwarRegistry::SINGLETON->tree_renamed(oldname, newname);
        }

        if (error) {
            fprintf(stderr, "Warning: Cannot handle invalid tree-name-change ('%s'; %s)",
                    awar_renamed->read_char_pntr(), error);
        }
    }
}

TreeAwarRegistry::TreeAwarRegistry(GBDATA *gb_main_)
    : gb_main(gb_main_)
{
    // uses global awar AWAR_ARB_TREE_RENAMED to synchronize between multiple ARB apps
    AW_root::SINGLETON->awar(AWAR_ARB_TREE_RENAMED)->add_callback(tree_renamed_cb);
    GB_atclose(gb_main_, destroy_TreeAwarRegistry, NULL);
}

void TreeAwarRegistry::tree_renamed(const char *oldname, const char *newname) {
    for (BoundTreeAwarCallbacks::iterator bcb = callbacks.begin(); bcb != callbacks.end(); ++bcb) {
        (*bcb)->rename_if(oldname, newname);
    }
}

// -------------------
//      interface

void AWT_initTreeAwarRegistry(GBDATA *gbmain) {
    /*! initialize TreeAwarRegistry.
     *
     * Has to be called after ARB_init_global_awars().
     * Will be destroyed on GB_close().
     *
     * Allows to use other functions provided by this module.
     */
    awt_assert(gbmain);
    if (TreeAwarRegistry::SINGLETON.isSet()) {
        if (TreeAwarRegistry::SINGLETON->get_gb_main() != gbmain) {
            GBK_terminate("double init of TreeAwarRegistry with different gbmain");
        }
    }
    else {
        TreeAwarRegistry::SINGLETON = new TreeAwarRegistry(gbmain);
    }
}

void AWT_registerTreeAwarCallback(AW_awar *awar, const TreeAwarCallback& tacb, bool triggerIfTreeDataChanges) {
    /*! bind and register TreeAwarCallback
     *
     * Signature of TreeAwarCallback is 'void cb(AW_awar *awar, bool treeDataChanged, ...)'
     * 'treeDataChanged' is true, if the callback is triggered by a change of the tree-data
     * 'treeDataChanged' is false, if the callback is triggered by a change of the awar
     *
     * if 'triggerIfTreeDataChanges' is false, the callback will not trigger if tree-data changes
     */

    TreeAwarRegistry::SINGLETON->add(new BoundTreeAwarCallback(awar, tacb, triggerIfTreeDataChanges));
}

static void null_cb() {}
void AWT_registerTreeAwarSimple(AW_awar *awar) {
    /*! just register a tree awar, w/o any client cb.
     * -> awar will just automatically follow tree-renames
     */
    AWT_registerTreeAwarCallback(awar, makeTreeAwarCallback(null_cb), false);
}

static void announce_renamed(const char *oldname, const char *newname) {
    AW_awar *awar_renamed = AW_root::SINGLETON->awar(AWAR_ARB_TREE_RENAMED);
    awar_renamed->write_string(GBS_global_string("%s=%s", oldname, newname)); // triggers tree_renamed_cb (in this and all other ARB apps)
}

void AWT_announce_tree_renamed(const char *oldname, const char *newname) { announce_renamed(oldname, newname); }
void AWT_announce_tree_deleted(const char *name)                         { announce_renamed(name,    NO_TREE_SELECTED); }


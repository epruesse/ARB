// ============================================================ //
//                                                              //
//   File      : tree_position.h                                //
//   Purpose   : provides relative position in tree             //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2016   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef TREE_POSITION_H
#define TREE_POSITION_H

#ifndef _GLIBCXX_MAP
#include <map>
#endif
#ifndef _GLIBCXX_STRING
#include <string>
#endif
#ifndef TREENODE_H
#include <TreeNode.h>
#endif
#ifndef NT_LOCAL_H
#include "NT_local.h"
#endif

class TreeRelativePosition {
    // describes relative position of a given subtree in a tree

    double relpos_sum; // sum of all involved relative positions
    size_t count;      // amount of positions involved

    TreeRelativePosition() : relpos_sum(0), count(0) {} // = no position
public:

    static TreeRelativePosition unknown() { return TreeRelativePosition(); }

    explicit TreeRelativePosition(double relpos_sum_) // create leaf-info
        : relpos_sum(relpos_sum_),
          count(1)
    {}
    TreeRelativePosition(const TreeRelativePosition& p1, const TreeRelativePosition& p2)
        : relpos_sum(p1.relpos_sum + p2.relpos_sum),
          count(p1.count + p2.count)
    {}

    bool is_known() const { return count; }
    double value() const {
        nt_assert(is_known());
        return relpos_sum / count;
    }
    int compare(const TreeRelativePosition &right) const {
        return double_cmp(value(), right.value());
    }
};

class TreePositionLookup {
    // provides relative position of species inside a tree

    typedef std::map<std::string, unsigned> PosMap;
    PosMap spos;

    void fillFromTree(const TreeNode *node) {
        if (node->is_leaf) {
            size_t pos       = spos.size();
            spos[node->name] = pos;
        }
        else {
            fillFromTree(node->get_leftson());
            fillFromTree(node->get_rightson());
        }
    }
public:
    explicit TreePositionLookup(const TreeNode *tree) {
        fillFromTree(tree);
    }

    TreeRelativePosition relative(const char *name) const {
        /*! returns TreeRelativePosition of species 'name' inside tree.
         */
        PosMap::const_iterator found = spos.find(name);
        return
            (found == spos.end())
            ? TreeRelativePosition::unknown()
            : TreeRelativePosition(found->second/double(spos.size()-1));
    }
};

#else
#error tree_position.h included twice
#endif // TREE_POSITION_H

#ifndef PS_CANDIDATE_HXX
#define PS_CANDIDATE_HXX

#ifndef PS_DEFS_HXX
#include "ps_defs.hxx"
#endif

using namespace std;

class PS_Candidate;
typedef PS_Candidate*                           PS_CandidatePtr;
typedef SmartPtr<PS_Candidate>                  PS_CandidateSPtr;
typedef map<unsigned long,PS_CandidateSPtr>     PS_CandidateMap;
typedef PS_CandidateMap::iterator               PS_CandidateMapIter;
typedef PS_CandidateMap::const_iterator         PS_CandidateMapCIter;
typedef PS_CandidateMap::reverse_iterator       PS_CandidateMapRIter;
typedef PS_CandidateMap::const_reverse_iterator PS_CandidateMapCRIter;

class PS_Candidate {
private:
    PS_Candidate();
    PS_Candidate( const PS_Candidate& );
    explicit PS_Candidate( float _distance, unsigned long _gain, const PS_NodePtr _ps_node, IDSet &_path, PS_CandidatePtr _parent ) {
        filling_level  = 0.0;
        depth          = ULONG_MAX;
        distance       = _distance;
        gain           = _gain;
        node           = _ps_node;
        path           = _path;
        parent         = _parent;
    }

public:
    float           filling_level;
    unsigned long   depth;
    float           distance;
    unsigned long   gain;
    IDSet           path;
    PS_NodePtr      node;
    PS_Candidate* parent;
    PS_CandidateMap children;

    bool hasChild( PS_NodePtr _ps_node ) const {
        PS_CandidateMapCIter found = children.end();
        for ( PS_CandidateMapCIter child = children.begin();
              (child != children.end()) && (found == children.end());
              ++child ) {
            if (child->second->node == _ps_node) found = child;
        }
        return (found != children.end());
    }

    bool alreadyUsedNode( const PS_NodePtr _ps_node ) const {
        if (node.Null()) return false;
        if (_ps_node == node) {
            return true;
        } else {
            return parent->alreadyUsedNode( _ps_node );
        }
    }

    int addChild( unsigned long _distance, unsigned long _gain, const PS_NodePtr _node, IDSet &_path ) {
        PS_CandidateMapIter found = children.find( _gain );
        if (found == children.end()) {
            PS_CandidateSPtr new_child( new PS_Candidate(_distance, _gain, _node, _path, this) );
            children[ _gain ] = new_child;
            return 2;
        } else if (_distance < found->second->distance) {
            children.erase( _gain );
            PS_CandidateSPtr new_child( new PS_Candidate(_distance, _gain, _node, _path, this) );
            children[ _gain ] = new_child;
            return 1;
        }
        return 0;
    }

    void reduceChildren( const float _filling_level ) {
        if (children.size() == 0) return;
        // prepare
        unsigned long best_gain   = children.rbegin()->first;
        unsigned long worst_gain  = children.begin()->first;
        unsigned long middle_gain = (best_gain + worst_gain) >> 1;
        PS_CandidateMapIter middle_child = children.find( middle_gain );
        if (middle_child == children.end()) {
            middle_child = children.lower_bound( middle_gain );
            middle_gain  = middle_child->first;
        }
        PS_CandidateMapIter to_delete = children.end();
        // delete unwanted childs depending on filling level
        if (_filling_level < 50.0) {
            if (children.size() <= 3) return;
            for ( PS_CandidateMapIter c = children.begin();
                  c != children.end();
                  ++c ) {
                if (to_delete != children.end()) {
                    children.erase( to_delete );
                    to_delete = children.end();
                }
                if ((c->first != worst_gain) &&
                    (c->first != middle_gain) &&
                    (c->first != best_gain)) {
                    to_delete = c;
                }
            }
        } else if (_filling_level < 75.0) {
            if (children.size() <= 2) return;
            for ( PS_CandidateMapIter c = children.begin();
                  c != children.end();
                  ++c ) {
                if (to_delete != children.end()) {
                    children.erase( to_delete );
                    to_delete = children.end();
                }
                if ((c->first != middle_gain) &&
                    (c->first != best_gain)) {
                    to_delete = c;
                }
            }
        } else {
            if (children.size() <= 1) return;
            for ( PS_CandidateMapIter c = children.begin();
                  c != children.end();
                  ++c ) {
                if (to_delete != children.end()) {
                    children.erase( to_delete );
                    to_delete = children.end();
                }
                if (c->first != best_gain) {
                    to_delete = c;
                }
            }
        }
        if (to_delete != children.end()) {
            children.erase( to_delete );
            to_delete = children.end();
        }
    }

    void print ( const unsigned long _depth = 0 ) {
        for (unsigned long i = 0; i < _depth; ++i) {
            printf( "   " );
        }
        if (node.Null()) {
            printf( "filling level (%.3f)  distance (%.2f)  gain (%lu)  depth (%lu)  path length (%i)  node (null)  children (%i)\n",
                    filling_level,
                    distance,
                    gain,
                    depth,
                    path.size(),
                    children.size() );
        } else {
            printf( "filling level (%.3f)  distance (%.2f)  gain (%lu)  depth (%lu)  path length (%i)  node (%p)  children (%i)\n",
                    filling_level,
                    distance,
                    gain,
                    depth,
                    path.size(),
                    &(*node),
                    children.size() );
        }
        fflush( stdout );
        for ( PS_CandidateMapRIter child = children.rbegin();
              child != children.rend();
              ++child ) {
            child->second->print( _depth+1);
        }
    }

    explicit PS_Candidate( float _distance ) {
        filling_level = 0.0;
        depth         = 0;
        distance      = _distance;
        gain          = 0;
        node.SetNull();
        parent        = 0;
    }
    ~PS_Candidate() {
        if (path.size() > 0) path.clear();
        if (children.size() > 0) children.clear();
    }
};

#else
#error ps_candidate.hxx included twice
#endif

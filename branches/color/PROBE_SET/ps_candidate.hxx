// =============================================================== //
//                                                                 //
//   File      : ps_candidate.hxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in October 2002                     //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PS_CANDIDATE_HXX
#define PS_CANDIDATE_HXX

#ifndef PS_DEFS_HXX
#include "ps_defs.hxx"
#endif
#ifndef PS_BITMAP_HXX
#include "ps_bitmap.hxx"
#endif

#ifndef _GLIBCXX_CLIMITS
#include <climits>
#endif

using namespace std;

class PS_Candidate;
typedef PS_Candidate*                           PS_CandidatePtr;
typedef SmartPtr<PS_Candidate>                  PS_CandidateSPtr;

struct cmp_candidates
{
  bool operator()(const PS_CandidateSPtr &c1, const PS_CandidateSPtr &c2) const
  {
    return &(*c1) < &(*c2);
  }
};
typedef set<PS_CandidateSPtr, cmp_candidates>    PS_CandidateSet;
typedef PS_CandidateSet::iterator               PS_CandidateSetIter;
typedef PS_CandidateSet::const_iterator         PS_CandidateSetCIter;
typedef PS_CandidateSet::reverse_iterator       PS_CandidateSetRIter;
typedef PS_CandidateSet::const_reverse_iterator PS_CandidateSetCRIter;

typedef map<unsigned long, PS_CandidateSPtr>     PS_CandidateByGainMap;
typedef PS_CandidateByGainMap::iterator               PS_CandidateByGainMapIter;
typedef PS_CandidateByGainMap::const_iterator         PS_CandidateByGainMapCIter;
typedef PS_CandidateByGainMap::reverse_iterator       PS_CandidateByGainMapRIter;
typedef PS_CandidateByGainMap::const_reverse_iterator PS_CandidateByGainMapCRIter;

typedef pair<SpeciesID, PS_BitSet::IndexSet>     ID2IndexSetPair;
typedef set<ID2IndexSetPair>                    ID2IndexSetSet;
typedef ID2IndexSetSet::iterator                ID2IndexSetSetIter;
typedef ID2IndexSetSet::const_iterator          ID2IndexSetSetCIter;
typedef ID2IndexSetSet::reverse_iterator        ID2IndexSetSetRIter;
typedef ID2IndexSetSet::const_reverse_iterator  ID2IndexSetSetCRIter;

class PS_Candidate : virtual Noncopyable {
    PS_Candidate();
    PS_Candidate(const PS_Candidate&);
    explicit PS_Candidate(float _distance, unsigned long _gain, const PS_NodePtr& _ps_node, IDSet &_path, PS_CandidatePtr _parent) {
        filling_level = 0.0;
        depth         = ULONG_MAX;
        distance      = _distance;
        gain          = _gain;
        passes_left   = MAX_PASSES;
        parent        = _parent;
        false_IDs     = 0;
        one_false_IDs = 0;
        one_false_IDs_matches = 0;
        map           = 0;
        node          = _ps_node;
        path          = _path;
    }

public:
    static const unsigned int MAX_PASSES = 3;
    float                  filling_level;
    unsigned long          depth;
    float                  distance;
    unsigned long          gain;
    unsigned int           passes_left;
    PS_Candidate          *parent;
    unsigned long          false_IDs;
    ID2IDSet              *one_false_IDs;
    unsigned long          one_false_IDs_matches;
    PS_BitMap_Counted     *map;
    IDSet                  path;
    PS_NodePtr             node;
    PS_CandidateByGainMap  children;

    unsigned long initFalseIDs(const SpeciesID  _min_id,
                                const SpeciesID  _max_id,
                                      SpeciesID &_min_sets_id,
                                      SpeciesID &_max_sets_id) {
        // if i already have the set return its size
        if (one_false_IDs) {
            return one_false_IDs->size();
        }
        else {
            one_false_IDs = new ID2IDSet;
            false_IDs     = 0;
            PS_BitSet::IndexSet falseIndices;       // temp. set to hold IDs of falses per ID
            // iterate over _min_id .. _max_id range in map
            for (SpeciesID id1 = _min_id;
                  id1 <= _max_id;
                  ++id1) {
                if (map->getCountFor(id1) == _max_id+1) continue;   // skip ID if its already filled
                if (id1 < _min_sets_id) _min_sets_id = id1;
                if (id1 > _max_sets_id) _max_sets_id = id1;
                map->getFalseIndicesFor(id1, falseIndices);
                while ((*(falseIndices.begin()) < id1) &&
                       !falseIndices.empty()) falseIndices.erase(*falseIndices.begin());
                if (falseIndices.empty()) continue;
                if (*falseIndices.begin()  < _min_sets_id) _min_sets_id = *falseIndices.begin();
                if (*falseIndices.rbegin() > _max_sets_id) _max_sets_id = *falseIndices.rbegin();
                // store
                false_IDs += falseIndices.size();
                if (falseIndices.size() == 1) {
                    one_false_IDs->insert(ID2IDPair(id1, *falseIndices.begin()));
                }
            }
        }
        return one_false_IDs->size();
    }

    unsigned long matchPathOnOneFalseIDs(IDSet &_path) {
        unsigned long matches = 0;
        IDSetCIter path_end = _path.end();
        for (ID2IDSetCIter p = one_false_IDs->begin();
              p != one_false_IDs->end();
              ++p) {
            if (_path.find(p->first) == path_end) {
                if (_path.find(p->second) != path_end) ++matches;
            }
            else {
                if (_path.find(p->second) == path_end) ++matches;
            }
        }
        return matches;
    }

    void decreasePasses() {
        --passes_left;
    }

    void getParentMap() {
        if (!parent) return;
        map = parent->map;      // grab parents version of the map
        parent->map = 0;        // unbind parent from map
    }

    bool hasChild(PS_NodePtr _ps_node) const {
        PS_CandidateByGainMapCIter found = children.end();
        for (PS_CandidateByGainMapCIter child = children.begin();
              (child != children.end()) && (found == children.end());
              ++child) {
            if (child->second->node == _ps_node) found = child;
        }
        return (found != children.end());
    }

    bool alreadyUsedNode(const PS_NodePtr& _ps_node) const {
        if (node.isNull()) return false;
        if (_ps_node == node) {
            return true;
        }
        else {
            return parent->alreadyUsedNode(_ps_node);
        }
    }

    int addChild(unsigned long     _distance,
                 unsigned long     _gain,
                 const PS_NodePtr& _node,
                 IDSet&            _path)
    {
        PS_CandidateByGainMapIter found = children.find(_gain);
        if (found == children.end()) {
            PS_CandidateSPtr new_child(new PS_Candidate(_distance, _gain, _node, _path, this));
            children[_gain] = new_child;
            return 2;
        }
        else if (_distance < found->second->distance) {
            children.erase(_gain);
            PS_CandidateSPtr new_child(new PS_Candidate(_distance, _gain, _node, _path, this));
            children[_gain] = new_child;
            return 1;
        }
        return 0;
    }

    bool updateBestChild(const unsigned long _gain,
                         const unsigned long _one_false_IDs_matches,
                         const float         _filling_level,
                         const PS_NodePtr&   _node,
                         IDSet&              _path)
    {
        if (children.empty()) {
            // no child yet
            PS_CandidateSPtr new_child(new PS_Candidate(0, _gain, _node, _path, this));
            new_child->depth = depth+1;
            new_child->filling_level = _filling_level;
            if (_filling_level >= 100.0) new_child->passes_left = 0;
            children[0] = new_child;
            one_false_IDs_matches = _one_false_IDs_matches;
            return true;
        }
        // return false if new child matches less 'must matches' than best child so far
        if (_one_false_IDs_matches < one_false_IDs_matches) return false;
        // return false if new child matches same 'must matches' but has less total gain
        if ((_one_false_IDs_matches == one_false_IDs_matches) &&
            (_gain <= children[0]->gain)) return false;

        children.clear();
        PS_CandidateSPtr new_child(new PS_Candidate(0, _gain, _node, _path, this));
        new_child->depth = depth+1;
        new_child->filling_level = _filling_level;
        if (_filling_level >= 100.0) new_child->passes_left = 0;
        children[0] = new_child;
        one_false_IDs_matches = _one_false_IDs_matches;
        return true;
    }

    void reduceChildren(const float _filling_level) {
        if (children.empty()) return;
        // prepare
        unsigned long best_gain   = children.rbegin()->first;
        unsigned long worst_gain  = children.begin()->first;
        unsigned long middle_gain = (best_gain + worst_gain) >> 1;
        PS_CandidateByGainMapIter middle_child = children.find(middle_gain);
        if (middle_child == children.end()) {
            middle_child = children.lower_bound(middle_gain);
            middle_gain  = middle_child->first;
        }
        PS_CandidateByGainMapIter to_delete = children.end();
        // delete unwanted children depending on filling level
        if (_filling_level < 50.0) {
            if (children.size() <= 3) return;
            for (PS_CandidateByGainMapIter c = children.begin();
                  c != children.end();
                  ++c) {
                if (to_delete != children.end()) {
                    children.erase(to_delete);
                    to_delete = children.end();
                }
                if ((c->first != worst_gain) &&
                    (c->first != middle_gain) &&
                    (c->first != best_gain)) {
                    to_delete = c;
                }
            }
        }
        else if (_filling_level < 75.0) {
            if (children.size() <= 2) return;
            for (PS_CandidateByGainMapIter c = children.begin();
                  c != children.end();
                  ++c) {
                if (to_delete != children.end()) {
                    children.erase(to_delete);
                    to_delete = children.end();
                }
                if ((c->first != middle_gain) &&
                    (c->first != best_gain)) {
                    to_delete = c;
                }
            }
        }
        else {
            if (children.size() <= 1) return;
            for (PS_CandidateByGainMapIter c = children.begin();
                  c != children.end();
                  ++c) {
                if (to_delete != children.end()) {
                    children.erase(to_delete);
                    to_delete = children.end();
                }
                if (c->first != best_gain) {
                    to_delete = c;
                }
            }
        }
        if (to_delete != children.end()) {
            children.erase(to_delete);
            to_delete = children.end();
        }
    }

    void printProbes(const SpeciesID      _species_count,
                      const unsigned long _depth = 0,
                      const bool          _descend = true) const {
        for (unsigned long i = 0; i < _depth; ++i) {
            printf("|  ");
        }
        printf("\n");
        if (node.isNull()) {
            for (unsigned long i = 0; i < _depth; ++i) {
                printf("|  ");
            }
            printf("[%p] depth (%lu) no node\n", this, _depth);
        }
        else if (node->countProbes() == 0) {
            for (unsigned long i = 0; i < _depth; ++i) {
                printf("|  ");
            }
            printf("[%p] depth (%lu)  node (%p)  no probes\n", this, _depth, &(*node));
        }
        else {
            for (PS_ProbeSetCIter probe = node->getProbesBegin();
                  probe != node->getProbesEnd();
                  ++probe) {
                for (unsigned long i = 0; i < _depth; ++i) {
                    printf("|  ");
                }
                printf("[%p] depth (%lu)  node (%p)  matches (%zu)       %u   %u\n",
                        this,
                        _depth,
                        &(*node),
                        ((*probe)->quality > 0) ? path.size() : _species_count - path.size(),
                        (*probe)->length,
                        (*probe)->GC_content);
            }
        }
        fflush(stdout);

        if (_descend) {
            for (PS_CandidateByGainMapCRIter child = children.rbegin();
                  child != children.rend();
                  ++child) {
                child->second->printProbes(_species_count, _depth+1);
            }
        }

    }

    void print (const unsigned long _depth = 0,
                 const bool          _print_one_false_IDs = false,
                 const bool          _descend = true) const {
        for (unsigned long i = 0; i < _depth; ++i) {
            printf("|  ");
        }
        printf("[%p] passes left (%u)  filling level (%.5f)  distance (%6.2f)  gain (%6lu)  depth (%3lu)  path length (%4zu)  ",
                this,
                passes_left,
                filling_level,
                distance,
                gain,
                depth,
                path.size());
        if (node.isNull()) {
            printf("node (undefined)  children (%2zu)  ", children.size());
        }
        else {
            printf("node (%p)  children (%2zu)  ", &(*node), children.size());
        }
        printf("(%4lu %4zu)/%lu", one_false_IDs_matches, (one_false_IDs) ? one_false_IDs->size() : 0, false_IDs);
        if (_print_one_false_IDs) {
            printf("\n");
            for (unsigned long i = 0; i < _depth; ++i) {
                printf("|  ");
            }
            printf("            one_false_IDs : ");
            if (one_false_IDs) {
                for (ID2IDSetCIter p = one_false_IDs->begin();
                      p != one_false_IDs->end();
                      ++p) {
                    printf("(%i %i)  ", p->first, p->second);
                }
            }
            else {
                printf("none");
            }
        }
        printf("\n"); fflush(stdout);
        if (_descend) {
            for (PS_CandidateByGainMapCRIter child = children.rbegin();
                  child != children.rend();
                  ++child) {
                child->second->print(_depth+1);
            }
        }
    }

    void save(PS_FileBuffer       *_file,
               const unsigned long  _bits_in_map) {
        unsigned long count;
        // gain
        _file->put_ulong(gain);
        // passes_left
        _file->put_uint(passes_left);
        // false_IDs
        _file->put_ulong(false_IDs);
        // one_false_IDs
        if (one_false_IDs) {
            count = one_false_IDs->size();
            _file->put_ulong(count);
            for (ID2IDSetCIter p = one_false_IDs->begin();
                  p != one_false_IDs->end();
                  ++p) {
                _file->put_int(p->first);
                _file->put_int(p->second);
            }
        }
        else {
            _file->put_ulong(0);
        }
        // one_false_IDs_matches
        _file->put_ulong(one_false_IDs_matches);
        // path
        count = path.size();
        _file->put_ulong(count);
        for (IDSetCIter id = path.begin();
              id != path.end();
              ++id) {
            _file->put_int(*id);
        }
        // children
        count = children.size();
        _file->put_ulong(count);
        for (PS_CandidateByGainMapIter child = children.begin();
              child != children.end();
              ++child) {
            child->second->save(_file, _bits_in_map);
        }
    }

    void load(PS_FileBuffer       *_file,
              const unsigned long  _bits_in_map,
              const PS_NodePtr&    _root_node)
    {
        unsigned long count;
        // gain
        _file->get_ulong(gain);
        // passes_left
        _file->get_uint(passes_left);
        // false_IDs & filling_level
        _file->get_ulong(false_IDs);
        filling_level = (float)(_bits_in_map - false_IDs) / _bits_in_map * 100.0;
        // one_false_IDs
        SpeciesID id1;
        SpeciesID id2;
        _file->get_ulong(count);
        one_false_IDs = (count) ? new ID2IDSet : 0;
        for (; count > 0; --count) {
            _file->get_int(id1);
            _file->get_int(id2);
            one_false_IDs->insert(ID2IDPair(id1, id2));
        }
        // one_false_IDs_matches
        _file->get_ulong(one_false_IDs_matches);
        // path & node
        _file->get_ulong(count);
        if (count) node = _root_node;
        for (; count > 0; --count) {
            _file->get_int(id1);
            path.insert(id1);
            node = node->getChild(id1).second;
        }
        // children
        _file->get_ulong(count);
        for (; count > 0; --count) {
            PS_CandidateSPtr new_child(new PS_Candidate(0.0));
            new_child->depth  = depth+1;
            new_child->parent = this;
            new_child->load(_file, _bits_in_map, _root_node);
            children[new_child->gain] = new_child;
        }
    }

    explicit PS_Candidate(float _distance) {
        filling_level = 0.0;
        depth         = 0;
        distance      = _distance;
        gain          = 0;
        passes_left   = 0;
        parent        = 0;
        false_IDs     = 0;
        one_false_IDs = 0;
        one_false_IDs_matches = 0;
        map           = 0;
        node.SetNull();
    }
    ~PS_Candidate() {
        if (map)           delete map;
        if (one_false_IDs) delete one_false_IDs;

        path.clear();
        children.clear();
    }
};

#else
#error ps_candidate.hxx included twice
#endif // PS_CANDIDATE_HXX

// =============================================================== //
//                                                                 //
//   File      : ps_database.hxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in October 2002                     //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PS_DATABASE_HXX
#define PS_DATABASE_HXX

#ifndef PS_NODE_HXX
#include "ps_node.hxx"
#endif

using namespace std;

class PS_Database : PS_Callback, virtual Noncopyable {
private:
    string FILE_ID;

    // file
    PS_FileBuffer *db_file;

    // ID<->name mappings
    Name2IDMap     db_name2id_map;
    ID2NameMap     db_id2name_map;

    // tree
    PS_NodePtr     db_rootnode;

    bool readHeader(PS_FileBuffer *_file);
    void writeHeader(PS_FileBuffer *_file);

    void readMappings(PS_FileBuffer *_file, ID2NameMap &_id2name_map, Name2IDMap &_name2id_map);
    void writeMappings(PS_FileBuffer *_file, ID2NameMap &_id2name_map);

    void readTree(PS_FileBuffer *_file);
    void writeTree(PS_FileBuffer *_file);

    // tree merging data structures and functions
    ID2IDMap   db_file2db_id_map;
    PS_NodePtr db_path;
    SpeciesID  db_MAX_ID;

    void callback(void *_caller) OVERRIDE;

public:
    static const bool   READONLY  = true;
    static const bool   WRITEONLY = false;

    //
    // I/O
    //
    bool load() {
        if (!db_file->isReadonly()) return false;       // cannot read a writeonly file
        if (!readHeader(db_file)) return false;         // not a file i wrote
        readMappings(db_file, db_id2name_map, db_name2id_map);
        readTree(db_file);
        return true;
    }

    bool save() {
        if (db_file->isReadonly()) return false;        // cannot write to a readonly file
        writeHeader(db_file);
        writeMappings(db_file, db_id2name_map);
        writeTree(db_file);
        return true;
    }

    bool saveTo(const char *_filename) {
        PS_FileBuffer *file = new PS_FileBuffer(_filename, PS_FileBuffer::WRITEONLY);
        writeHeader(file);
        writeMappings(file, db_id2name_map);
        writeTree(file);
        if (file) delete file;
        return true;
    }

    bool merge(const char *_other_db_name);

    //
    // access tree
    //
    PS_NodePtr getRootNode() {
        return db_rootnode;
    }
    const PS_NodePtr getConstRootNode() {
        return db_rootnode;
    }

    //
    // access mappings
    //
    bool getIDForName(SpeciesID &_id, const string &_name) {
        Name2IDMapCIter it = db_name2id_map.find(_name);
        if (it != db_name2id_map.end()) {
            _id = it->second;
            return true;
        }
        else {
            return false;
        }
    }

    bool getNameForID(const SpeciesID _id, string &_name) {
        ID2NameMapCIter it = db_id2name_map.find(_id);
        if (it != db_id2name_map.end()) {
            _name = it->second;
            return true;
        }
        else {
            return false;
        }
    }

    bool insertMapping(SpeciesID _id, const string &_name) {
        ID2NameMapCIter it = db_id2name_map.find(_id);
        if (it != db_id2name_map.end()) {
            return (it->second.compare(_name) == 0);   // return false if _name is not the string already stored for _id
        }
        else {
            db_id2name_map[_id] = _name;
            db_name2id_map[_name] = _id;
            return true;
        }
    }

    void setMappings(Name2IDMap &_name2id, ID2NameMap &_id2name) {
        db_name2id_map.clear();
        db_name2id_map = Name2IDMap(_name2id);
        db_id2name_map.clear();
        db_id2name_map = ID2NameMap(_id2name);
    }

    SpeciesID getMaxID() {
        return db_id2name_map.rbegin()->first;
    }
    SpeciesID getMinID() {
        return db_id2name_map.begin()->first;
    }
    long getSpeciesCount() {
        return db_id2name_map.size();
    }

    //
    // utility-functions
    //
    bool isReadonly() { return db_file->isReadonly(); }

    //
    // initialization-functions
    //
    void reinit(const char *_name, bool _readonly);   // reinit. with new file

    PS_Database(const char *_name, bool _readonly)
        : FILE_ID("PROBE_SET_DATABASE V1.0\n"),
          db_file(NULL) 
    {
        db_rootnode.SetNull();
        reinit(_name, _readonly);
    }

    ~PS_Database() OVERRIDE {
        if (db_file) delete db_file;
        db_name2id_map.clear();
        db_id2name_map.clear();
        db_rootnode.SetNull();
    }

};

#else
#error ps_database.hxx included twice
#endif // PS_DATABASE_HXX

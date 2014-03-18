
#include "ps_database.hxx"

using namespace std;

void PS_Database::reinit(const char *_name, bool _readonly) {
    if (db_file) {
        db_file->reinit(_name, _readonly);
    }
    else {
        db_file = new PS_FileBuffer(_name, _readonly);
    }
    db_name2id_map.clear();
    db_id2name_map.clear();
    if (!db_rootnode.isNull()) db_rootnode.SetNull();
    db_rootnode = new PS_Node(-1);
}

void PS_Database::readMappings(PS_FileBuffer *_file, ID2NameMap &_id2name_map, Name2IDMap &_name2id_map) {
    char *buffer = (char *)malloc(_file->BUFFER_SIZE);
    // read number of species
    unsigned long int number_of_species = 0;
    _file->get_ulong(number_of_species);
    // read mappings
    for (unsigned long int i = 0; i < number_of_species; ++i) {
        // read id
        SpeciesID id;
        _file->get(&id, sizeof(SpeciesID));
        // read name
        unsigned int length_of_name;
        _file->get_uint(length_of_name);
        _file->get(buffer, length_of_name);
        // store in mappings
        string name(buffer, length_of_name);
        _id2name_map[id] = name;
        _name2id_map[name] = id;
    }
    free(buffer);
}

void PS_Database::writeMappings(PS_FileBuffer *_file, ID2NameMap &_id2name_map) {
    // write number of species
    _file->put_ulong(_id2name_map.size());
    // write mappings
    for (ID2NameMapCIter i = _id2name_map.begin(); i != _id2name_map.end(); ++i) {
        // write id
        _file->put(&(i->first), sizeof(SpeciesID));
        // write name
        unsigned int length_of_name = i->second.size();
        _file->put_uint(length_of_name);
        _file->put(i->second.c_str(), length_of_name);
    }
}

void PS_Database::readTree(PS_FileBuffer *_file) {
    if (!db_rootnode.isNull()) db_rootnode.SetNull();     // discard old tree
    db_rootnode = new PS_Node(-1);
    db_rootnode->load(_file);
}

void PS_Database::writeTree(PS_FileBuffer *_file) {
    if (db_rootnode.isNull()) return;                     // no tree, no write
    db_rootnode->save(_file);
}

bool PS_Database::readHeader(PS_FileBuffer *_file) {
    char *buffer = (char *) malloc(FILE_ID.size());
    _file->get(buffer, FILE_ID.size());
    bool file_ok = (FILE_ID.compare(buffer) == 0);
    if (buffer) free(buffer);
    return file_ok;
}

void PS_Database::writeHeader(PS_FileBuffer *_file) {
    _file->put(FILE_ID.c_str(), FILE_ID.size());
}

void PS_Database::callback(void *_caller) {
    //
    // return if node has no probes
    //
    if (!((PS_Node *)_caller)->hasProbes()) return;

    //
    // convert IDs from file to DB-IDs
    //
    IDSet      path;
    PS_NodePtr current_node = db_path;
    while (current_node->hasChildren()) {
        // get next node in path
        pair<bool, PS_NodePtr> child = current_node->getChild(0);
        ps_assert(child.first);
        current_node = child.second;
        // get ID from node
        SpeciesID id = current_node->getNum();
        // store ID in ID-Set
        ID2IDMapCIter db_id = db_file2db_id_map.find(id);
        path.insert((db_id == db_file2db_id_map.end()) ? id : db_id->second);
    }

    //
    // assert path
    //
    current_node = db_rootnode;
    for (IDSetCIter id=path.begin(); id != path.end(); ++id) {
        current_node = current_node->assertChild(*id);
    }

    //
    // append probes
    //
    current_node->addProbes(((PS_Node *)_caller)->getProbesBegin(), ((PS_Node *)_caller)->getProbesEnd());
}

bool PS_Database::merge(const char *_other_db_name) {
    //
    // read other DB's mappings
    //
    PS_FileBuffer *other_db_file = new PS_FileBuffer(_other_db_name, PS_FileBuffer::READONLY);
    Name2IDMap     other_name2id_map;
    ID2NameMap     other_id2name_map;

    if (!readHeader(other_db_file)) return false;   // not a file i wrote
    readMappings(other_db_file, other_id2name_map, other_name2id_map);

    //
    // get next assignable ID from highest used ID in mappings
    //
    SpeciesID next_usable_ID = (db_id2name_map.rbegin()->first > other_id2name_map.rbegin()->first) ? db_id2name_map.rbegin()->first + 1 : other_id2name_map.rbegin()->first + 1;

    //
    // iterate over DB names
    //
    db_file2db_id_map.clear();
    for (Name2IDMapCIter i=db_name2id_map.begin(); i != db_name2id_map.end(); ++i) {
        // lookup name in other mapping
        Name2IDMapIter other_i = other_name2id_map.find(i->first);

        // if name not in other mapping
        if (other_i == other_name2id_map.end()) {
            // lookup ID in other mapping
            ID2NameMapIter other_i2 = other_id2name_map.find(i->second);

            // if ID is used for other name in other_mappings
            if (other_i2 != other_id2name_map.end()) {
                // lookup other name in DB mapping
                Name2IDMapIter i2 = db_name2id_map.find(other_i2->second);

                // if other name is not in DB mapping
                if (i2 == db_name2id_map.end()) {
                    // store file->DB ID mapping
                    db_file2db_id_map[other_i2->first] = next_usable_ID;
                    ++next_usable_ID;
                    // erase handled name
                    other_name2id_map.erase(other_i2->second);
                }
            }
        }
        // if name in other mapping with different ID
        else if (other_i->second != i->second) {
            // store file->DB ID mapping
            db_file2db_id_map[other_i->second] = i->second;
            // erase handled name
            other_name2id_map.erase(other_i);
        }
        // if name in other mapping with same ID
        else {
            // erase handled name
            other_name2id_map.erase(other_i);
        }
    }

    //
    // iterate over remaining file names
    //
    for (Name2IDMapCIter other_i=other_name2id_map.begin(); other_i != other_name2id_map.end(); ++other_i) {
        db_file2db_id_map[other_i->second] = next_usable_ID;
        ++next_usable_ID;
    }
    db_MAX_ID = next_usable_ID-1;

    // append tree if mappings are equal
    if (db_file2db_id_map.empty()) {
        return db_rootnode->append(other_db_file);
    }
    // merge in tree if mappings differ
    else {
        if (db_path.isNull()) db_path = new PS_Node(-1);
        return db_path->read(other_db_file, this);
    }
}

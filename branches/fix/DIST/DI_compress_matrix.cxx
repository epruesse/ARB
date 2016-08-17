#include "di_matr.hxx"

int DI_MATRIX::search_group(TreeNode *node, GB_HASH *hash, size_t& groupcnt, char *groupname, DI_ENTRY **groups) {
    //  returns 1 only if groupname != null and there are species for that group

    if (node->is_leaf) {
        if (!node->name) return 0;
        DI_ENTRY *phentry = (DI_ENTRY *)GBS_read_hash(hash, node->name);
        if (!phentry) {     // Species is not a member of tree
            return 0;
        }
        if (groupname) {        // There is a group for this species
            phentry->group_nr = groupcnt;
            return 1;
        }
        return 0;
    }
    char *myname;
    if (groupname) {
        myname = groupname;
    }
    else {
        myname = 0;
        if (node->gb_node && node->name) {      // but we are a group
            GBDATA *gb_grouped = GB_entry(node->gb_node, "grouped");
            if (gb_grouped && GB_read_byte(gb_grouped)) {
                myname = node->name;
            }
        }
    }
    int erg =
        search_group(node->get_leftson(),  hash, groupcnt, myname, groups) +
        search_group(node->get_rightson(), hash, groupcnt, myname, groups);

    if (!groupname) {       // we are not a sub group
        if (myname) {       // but we are a group
            if (erg>0) {            // it is used
                groups[groupcnt] = new DI_ENTRY(myname, this);
                groupcnt++;
            }
        }
        return 0;
    }
    else {
        return erg;         // We are using an inherited group
    }
}

char *DI_MATRIX::compress(TreeNode *tree) {
    // create a hash table of species
    GB_HASH *hash = GBS_create_hash(nentries, GB_IGNORE_CASE);
    char *error = 0;
    for (size_t i=0; i<nentries; i++) {
        if (entries[i]->name) {
            GBS_write_hash(hash, entries[i]->name, (long)entries[i]);
        }
        entries[i]->group_nr = -1;
    }

    size_t     groupcnt = 0;
    DI_ENTRY **groups   = new DI_ENTRY *[nentries];
    search_group(tree, hash, groupcnt, 0, groups); // search a group for all species and create groups

    DI_ENTRY **found_groups = 0;

    if (groupcnt) { // if we found groups => make copy of group array
        found_groups = new DI_ENTRY *[groupcnt];
        memcpy(found_groups, groups, groupcnt*sizeof(*groups));
    }

    int nongroupcnt = 0; // count # of species NOT in groups and copy them to 'groups'
    for (size_t i=0; i<nentries; i++) {
        if (entries[i]->name && entries[i]->group_nr == -1) { // species not in groups
            groups[nongroupcnt] = new DI_ENTRY(entries[i]->name, this);
            entries[i]->group_nr = nongroupcnt++;
        }
        else { // species is in group => add nentries to group_nr
            entries[i]->group_nr = entries[i]->group_nr + nentries;
        }
    }

    // append groups to end of 'groups'
    if (groupcnt) {
        memcpy(groups+nongroupcnt, found_groups, groupcnt*sizeof(*groups)); // copy groups to end of 'groups'
        delete [] found_groups;
        found_groups = 0;

        for (size_t i=0; i<nentries; i++) {
            if (entries[i]->name && entries[i]->group_nr>=int(nentries)) {
                entries[i]->group_nr = entries[i]->group_nr - nentries + nongroupcnt; // correct group_nr's for species in groups
            }
        }
    }

    groupcnt += nongroupcnt; // now groupcnt is # of groups + # of non-group-species

    AP_smatrix count(groupcnt);
    AP_smatrix *sum = new AP_smatrix(groupcnt);

    // Now we have create a new DI_ENTRY table, let's do the matrix
    for (size_t i=0; i<nentries; i++) {
        for (size_t j=0; j<=i; j++) {
            int x = entries[i]->group_nr;   if (x<0) continue;
            int y = entries[j]->group_nr;   if (y<0) continue;
            // x,y are the destination i,j
            count.set(x, y, count.get(x, y)+1.0);
            sum->set(x, y, sum->get(x, y)+matrix->get(i, j));
        }
    }

    for (size_t i=0; i<groupcnt; i++) {    // get the arithmetic average
        for (size_t j=0; j<=i; j++) {
            AP_FLOAT c = count.get(i, j);
            if (c > 0) {
                sum->set(i, j, sum->get(i, j) / c);
            }
        }
    }
    delete matrix;
    matrix = sum;

    for (size_t i=0; i<nentries; i++) { // delete everything
        delete entries[i];
        entries[i] = 0;
    }
    free(entries);

    entries           = groups;
    nentries          = groupcnt;
    allocated_entries = groupcnt;
    matrix_type       = DI_MATRIX_COMPRESSED;

    GBS_free_hash(hash);
    return error;
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>

#include <awt_tree.hxx>
#include "dist.hxx"

#include <di_matr.hxx>
#include <di_view_matrix.hxx>

/*  returns 1 only if groupname != null and there are species for that group */
int PHMATRIX::search_group(GBT_TREE *node,GB_HASH *hash, long *groupcnt,char *groupname, PHENTRY **groups){
    if (node->is_leaf) {
        if (!node->name) return 0;
        PHENTRY *phentry = (PHENTRY *)GBS_read_hash(hash,node->name);
        if (!phentry) {     // Species is not a member of tree
            return 0;
        }
        if (groupname) {        // There is a group for this species
            phentry->group_nr = *groupcnt;
            return 1;
        }
        return 0;
    }
    char *myname;
    if (groupname) {
        myname = groupname;
    }
    else{
        myname = 0;
        if (node->gb_node && node->name){       // but we are a group
            GBDATA *gb_grouped = GB_find(node->gb_node, "grouped",0,down_level);
            if (gb_grouped && GB_read_byte(gb_grouped)) {
                myname = node->name;
            }
        }
    }
    int erg = search_group(node->leftson,hash,groupcnt,myname,groups) +
        search_group(node->rightson,hash,groupcnt,myname,groups);

    if (!groupname){        // we are not a sub group
        if (myname){        // but we are a group
            if (erg>0) {            // it is used
                groups[*groupcnt] = new PHENTRY(myname,this);
                (*groupcnt)++;
            }
        }
        return 0;
    }
    else {
        return erg;         // We are using an inherited group
    }
}

char *PHMATRIX::compress(GBT_TREE *tree){
    GB_HASH *hash = GBS_create_hash(nentries*2, 1);
    int i,j;            // create a hash table of species
    char *error = 0;
    for (i=0;i<nentries;i++) {
        if (entries[i]->name) {
            GBS_write_hash(hash, entries[i]->name, (long)entries[i]);
        }
        entries[i]->group_nr = -1;
    }

    long groupcnt = 0;
    PHENTRY **groups = new PHENTRY *[nentries];
    search_group(tree,hash,&groupcnt,0,groups); // search a group for all species and create groups

    PHENTRY **found_groups = 0;

    if (groupcnt) { // if we found groups => make copy of group array
        found_groups = new PHENTRY *[groupcnt];
        memcpy(found_groups, groups, groupcnt*sizeof(*groups));
    }

    int nongroupcnt = 0; // count # of species NOT in groups and copy then to 'groups'
    for (i=0;i<nentries;i++) {
        if (entries[i]->name && entries[i]->group_nr == -1) { // species not in groups
            groups[nongroupcnt] = new PHENTRY(entries[i]->name,this);
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

        for (i=0; i<nentries; i++) {
            if (entries[i]->name && entries[i]->group_nr>=nentries) {
                entries[i]->group_nr = entries[i]->group_nr - nentries + nongroupcnt; // correct group_nr's for species in groups
            }
        }
    }

    groupcnt += nongroupcnt; // now groupcnt is # of groups + # of non-group-species

    AP_smatrix count(groupcnt);
    AP_smatrix *sum = new AP_smatrix(groupcnt);

    // Now we have create a new PHENTRY table, let's do the matrix
    for (i=0;i<nentries;i++){
        for (j=0;j<=i;j++) {
            int x = entries[i]->group_nr;   if (x<0) continue;
            int y = entries[j]->group_nr;   if (y<0) continue;
            // x,y are the destination i,j
            count.set(x,y,count.get(x,y)+1.0);
            sum->set(x,y,sum->get(x,y)+matrix->get(i,j));
        }
    }

    for (i=0;i<groupcnt;i++){       // get the arithmetic average
        for (j=0;j<=i;j++) {
            AP_FLOAT c = count.get(i,j);
            if (c > 0) {
                sum->set(i,j, sum->get(i,j) / c);
            }
        }
    }
    delete matrix;
    matrix = sum;

    for (i=0;i<nentries;i++) {  // delete everything
        delete entries[i];
        entries[i] = 0;
    }
    free(entries);

    entries = groups;
    nentries = groupcnt;
    entries_mem_size = groupcnt;
    matrix_type = PH_MATRIX_COMPRESSED;

    GBS_free_hash(hash);
    return error;
}

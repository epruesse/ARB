#include <stdio.h>
#include <stdlib.h>
/* #include <malloc.h> */
#include <string.h>
#include "aisc.h"

#include "struct_man.h"

/* AISC_MKPT_PROMOTE:struct aisc_hash_node; */

/********************************* HASH TABLES ****************************/
#define CORE
#define HASH_SIZE 103123
#define TRF_HASH_SIZE 103123

struct aisc_hash_node {
    char           *key;
    long             data;
    struct aisc_hash_node *next;
};


struct aisc_hash_node **
aisc_init_hash(int size)
{
    struct aisc_hash_node **tab;
    tab = (struct aisc_hash_node **) calloc(sizeof(struct aisc_hash_node *), size);
    tab[0] = (struct aisc_hash_node *) calloc(sizeof(struct aisc_hash_node), 1);
    tab[0]->data = size;
    tab[0]->key = (char *)strdup("len_of_hash_table_(c) oliver_strunk 1.3.93");
    return tab;
}

int aisc_hash(char *key,int size)
{
    unsigned int i,len,x;
    len = strlen(key);
    x = 0;
    for (i=0;i<len;i++) {
        x = x<<2 ^ key[i];
    }
    x = x%size;
    return x;
}

void
aisc_free_key(struct aisc_hash_node **table,char *key)
{
    long                   i, size;
    struct aisc_hash_node *hn, *hhn;

    if (table && table[0]) {
        size = table[0]->data;
        i = aisc_hash(key, (int)size);

        for (hn = hhn = table[i]; hn; hn = hn->next) {
            if (strcmp(key, hn->key)) {
                hhn = hn;
                continue;
            }
            if (hn != hhn)
                hhn->next = hn->next;
            else {
                table[i] = hhn->next;
            }
            free(hn->key);
            free((char *)hn);
            break;
        }
    }
}

void aisc_free_hash(struct aisc_hash_node **table)
{
    long i,end;
    struct aisc_hash_node *hn,*hnn;

    end = table[0]->data;
    for (i=0;i<end;i++){
        for (hn = table[i];hn;hn=hnn) {
            hnn = hn->next;
            free(hn->key);
            free((char *)hn);
        }
    }
    free((char *)table);
}


void aisc_insert_hash(struct aisc_hash_node **table,char *key,long data)
{
    long i,size;
    struct aisc_hash_node *hn,*hnl;

    size = table[0]->data;
    i = aisc_hash(key,(int)size);
    hnl = 0;
    for (hn=table[i];hn;hn=hn->next) {
        hnl = hn;
        if (strcmp(key,hn->key) == 0) {
            hn->data = data;
            return;
        };
    };
    hn = (struct aisc_hash_node *)calloc(sizeof(struct aisc_hash_node),1);
    hn->key = (char *)strdup(key);
    hn->data = data;
    if (hnl) {
        hnl->next = hn;
    }else{
        table[i] = hn;
    }
}

long aisc_read_hash(struct aisc_hash_node **table,char *key)
{
    long i,size;
    struct aisc_hash_node *hn;

    if( table && table[0]){
        size = table[0]->data;
        i = aisc_hash(key,(int)size);
        for (hn=table[i];hn;hn=hn->next) {
            if (strcmp(key,hn->key) == 0) return hn->data;
        }
    }
    return 0;
}
/*************************************** LINK CONTROL ***********************/
const char *aisc_link(dllpublic_ext * parent, dllheader_ext * mh) {
    if (!mh) {
        CORE;
        return "Object is (NULL)";
    }
    if (mh->parent) {
        CORE;
        return "Object already linked";
    }
    if (!parent) {
        CORE;
        return "Parent is (NULL)";
    }
    if (parent->key != mh->key) {
        CORE;
        return "Parent key doesnt match Object key";
    }
    if (mh->ident) {
        if (strlen(mh->ident) <= 0) {
            CORE;
            return "Too short ident";
        }
        if (parent->hash) {
            if (aisc_read_hash((struct aisc_hash_node **)parent->hash, mh->ident)) {
                CORE;
                return "Object already in list";
            } else {
                aisc_insert_hash((struct aisc_hash_node **)parent->hash, mh->ident, (long)mh);
            }
        } else {
            parent->hash = (long)aisc_init_hash(HASH_SIZE);
            aisc_insert_hash((struct aisc_hash_node **)parent->hash, mh->ident, (long)mh);
        }
    }
    mh->next = mh->previous = NULL;
    if (!parent->first) {
        parent->cnt = 1;
        parent->first = mh;
        parent->last = mh;
    } else {
        parent->cnt++;
        mh->previous = parent->last;
        parent->last->next = mh;
        parent->last = mh;
    }
    mh->parent = parent;
    return 0;
}



const char *aisc_unlink(dllheader_ext * mh)
{
    dllpublic_ext        *parent;

    if (!(parent = (dllpublic_ext *)mh->parent)) {
        CORE;
        return "Object not linked";
    }
    if (parent->hash) {
        aisc_free_key((struct aisc_hash_node **)parent->hash, mh->ident);
    }
    if (parent->cnt <= 0) {
        CORE;
        return "Parent count is 0";
    }
    if (mh->previous) {
        if (mh->previous->next != mh) {
            CORE;
            return "Fatal Error: Object is a copy, not original";
        }
        mh->previous->next = mh->next;
    } else {
        parent->first = mh->next;
    }
    if (mh->next) {
        mh->next->previous = mh->previous;
    } else {
        parent->last = mh->previous;
    }
    mh->parent = NULL;
    mh->previous = NULL;
    mh->next = NULL;

    parent->cnt--;
    if (! parent->cnt) {
        if (parent->hash) {
            aisc_free_hash((struct aisc_hash_node **)parent->hash);
            parent->hash = 0;
        }
    }
    return 0;
}

long aisc_find_lib(dllpublic_ext *parent, char *ident)
{
    if (!parent->hash) return 0;
    if (!ident) return 0;
    return aisc_read_hash((struct aisc_hash_node **)parent->hash, ident);
}

/*************************************** AISC P_MOVE *********************************/
struct trf_dest_struct {
    struct trf_dest_struct *next;
    long	*dest;
};

struct trf_struct	{
    struct trf_struct *next;
    long	new_item;
    long	old;
    struct trf_dest_struct	*dests;
};

int trf_hash(long p)
{
    return (p+(p>>8))&(TRF_HASH_SIZE-1);
}

static int trf_level = 0;
static struct trf_struct **trf_sp = 0;

long trf_create(long old, long new_item)
{
    long i;
    struct trf_struct *ts;
    struct trf_dest_struct *tds,*ntds;
    if (!trf_sp) return 0;
    i = trf_hash(old);
    for (ts = trf_sp[i]; ts; ts = ts->next) {
        if (ts->old == old)	{
            if (ts->new_item && (ts->new_item != new_item)) {
                fprintf(stderr, "ERROR IN trf_commit:\n");
                *(int *) NULL = 0;
            }else{
                ts->new_item = new_item;
                for (tds = ts->dests; tds; tds = ntds) {
                    *tds->dest = new_item;
                    ntds = tds->next;
                    free((char *)tds);
                }
            }
            return 0;
        }
    }
    ts = (struct trf_struct *)calloc(sizeof(struct trf_struct),1);
    ts->next = trf_sp[i];
    trf_sp[i] = ts;
    ts->new_item = new_item;
    ts->old = old;
    return 0;
}

void trf_link(long old, long *dest)
{
    long i;
    struct trf_struct *ts,*fts;
    struct trf_dest_struct *tds;
    if (!trf_sp) return;
    i = trf_hash(old);
    fts = 0;
    for (ts = trf_sp[i]; ts; ts = ts->next) {
        if (ts->old == old)	{ fts = ts; break;}
    }
    if (!fts) {
        ts = (struct trf_struct *)calloc(sizeof(struct trf_struct),1);
        ts->next = trf_sp[i];
        trf_sp[i] = ts;
        ts->old = old;
        fts = ts;
    }
    tds = (struct trf_dest_struct *)calloc(sizeof(struct trf_dest_struct),1);
    tds->next = fts->dests;
    fts->dests = tds;
    tds->dest = dest;
}

int trf_begin(void)
{
    if (trf_level==0){
        trf_sp = (struct trf_struct **)calloc(sizeof(struct trf_struct *),TRF_HASH_SIZE);
    }
    trf_level ++;
    return 0;
}

int trf_commit(int errors)	/* if errors == 1 then print errors and CORE */
{
    int i;
    struct trf_dest_struct *tds,*ntds;
    struct trf_struct *ts,*nts;
    trf_level --;
    if (!trf_level) {
        for (i = 0; i < TRF_HASH_SIZE; i++) {
            for (ts = trf_sp[i]; ts; ts = nts) {
                if (errors) {
                    if (ts->dests) {
                        fprintf(stderr, "ERROR IN trf_commit:\n");
                        *(int *) NULL = 0;
                    }
                } else {
                    for (tds = ts->dests; tds; tds = ntds) {
                        ntds = tds->next;
                        free((char *)tds);
                    }
                }
                nts = ts->next;
                free((char *)ts);
            }
        }
        free((char *)trf_sp);
        trf_sp = 0;
    }
    return 0;
}

/***************************************** bytestring functions *************************************/

int aisc_server_dllint_2_bytestring(dllpublic_ext * pb,bytestring *bs,int offset)
{
    int	*ptr;
    dllheader_ext * mh;
    if (bs->data) free(bs->data);
    bs->data = 0;
    bs->size = 0;
    if (pb->cnt == 0) return 0;

    bs->size = sizeof(int) * pb->cnt;
    ptr = (int *)malloc(bs->size);
    bs->data = (char *)ptr;

    for (mh=pb->first;mh;mh=mh->next) {
        *(ptr++) = *(int *) (((char *)mh)+offset);
    }
    return 0;
}

int aisc_server_dllstring_2_bytestring(dllpublic_ext * pb,bytestring *bs,int offset)
{
    int 	size;
    int	*ptr;
    dllheader_ext * mh;
    char	*strptr,*str;
    int	stringlenghts;

    if (bs->data) free(bs->data);
    bs->data = 0;
    bs->size = 0;
    if (pb->cnt == 0) return 0;

    size = sizeof(int) * (pb->cnt+1);
    stringlenghts = 0;
    for (mh=pb->first;mh;mh=mh->next) {
        str = *(char **) (((char *)mh)+offset);
        if (str) {
            stringlenghts += strlen(str)+1;
        }
    }
    bs->size = size+stringlenghts;
    ptr = (int *)malloc(bs->size);
    bs->data = (char *)ptr;
    strptr = ((char *)ptr)+size;

    for (mh=pb->first;mh;mh=mh->next) {
        str = *(char **) (((char *)mh)+offset);
        if (str) {
            size = strlen(str);
            memcpy(strptr,str,size+1);
            *(ptr++) = strptr - bs->data;
            strptr += size +1;
        }else{
            *(ptr++) = 0;
        }
    }
    *(ptr++) = -1;
    return 0;
}

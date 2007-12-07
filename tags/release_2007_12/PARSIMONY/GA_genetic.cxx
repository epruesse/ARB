#include <stdlib.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <string.h>
#include <stdio.h>
#include <memory.h>
// #include <malloc.h>
#include <iostream.h>
#include "AP_buffer.hxx"
#include "parsimony.hxx"
#include "ap_tree_nlen.hxx"
#include "GA_genetic.hxx"

#define AP_GWT_SIZE 0
#define AP_PUT_DATA 1


GA_genetic::GA_genetic(void) {
	gb_tree_start = 0;
	gb_tree_opt = 0;
	gb_joblist = 0;
	gb_genetic = 0;
	gb_main = 0;
	gb_jobCount = 0;
	gb_bestTree = 0;
	min_job = 1;
}

GA_genetic::~GA_genetic(void) {
	if (fout) {
		if (fclose(fout) != 0) {
			new AP_ERR("~GA_genetic","coldnt close output");
		}
	}
}

void GA_genetic::init(GBDATA *gbmain) {
	this->gb_main = gbmain;
	GB_push_transaction(gb_main);
	gb_genetic = GB_search(gb_main,"genetic",GB_CREATE_CONTAINER);
	gb_presets = GB_find(gb_genetic,"presets",NULL,down_level);
	gb_tree_start = GB_find(gb_genetic,"tree_start",NULL,down_level);
	gb_tree_opt = GB_find(gb_genetic,"tree_opt",NULL,down_level);
	gb_joblist = GB_find(gb_genetic,"job_list",NULL,down_level);
	gb_bestTree = GB_find(gb_presets,"bestTree",NULL,down_level);
	gb_jobCount = GB_find(gb_presets,"jobCount",NULL,down_level);
	gb_maxTree = GB_find(gb_presets,"maxTree",NULL,down_level);
	gb_treeName = GB_find(gb_presets,"treeName",NULL,down_level);

	if (gb_presets == 0) {
		new AP_ERR("init","No presets defined");
		return;
	}
	if (gb_joblist == 0){
		gb_joblist = GB_create_container(gb_genetic,0,"job_list");
	}
	if (gb_tree_start == 0){
		gb_tree_start = GB_create_container(gb_genetic,0,"tree_start");
	}
	if (gb_tree_opt == 0){
		gb_tree_opt = GB_create_container(gb_genetic,0,"tree_opt");
	}
	if (gb_treeName == 0){
		gb_treeName = GB_create(gb_genetic,0,"treeName");
		GB_write_int(gb_treeName,0);
	}
	//
	// read presets
	GBDATA *gbp;
	if ( (gbp = GB_find(gb_presets,"max_cluster",NULL,down_level)) == 0) {
		new AP_ERR("GA_init","some preset not found");
		GB_pop_transaction(gb_main);
		return;
	}
	max_cluster = (int)GB_read_int(gbp);

	  if ( (gbp = GB_find(gb_presets,"maxTree",NULL,down_level)) == 0) {
	  new AP_ERR("GA_init","some preset not found");
	  GB_pop_transaction(gb_main);
	  return;
	  }
	  maxTree = (int)GB_read_int(gbp);

	if ( (gbp = GB_find(gb_presets,"max_jobs",NULL,down_level)) == 0) {
		new AP_ERR("GA_init","some preset not found");
		GB_pop_transaction(gb_main);
		return;
	}
	max_jobs = (int)GB_read_int(gbp);

	// allocate memory
	treelist = (long **)calloc((size_t)max_cluster+1,sizeof(long **));
	for (int i=0;i<max_cluster;i++) {
		treelist[i] = (long *)calloc((size_t)maxTree+1,sizeof(long *));
	}
	treePerCluster = (int *)calloc((size_t)max_cluster+1,sizeof(int));

	GB_pop_transaction(gb_main);
	//
	// open Filestream for output
	//
	fout = fopen("GAgeneticOutput","w");
	if (fout==0) new AP_ERR("GA_genetic::init","couldnt open Output File");

	return;
}

void GA_genetic::init_first(GBDATA *gbmain) {
	// makes protodb
	gb_main =gbmain;
		this->gb_main = gbmain;
	GB_push_transaction(gb_main);
	gb_genetic = GB_create_container(gb_main,0,"genetic");
	gb_presets = GB_create_container(gb_genetic,0,"presets");
	gb_tree_start = GB_create_container(gb_genetic,0,"tree_start");
	gb_tree_opt = GB_create_container(gb_genetic,0,"trees_opt");
	gb_joblist = GB_create_container(gb_genetic,0,"job_list");



	// write presets
	GBDATA *gbp;
	if ( (gbp = GB_create_container(gb_presets,0,"max_cluster")) == 0) {
		new AP_ERR("GA_init","some preset not found");
		GB_pop_transaction(gb_main);
		return;
	}
	GB_write_int(gbp,3);

	if ( (gbp = GB_create_container(gb_presets,0,"maxTree")) == 0) {
		new AP_ERR("GA_init","some preset not found");
		GB_pop_transaction(gb_main);
		return;
	}
		GB_write_int(gbp,4);
	if ( (gbp = GB_create_container(gb_presets,0,"max_jobs")) == 0) {
		new AP_ERR("GA_init","some preset not found");
		GB_pop_transaction(gb_main);
		return;
	}
	GB_write_int(gbp,5);

       	GB_pop_transaction(gb_main);

	tree_prototype = (AP_tree *) new AP_tree_nlen;
	return;
}

void GA_genetic::exit() {
}

AP_ERR * GA_genetic::read_presets() {
	GBDATA *gbp;
	if (gb_presets == 0)
		return new AP_ERR("GA_genetic","not inited");
	if ( (gbp = GB_find(gb_presets,"jobOpt",NULL,down_level)) == 0) {
		return new AP_ERR("GA_genetic","some preset not found");
	}
	jobOpt = (int)GB_read_int(gbp);
	if ( (gbp = GB_find(gb_presets,"jobCross",NULL,down_level)) == 0) {
		return new AP_ERR("GA_genetic","some preset not found");
	}
	jobCrossover = (int)GB_read_int(gbp);
	if ( (gbp = GB_find(gb_presets,"jobOther",NULL,down_level)) == 0) {
		return new AP_ERR("GA_genetic","some preset not found");
	}
	jobOther = (int)GB_read_int(gbp);

	if (gb_jobCount) jobCount = (int)GB_read_int(gb_jobCount);
	if (gb_bestTree) bestTree = GB_read_APfloat(gb_bestTree);
	if (gb_maxTree) maxTree = (int)GB_read_int(gb_maxTree);

	return 0;
}



double GA_genetic::AP_atof(char *str)
{
    double res = 0.0;
    double val = 1.0;
    register long neg = 0;
    register char c;
    register char *p = str;
    while (c= *(p++)){
	if (c == '.'){
	    val = .1;
	    continue;
	}
	if (c== '-') {
	    neg = 1;
	    continue;
	}
	if (val == 1.0) {
	    res*= 10.0;
	    res += c-'0';
	    continue;
	}
	res += (c-'0')*val;
	val *= .1;
    }
    if (neg) return -res;
    return res;
}

GBDATA *GA_genetic::get_cluster(GBDATA *container,int cluster) {
	GBDATA *gb_cluster;
	GBDATA *gb_anzahl;
	char clustername[20];
	if (cluster > max_cluster) {
		new AP_ERR("too large clusternumber",0);
	}
	if (container == 0) {
		new AP_ERR("get_cluster","container valid !");
		return 0;
	}
	sprintf(clustername,"cl_%d",cluster);
       	gb_cluster = GB_find(container,clustername,NULL,down_level);
	if (gb_cluster ==0) {
		printf("cl created\n");
		gb_cluster = GB_create_container(container,0,clustername);
		if (gb_cluster ==0) {
			new AP_ERR("get_cluster","Coldnt create cluster");
		}
		gb_anzahl = GB_create(gb_cluster,0,"count");
		GB_write_int(gb_anzahl,0);
	}
	return gb_cluster;
}


long GA_genetic::get_treeid(GBDATA *gbtree) {
	if (gbtree !=0) {
		GBDATA *gbid = GB_find(gbtree,"id",0,down_level);
		if (gbid) {
			return (int)GB_read_int(gbid);
		} else {
			new AP_ERR("No tree id in Database!");
		}
	}
	return -1;
}

GBDATA *GA_genetic::get_tree(GBDATA *container,long treeid) {
	// returns pointer to tree, if treeid == -1 retruns first tree in container
	GBDATA *gb_tree,*gbid;
	char treename[20];
	long id;

	if (container == 0) {
		new AP_ERR("get_tree","container valid !");
		return 0;
	}
       	gb_tree = GB_find(container,"tree",0,down_level);
	if (treeid >= 0) {
		while (gb_tree != 0) {
			gbid = GB_find(gb_tree,"id",0,down_level);
			id = GB_read_int(gbid);
			if (id == treeid) break;
			gb_tree = GB_find(gb_tree,"tree",0,this_level|search_next);
		}
	}
	return gb_tree;
}



/**********************************************************************

 read and write a tree in the database
 creating a string in which the tree is coded

**************************************************/

char *GA_genetic::write_tree_rek( AP_tree *node, char *dest, long mode)
	{
	char buffer[40];		/* just real numbers */
	char	*c1;
	if (node->is_leaf){
		if (mode == AP_PUT_DATA) {
			*(dest++) = 'L';
			if (node->name) strcpy(dest,node->name);
			while (c1= (char *)strchr(dest,1)) *c1 = 2;
			dest += strlen(dest);
			*(dest++) = 1;
			return dest;
		}else{
			if (node->name) return dest+1+strlen(node->name)+1;	/* N name term */
			return dest+1+1;
		}
	}else{
		sprintf(buffer,"%f,%f;",node->leftlen,node->rightlen);
		if (mode == AP_PUT_DATA) {
			*(dest++) = 'N';
			strcpy(dest,buffer);
			dest += strlen(buffer);
		}else{
			dest += strlen(buffer)+1;
		}
		dest = write_tree_rek(node->leftson,dest,mode);
		dest = write_tree_rek(node->rightson,dest,mode);
		return dest;
	}
}


/********************************************************************************************
					some tree read functions
********************************************************************************************/
AP_tree *GA_genetic::read_tree_rek(char **data)
	{
	AP_tree *node;
	char	c;
	char	*p1;

	node = (AP_tree *)new AP_tree_nlen;
	c = *((*data)++);
	if (c=='N') {
		p1 = (char *)strchr(*data,',');
		*(p1++) = 0;
    		node->leftlen = AP_atof(*data);
		*data = p1;
		p1 = (char *)strchr(*data,';');
		*(p1++) = 0;
		node->rightlen = AP_atof(*data);
		*data = p1;

		node->leftson = read_tree_rek(data);
		if (!node->leftson) {
			free((char *)node);
			return 0;
		}
		node->rightson = read_tree_rek(data);
		if (!node->rightson) {
			free((char *)node);
			return 0;
		}
		node->leftson->father = node;
		node->rightson->father = node;
	}else if (c=='L') {
		node->is_leaf = GB_TRUE;
		p1 = (char *)strchr(*data,1);
		*p1 = 0;
		node->name = (char *)strdup(*data);
		*data = p1+1;
	}else{
		new AP_ERR("GENETIC","Error reading tree 362436\n");
		return 0;
	}
	return node;
}

AP_ERR *GA_genetic::write_tree( GBDATA *gb_cluster, GA_tree *ga_tree)
{
	/* writes a tree to the database.
	   if there are GBDATA pointers in the inner nodes
	   the tree_name must be zero
	   to copy a tree call GB_copy((GBDATA *)dest,(GBDATA *)source);
	   */
	char	*treedata,*t_size;
	char    *treename;
	GBDATA	*gb_treedata;
	AP_ERR *ap_err = 0;
	GBDATA * gb_ref_count;
	GBDATA * gb_criteria;
	GBDATA * gb_tree;
	GBDATA * gb_id;

	treename = new char[20];
	if (!gb_cluster) {
		return new AP_ERR("write tree","no gbdata !");
	}
	/*if (ga_tree->id > maxTree *max_cluster) {
		return new AP_ERR("tree id too big!");
	}*/
	GB_push_transaction(gb_main);
       	if (ga_tree->id < 0) {
		ga_tree->id = GB_read_int(gb_treeName);
		GB_write_int(gb_treeName,1+ga_tree->id);
	}
	if ((gb_tree = get_tree(gb_cluster,ga_tree->id))==0) {
		 gb_tree = GB_create_container(gb_cluster,0,"tree");
	}

	gb_ref_count = GB_create(gb_tree,0,"ref_count");
	gb_criteria = GB_create(gb_tree,0,"criteria");
	gb_id = GB_create(gb_tree,0,"id");

	GB_write_int(gb_ref_count,ga_tree->ref_count);
	GB_write_APfloat(gb_criteria, ga_tree->criteria);
	GB_write_int(gb_id,ga_tree->id);

	gb_treedata = GB_search(gb_tree,"tree_data",GB_CREATE);
	t_size = write_tree_rek((AP_tree *)ga_tree->tree, 0, AP_GWT_SIZE);
	treedata= (char *)calloc(sizeof(char),(size_t)(t_size+1));
	t_size = write_tree_rek((AP_tree *)ga_tree->tree, treedata, AP_PUT_DATA);
	*(t_size) = 0;

	GB_write_string(gb_treedata,treedata);
	bestTree = GB_read_APfloat(gb_bestTree);
	if (ga_tree->criteria > bestTree) {
		bestTree = ga_tree->criteria;
		GB_write_APfloat(gb_bestTree,bestTree);
		GBT_write_tree(gb_main,0,0,(GBT_TREE *)ga_tree->tree);
	}
	free(treedata);
	delete treename;
	/*
	  gb_nnodes = GB_search(gb_tree,"nnodes",GB_CREATE);
	  error = GB_write_int(gb_nnodes,size);
	  if (error) return new AP_ERR(error,"");
	  */
	GB_pop_transaction(gb_main);
	return 0;
}


GA_tree *GA_genetic::read_tree(GBDATA *gb_cluster,long tree_id)
	/* read a tree
	   and removes it if no referenz is existing */
{

	char           *fbuf;
	GBDATA         *gb_treedata=0;
	GBDATA *gb_tree =0;
	GBDATA *gb_count=0;
	int count;
	char *cptr[1];
	GA_tree *tree;

	gb_count = GB_find(gb_cluster,"count",0,down_level);
	count = (int)GB_read_int(gb_count);

	gb_tree = get_tree(gb_cluster,tree_id);
	if(gb_tree == 0) {
			new AP_ERR("read_tree","this tree not found");
		}

	if (gb_tree ==0) {
		new AP_ERR("read_tree","tree not found");
		return 0;
	}

	tree = new GA_tree;
	GBDATA *gb_ref_count = GB_search(gb_tree,"ref_count",GB_FIND);
	GBDATA *gb_criteria = GB_search(gb_tree,"criteria",GB_FIND);
	GBDATA *gb_id = GB_search(gb_tree,"id",GB_FIND);

	if (gb_ref_count)
		tree->ref_count = (int)GB_read_int(gb_ref_count);
	if (gb_criteria)
		tree->criteria =GB_read_APfloat(gb_criteria);
	if (gb_id)
		tree->id = GB_read_int(gb_id);

	gb_treedata = GB_search(gb_tree,"tree_data",GB_FIND);
	if (gb_treedata) {
		fbuf =cptr[0] = GB_read_string(gb_treedata);
		tree->tree = (AP_tree_nlen *)read_tree_rek(cptr);
		free (fbuf);
	}
	delete_tree(gb_cluster,gb_tree);
	GBT_link_tree((GBT_TREE*)tree->tree,gb_main, 0, 0);
	return tree;
}



/********************************************************************************
*********************************************************************
		Funktionen fuer die genetischen algorithnen

**********************************************************************
******************************************************/

AP_ERR * GA_genetic::put_start_tree(AP_tree *tree,const long tree_id, int  cluster) {
	char * error = 0;
	AP_ERR *ap_err = 0;
	GBDATA * gb_cluster;
	GA_tree * ga_tree;
	GBDATA *gb_anzahl;
	int anzahl;

	GB_push_transaction(gb_main);
	if (gb_tree_start == 0) {
		gb_tree_start = GB_find(gb_genetic,"tree_start",0,down_level);
		if (gb_tree_start == 0) {
			gb_tree_start = GB_create_container(gb_genetic,0,"tree_start");
		}
	}
	if ( cluster > max_cluster) {
		GB_pop_transaction(gb_main);
		return new AP_ERR("put_start_tree","cluster unvalid");
	}

	if (gb_tree_start == 0) {
		GB_pop_transaction(gb_main);
		ap_err = new AP_ERR("Couldn't create container");
		return ap_err;
	}
	gb_cluster = get_cluster(gb_tree_start,cluster);

	if (gb_cluster ==0) {
		char clustername[20];
		sprintf(clustername,"cl_%d",cluster);
		gb_cluster = GB_create_container(gb_tree_start,0,clustername);
		if (gb_cluster ==0) {
			GB_pop_transaction(gb_main);
			return new AP_ERR("putStartTree","no cluster found");
		}
		gb_anzahl = GB_create(gb_cluster,0,"count");
		GB_write_int(gb_anzahl,0);
	}
	gb_anzahl = GB_find(gb_cluster,"count",0,down_level);
	anzahl = (int)GB_read_int(gb_anzahl);

	ga_tree = new GA_tree;
	ga_tree->ref_count = 0;
	ga_tree->criteria  = tree->mutation_rate;
	ga_tree->id = tree_id;
	ga_tree->tree = (AP_tree_nlen *)tree;

	ap_err = write_tree(gb_cluster,ga_tree);

	if (ap_err == 0) {
		anzahl ++;
		GB_write_int(gb_anzahl,anzahl);
	}

	delete ga_tree;
	GB_pop_transaction(gb_main);
	return ap_err;
}

GA_tree * GA_genetic::get_start_tree(int cluster) {
	GBDATA *gb_cluster;
	GBDATA *gb_anzahl;
	int anzahl;
	GA_tree *tree =0;
	GB_push_transaction(gb_main);
	gb_cluster = this->get_cluster(gb_tree_start,cluster);
        gb_anzahl = GB_find(gb_cluster,"count",0,down_level);
	anzahl = (int)GB_read_int(gb_anzahl);
	if (anzahl >= 1 ) {
		// gb_tree = GB_find(gb_cluster,"tree",0,down_level);
		GBDATA *gbt = get_tree(gb_cluster,-1);
		tree = this->read_tree(gb_cluster,-1);
		GB_pop_transaction(gb_main);
		return tree;
	}
	GB_pop_transaction(gb_main);
	return tree;
}




AP_ERR * GA_genetic::put_optimized(GA_tree *tree,int cluster) {
	GBDATA *gb_cluster;
       	int anzahl = 0;
	GBDATA *gb_anzahl;

	GB_push_transaction(gb_main);
	gb_cluster = this->get_cluster(gb_tree_opt,cluster);
	if (gb_cluster ==0) {
		char clustername[20];
		sprintf(clustername,"cl_%d",cluster);
		gb_cluster = GB_create_container(gb_tree_opt,0,clustername);
		if (gb_cluster ==0) {
			GB_pop_transaction(gb_main);
			return new AP_ERR("putStartTree","no cluster found");
		}
		gb_anzahl = GB_create(gb_cluster,0,"count");
		GB_write_int(gb_anzahl,0);
	}

	gb_anzahl = GB_find(gb_cluster,"count",0,down_level);
	anzahl = (int)GB_read_int(gb_anzahl);
	while (anzahl >= maxTree) {
		remove_job(gb_cluster);
	}

	// baum speichern und anzahl erhoehen
	// kreire jobs
	tree->id = -1;
	write_tree(gb_cluster,tree);
	anzahl ++;
	GB_write_int(gb_anzahl,anzahl);
	create_jobs(tree,cluster);
      	GB_pop_transaction(gb_main);
	return 0;
}


AP_ERR *GA_genetic::delete_tree(GBDATA *gb_cluster,GBDATA *gb_tree) {
	int ref_count =0;
	if (gb_tree == 0) {
		return new AP_ERR("delete_tree","no tree given");
	}

	GBDATA *gb_ref_count = GB_search(gb_tree,"ref_count",GB_FIND);
	if (gb_ref_count) {
		ref_count = (int)GB_read_int(gb_ref_count);
		if (ref_count > 0) {
			GB_write_int(gb_ref_count,ref_count - 1);
		} else {
			GB_delete(gb_tree);
			GBDATA *gb_count = GB_find(gb_cluster,"count",0,down_level);
			int count = (int)GB_read_int(gb_count);
			GB_write_int(gb_count,--count);
		}
	}
	return 0;
}

/**************************************************
Alles was mit jobs zu tuen hat
**************************************************/

GA_job * GA_genetic::get_job(int cluster) {
	int count;
	GBDATA * gb_cluster;
	GBDATA * gb_best_job;
	int bew =0;
	int best = 0;
	AP_FLOAT crit_best,crit_next;
	GA_job *job=0;
	GBDATA *gbp_job=0;
	GBDATA *gbp_criteria=0;

	GB_push_transaction(gb_main);
       	if (gb_joblist == 0) {
		new AP_ERR("get_job","no gbdata defined");
		GB_pop_transaction(gb_main);
		return 0;
	}

       	count = (int)GB_read_int(gb_jobCount); // globaler zaehler
	if (count <= 0) {
		new AP_ERR("get_job","no job");
		GB_pop_transaction(gb_main);
		return 0;
	}

	//
	// suche besten job aus cluster und liefer ihn zurueck
	//
	gb_cluster = get_cluster(gb_joblist,cluster);
	if (gb_cluster ==0) {
		new AP_ERR("no cluster found");
		GB_pop_transaction(gb_main);
		return 0;
	}
	GBDATA *gb_count = GB_find(gb_cluster,"count",0,down_level);
	count = (int)GB_read_int(gb_count); // globaler zaehler
	if (count <= 0) {
		new AP_ERR("get_job","no job");
		GB_pop_transaction(gb_main);
		return 0;
	}

	gb_best_job = gbp_job = GB_find(gb_cluster,"job",0,down_level);
	if (gbp_job != 0)
		gbp_criteria = GB_find(gbp_job,"criteria",0,down_level);

	if (gbp_criteria != 0)
		crit_best = GB_read_APfloat(gbp_criteria);

	while ((gbp_job = GB_find(gbp_job,"job",0,this_level|search_next)) !=0) {
		gbp_criteria = GB_find(gbp_job,"criteria",0,down_level);
		crit_next = GB_read_APfloat(gbp_criteria);
		if (crit_next > crit_best ) {
			crit_best = crit_next;
			gb_best_job = gbp_job;
		}
	}
    	if (gb_best_job == 0) {
		GB_pop_transaction(gb_main);
		return 0;
	}

	// lade baeume und loesche job
	// erhoehe refpointer
	job = new GA_job;
	job->criteria = crit_best;
	// decrement refcounter & delete trees;
	GBDATA *gbd = GB_find(gb_best_job,"cluster0",0,down_level);
        if (gbd) job->cluster0 = (int)GB_read_int(gbd);
	gbd = GB_find(gb_best_job,"cluster1",0,down_level);
	if (gbd) job->cluster1 = (int)GB_read_int(gbd);
	gbd = GB_find(gb_best_job,"id0",0,down_level);
	if (gbd) job->id0 = GB_read_int(gbd);
	gbd = GB_find(gb_best_job,"id1",0,down_level);
	if (gbd) job->id1 = GB_read_int(gbd);
	gbd = GB_find(gb_best_job,"modus",0,down_level);
	if (gbd) job->modus = (GA_JOB_MODE)GB_read_int(gbd);

	// finde baeume
	// Der erste Baum ist im selben Cluster !
	gb_cluster = get_cluster(gb_tree_opt,job->cluster0);
	job->tree0 = read_tree(gb_cluster,job->id0);

	if (job->id1 != -1 ){ // falls zweiter baum angegeben
		gb_cluster = get_cluster(gb_tree_opt,job->cluster1);
		job->tree1 = read_tree(gb_cluster,job->id1);
	}
	// loesche job in DB
	GB_delete(gb_best_job);
	GB_pop_transaction(gb_main);
	return job;
}


AP_ERR *GA_genetic::put_job(int cluster,GA_job *job) {
	if (cluster > max_cluster) {
		return new AP_ERR("put_job","wrong cluster");
	}
	GBDATA *gb_cluster;
	GBDATA *gb_jobcluster;
	GBDATA * gb_job;
	GBDATA *gb_ref= 0;
	GBDATA *gb_anzahl;
	long anzahl;
	AP_FLOAT crit0=0,crit1=0;
	AP_ERR *aperr = 0;
	char *err =0;
	GBDATA *gbp;

	if (job == 0) {
		return new AP_ERR("put_job","no job given !");
	}
	if (cluster != job->cluster0) {
		return new AP_ERR("put_job","internal Job error!");
	}
	GB_push_transaction(gb_main);
        // beide referenzcounter erhoehen
	gb_jobcluster = gb_cluster = get_cluster(gb_joblist,job->cluster0);
	gb_anzahl = GB_find(gb_cluster,"count",0,down_level);
	anzahl =GB_read_int(gb_anzahl);
	// Criteria Ausrechnen !!
	if (job->id1 >= 0) {
		GBDATA *gbcl = get_cluster(gb_tree_opt,job->cluster1);
		GBDATA *gbt  = get_tree(gbcl,job->id1);
		gbp = GB_find(gbt,"ref_count",0,down_level);
		int count = (int)GB_read_int(gbp) + 1;
		GB_write_int(gbp,count);
		gbp = GB_find(gbt,"criteria",0,down_level);
		crit1 = GB_read_APfloat(gbp);
	}

	if (job->id0 >= 0) {
		GBDATA *gbcl = get_cluster(gb_tree_opt,job->cluster0);
		GBDATA *gbt  = get_tree(gbcl,job->id0);
		gbp = GB_find(gbt,"ref_count",0,down_level);
		int count = (int)GB_read_int(gbp) + 1;
		GB_write_int(gbp,count);
		gbp = GB_find(gbt,"criteria",0,down_level);
		crit0 = GB_read_APfloat(gbp);
	}
	job->calcCrit(crit0,crit1);
	gb_job = GB_create_container(gb_jobcluster,0,"job");
	gbp = GB_search(gb_job,"criteria",GB_CREATE);
	GB_write_APfloat(gbp,job->criteria);
	gbp =  GB_search(gb_job,"modus",GB_CREATE);
	GB_write_int(gbp,(int)job->modus);
	gbp =  GB_search(gb_job,"cluster0",GB_CREATE);
	err = GB_write_int(gbp,job->cluster0);
	gbp =  GB_search(gb_job,"cluster1",GB_CREATE);
	err = GB_write_int(gbp,job->cluster1);
	gbp =  GB_search(gb_job,"id0",GB_CREATE);
	err = GB_write_int(gbp,job->id0);
	gbp =  GB_search(gb_job,"id1",GB_CREATE);
	err = GB_write_int(gbp,job->id1);

	if (err !=0) {
		aperr = new AP_ERR("put job","error while writing job to database");
		GB_abort_transaction(gb_main);
		return aperr;
	}

	jobCount = (int)GB_read_int(gb_jobCount);
	jobCount++;
	anzahl++;
	GB_write_int(gb_jobCount,jobCount);
	GB_write_int(gb_anzahl,anzahl);

	while (anzahl > maxTree) {
		cout << "cluster" << job->cluster0;
		remove_job(gb_jobcluster);
		anzahl = GB_read_int(gb_anzahl);
	}

	jobCount = (int)GB_read_int(gb_jobCount);

	while (jobCount >= max_jobs) {
		remove_job(0);
	}

	GB_pop_transaction(gb_main);
	job->printl();
	return aperr;
}


AP_ERR * GA_genetic::delete_job(GBDATA *gb_job) {
	// loesche die einzelnen tree referenzen
	// ggf. die trees
	GBDATA * gbp;
	GBDATA *gbt;
	GBDATA *gb_cluster,*gbcl;
	GA_job *job;
	job = new GA_job;
	GBDATA *jobcl;

	if (gb_job == 0)
		return new AP_ERR("delete_job","no job given !");
	jobcl = GB_get_father(gb_job);

        gbp = GB_find(gb_job,"cluster0",0,down_level);
        if (gbp) job->cluster0 = (int)GB_read_int(gbp);
	gbp = GB_find(gb_job,"cluster1",0,down_level);
	if (gbp) job->cluster1 = (int)GB_read_int(gbp);
	gbp = GB_find(gb_job,"id0",0,down_level);
	if (gbp) job->id0 = GB_read_int(gbp);
	gbp = GB_find(gb_job,"id1",0,down_level);
	if (gbp) job->id1 = GB_read_int(gbp);

	// finde baeume
	// Der erste Baum ist im selben Cluster !
	gbcl = gb_cluster = get_cluster(gb_tree_opt,job->cluster0);

	gbt = get_tree(gb_cluster,job->id0);
	delete_tree(gb_cluster,gbt);
	if (job->id1 != -1 ){ // falls zweiter baum angegeben
		gb_cluster = get_cluster(gb_tree_opt,job->cluster1);
		gbt = get_tree(gb_cluster,job->id1);
		delete_tree(gb_cluster,gbt);
	}
	GBDATA *gb_count = GB_find(jobcl,"count",0,down_level);
	int count = (int)GB_read_int(gb_count) - 1;
	GB_write_int(gb_count,count);

	GB_delete(gb_job);
	jobCount = (int)GB_read_int(gb_jobCount);
	jobCount--;
	GB_write_int(gb_jobCount,jobCount);
	delete job;
	return 0;
}

AP_ERR * GA_genetic::remove_job(GBDATA *gb_cluster) {
	// find worst job in cluster (any if gb_cluster == 0)
	// an delet it
	GBDATA *gbjob;
	GBDATA *gbworst = 0;
	GBDATA *gbp_criteria;
	GBDATA *gb_anzahl;
	AP_FLOAT crit_next,crit_worst;
	int cl,anzahl =0,safty = 0;

	if (gb_cluster == 0) {
		while (gb_cluster == 0) {
			safty ++;
			cl = (int)random()%max_cluster;
			gb_cluster = get_cluster(gb_joblist,cl);
			gb_anzahl = GB_find(gb_cluster,"count",0,down_level);
			anzahl = (int)GB_read_int(gb_anzahl);
			if (anzahl <= min_job) {
				if (safty > GA_SAFETY) {
					new AP_ERR("remove job","get looped");
				} else {
					gb_cluster = 0;
					}
			}
		}
	}

	gbjob = GB_find(gb_cluster,"job",0,down_level);

	if (gbjob != 0) {
		gbp_criteria = GB_find(gbjob,"criteria",0,down_level);
		if (gbp_criteria != 0) {
			crit_worst = GB_read_APfloat(gbp_criteria);
			gbworst = gbjob;
		}
		while ((gbjob = GB_find(gbjob,"job",0,this_level|search_next)) !=0) {
			gbp_criteria = GB_find(gbjob,"criteria",0,down_level);
			crit_next = GB_read_APfloat(gbp_criteria);
			if (crit_next > crit_worst ) {
				crit_worst = crit_next;
				gbworst = gbjob;
			}
		}
	}
    	if (gbworst == 0) {
		cout << "Ccluster" << cl;
		return new AP_ERR("remove_job","No job found");
	}

	delete_job(gbworst);
	return 0;
}



AP_ERR *GA_genetic::create_jobs(GA_tree *tree,int cluster) {
	// generiert jobs fuer einen Baum und
	// speichert diese in der Datenbank
	int i,t,jc=0;
	GA_job *job;
	if (tree == 0)
		return new AP_ERR("create_jobs","no tree given");

	read_presets();

	/*
	  for (i=0;i<jobOpt;i++){	// Optimierungsjobs
	  job = new GA_job;
	  job->cluster0 = cluster;
	  job->id0 = tree->id;
	  job->cluster1 = -1;
	  job->id1 = -1;
	  job->modus = GA_KERNIGHAN;
	  if (put_job(job->cluster0,job) !=0)
	  ;
	  delete job;
	  }
	  // create list of all trees in clusters
	  // and count number of them
	  */
	GBDATA *gbt;
	GBDATA *gbcl;
	GBDATA *gbc;
	int count,treeid,treecount=0;


	for(i =0;i<max_cluster;i++) {
		cout << "\n" << i << " : ";
		gbcl = get_cluster(gb_tree_opt,i);
		if (gbcl != 0) {
			gbc = GB_find(gbcl,"count",0,down_level);
			if (gbc) count = (int)GB_read_int(gbc);
			gbt = GB_find(gbcl,"tree",0,down_level);
			for (t=0;t<maxTree;t++) {
				if (gbt != 0) {
					treecount ++;
					treelist[i][t] = get_treeid(gbt);
					cout << " " << treelist[i][t];
					gbt = GB_find(gbt,"tree",0,this_level|search_next);
				} else break;
			}
			cout << " ** " << t << " count " << count;
			treePerCluster[i] = t;
			gbcl = GB_find(gbcl,"cl*",0,this_level|search_next);
		} else break;
	}
	clusterCount = i;
	cout << "\n";
	cout.flush();

	for (i=0;i<jobCrossover;i++){	// Crossoverjobs
		if (treePerCluster[cluster] > 1) { // in same cluster
			job = new GA_job;
			job->cluster0 = cluster;
			job->id0 = tree->id;
			job->cluster1 = cluster;
				// choose random tree
			treeid = (int)random()%treePerCluster[cluster];
			while (treelist[cluster][treeid] == job->id0) {
				treeid = (int)random()%treePerCluster[cluster];
			};
				// remove it from treelist by a swap
			job->id1 = treelist[cluster][treeid];
			treelist[cluster][treeid] =
				treelist[cluster][treePerCluster[cluster]-1];
			treePerCluster[cluster] --;
			job->modus = GA_CROSSOVER;
			if (put_job(job->cluster0,job) ==0)
			     jc++;
			delete job;
		}
	}

	int rcl;	// random cluster

	for (i=0;i<jobCrossover;i++){	// Crossoverjobs
		if (clusterCount <= 1) break;
		if (treecount <= treePerCluster[cluster]) break;
		// in same cluster
		job = new GA_job;
		job->cluster0 = cluster;
		job->id0 = tree->id;
				// search random cluster other than cluster
		rcl = (int)random()%clusterCount;

		int safty = 0;
		while ((rcl == cluster) || (treePerCluster[rcl] < 1) )  {
			safty ++;
			rcl = (int)random()%clusterCount;
		if (safty>GA_SAFETY) break;
		}
		if (safty >GA_SAFETY) {
			delete job;
			break;
		}
		job->cluster1 = rcl;
		treeid = (int)random()%treePerCluster[rcl];
		//	while (treelist[rcl][treeid] == job->id0) {
			treeid = (int)random()%treePerCluster[rcl];

		//};

		// remove it from treelist by a swap
		job->id1 = treelist[rcl][treeid];
		treelist[rcl][treeid] =
			treelist[rcl][treePerCluster[rcl]-1];
		treePerCluster[rcl] --;
		job->modus = GA_CROSSOVER;
		if (put_job(job->cluster0,job) ==0)
			jc++;
		delete job;
	}
	if (jc < 1) {	// create jobs falls keine anderen aufgetaucht sind
		job = new GA_job;
		job->cluster0 = cluster;
		job->id0 = tree->id;
		job->id1 = job->cluster1 = -1;
		job->modus = GA_CREATEJOBS;
		put_job(job->cluster0,job);
	}
	return 0;
}


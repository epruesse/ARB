GB_ERROR save_Pool(const container_group_ptr& con,int generation) {
    string filename;
    {
        char tname[200];
        time_t t;
        struct tm *tm;
        
        time(&t);
        tm = localtime(&t);        
        strftime(tname, 200, "%Y%b%d_%H%M%S", tm);
        
        filename = string("./genoms_save_")+tname+".arb";
    }
    
#if defined(DEBUG)
    printf("filename='%s'\n", filename.c_str());
#endif // DEBUG    

    /*-------------------------------------------------------------------------------------------*/
    /*---------------------Structure of Genom Information Database-------------------------------*/
    /*-------------------------------------------------------------------------------------------*/

    GBDATA *save_main = GB_open(filename.c_str(), "wct");
    GB_ERROR error = 0;
    
    if (!save_main) {
        error = GB_export_error("Database %s couldn't be created",filename.c_str());
    }
   
    else {
        GB_transaction dummy(save_main);
        
        GBDATA *organisms_container = GB_create_container(save_main, "organisms");
         
       
	if (organisms_container) {	
	   GBDATA *field;
	   field = GB_create(organisms_container, "number_of_entrees", GB_INT);//number of Genoms in database
	   field = GB_create(organisms_container, "last_updated", GB_INT);
	   
	   
	   GBDATA *organism = GB_create(organisms_container, "organism");
	   {
	     if (organism) {
            
                    GBDATA *field;
                    
                    field = GB_create(organism, "accession", GB_STRING);
                    GB_write_string(field, g->get_gr()->get_inf().accession);
                    
                    field = GB_create(organism, "entree_definition", GB_STRING); /*description of sequence entree*/
                    GB_write_string(field, g->get_gr()->get_inf().entreedefinition);
                    
                    field = GB_create(organism, "species", GB_STRING);
                    GB_write_string(field, g->get_gr()->get_inf().species);

                    field = GB_create(organism, "strain", GB_STRING);
                    GB_write_string(field, g->get_gr()->get_def().strain);
		    
                    
                   
		    GBDATA *genes_container = GB_create_container(organism, "genes");
                    if (genes_container) {
                        const PG_species_container& sp = g->get_gr()->get_li();
                        
                        for (PG_genes_container::const_iterator s=sp.begin(); !error && s!=sp.end(); ++s) {
                            GBDATA *species = GB_create(species_container, "name", GB_STRING);
                            GB_write_string(species, s->get_name(error));
                        }
                    }
                    
                    GBDATA *probe_container = GB_create_container(group, "probes");
                    if (probe_container) {
                        const PG_sequence_cont& pr = g->get_gr()->get_inf().seq_list;
                        for (PG_sequence_cont::const_iterator p=pr.begin(); !error && p!=pr.end(); ++p) {
                            GBDATA *probe = GB_create(probe_container, "data", GB_STRING);
                            GB_write_string(probe, p->c_str());
                        }
                        GBDATA *probe = GB_create(probe_container, "history", GB_STRING);
                        GB_write_string(probe, g->get_gr()->get_history().c_str());
                    }                    
                }
                if (!error) error = GB_get_error();
            }
            
        }
        
        if (!error) error = GB_get_error();    
        if (!error) error = GB_save(save_main, (char*)filename.c_str(), "a");
        
        GB_close(save_main);
    }   
    
    if (!error) error = GB_get_error();    
    return error;
}

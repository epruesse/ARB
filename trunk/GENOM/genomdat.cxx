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
        
        GBDATA */*organisms_container = GB_create_container(save_main, "organisms");*/
        organisms_container = GB_search(save_main, "organisms", GB_CREATE_CONTAINER);
         
       
        if (organisms_container) {	
            GBDATA *field;
            //field = GB_create(organisms_container, "number_of_entrees", GB_INT);//number of Genoms in database
            field = /*GB_create(organisms_container, "last_update", GB_INT);*/ GB_search(organisms_container, "last_update", GB_CREATE); // typlos?
            GB_write_int(field, ...........................);
	   
            
            char acc_num = ...;
            
            // @@@ test ob organism mit acc_num bereits existiert!            
            bool exist_acc = false;
            
            if (!exist_acc) {
                GBDATA *organism = GB_create(organisms_container, "organism");
                {
                    if (organism) {
            
                        GBDATA *field;
                    
                        field = GB_create(organism, "accession", GB_STRING);
                        GB_write_string(field, /*string*/);                    
                    
                        field = GB_create(organism, "entree_definition", GB_STRING); /*description of sequence entree*/
                        GB_write_string(field, entreedefinition);
                    
                        field = GB_create(organism, "full_name", GB_STRING);
                        GB_write_string(field, species);
                    
                        field = GB_create(organism,  "name",  GB_STRING);
                        GB_write_string(field, ...........................);
                    
                        field = GB_create(organism, "strain", GB_STRING);
                        GB_write_string(field, g->get_gr()->get_def().strain);
                    
                        field = GB_create(organism,  "sequence",  GB_STRING);
                        GB_write_string(field,  ...........................);
                    
                    
                    
                   
                        GBDATA *genomfragments_container = GB_create_container(organism, "fragments"); 
                        if (genomfragments_container) {                        
                            while (fragment=...) {
                                GBDATA *fragment_container = GB_create_container(organism, "frag"); 
                            
                                GBDATA *species = GB_create(species_container, "name", GB_STRING);
                                GB_write_string(species, );
                            }
                        }
                    
                        if (!error) error = GB_get_error();
                    }
            
                }
            }
        
        if (!error) error = GB_get_error();    
        if (!error) error = GB_save(save_main, (char*)filename.c_str(), "a");
        
        GB_close(save_main);
    }   
    
    if (!error) error = GB_get_error();    
    return error;
}

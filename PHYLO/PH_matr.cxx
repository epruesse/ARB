#include "phylo.hxx"
#include <aw_global.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <cstring>

#warning module completely unused

extern void ph_view_matrix_cb(AW_window *);


#define CHECK_NAN(x) if ( (!(x>=0.0)) && (!(x<0.0))  ) *(int *)0=0;


static AP_smatrix *global_ratematrix = 0;


void set_globel_r_m_value(AW_root *aw_root, long i, long j) {
    char buffer[256];
    sprintf(buffer,"phyl/ratematrix/val_%li_%li",i,j);
    global_ratematrix->set(i,j,aw_root->awar(buffer)->read_float());
}

void create_matrix_variables(AW_root *aw_root, AW_default def)
{
    aw_root->awar_string( "tmp/dummy_string","0",def);


    aw_root->awar_string( "phyl/which_species","marked",def);
    aw_root->awar_string( "phyl/alignment","",def);

    aw_root->awar_string( "phyl/filter/alignment","none",def);
    aw_root->awar_string( "phyl/filter/name","none",def);
    aw_root->awar_string( "phyl/filter/filter","",def);

    aw_root->awar_string( "phyl/weights/name","none",def);
    aw_root->awar_string( "phyl/weights/alignment","none",def);

    aw_root->awar_string( "phyl/rates/name","none",def);
    aw_root->awar_string( "phyl/cancel/chars",".",def);
    aw_root->awar_string( "phyl/correction/transformation","none",def);

    aw_root->awar("phyl/filter/alignment")->map("phyl/alignment");
    aw_root->awar("phyl/weights/alignment")->map("phyl/alignment");

    aw_create_selection_box_awars(aw_root, "tmp/phyl/save_matrix", ".", "", "infile", def);

    aw_root->awar_string( "phyl/tree/tree_name","tree_temp",def);

    aw_root->awar_float( "phyl/alpha",1.0,def);

    global_ratematrix = new AP_smatrix(AP_MAX);

    long i,j;
    for (i=0; i <AP_MAX; i++){
        for (j=0; j <AP_MAX; j++){
            if (i!=j) global_ratematrix->set(i,j,1.0);
        }
    }
    for (i=AP_A; i <AP_MAX; i*=2){
        for (j = AP_A; j < i; j*=2){
            char buffer[256];
            sprintf(buffer,"phyl/ratematrix/val_%li_%li",i,j);
            aw_root->awar_float(buffer, 1.0, def);
            aw_root->awar(buffer)->add_callback((AW_RCB)set_globel_r_m_value,(AW_CL)i,(AW_CL)j);
        }
    }
}




void ph_calculate_matrix_cb(AW_window *aww,AW_CL cb1,AW_CL cb2)  
{ AWUSE(cb1); AWUSE(cb2);
 char *cancel,*transformation;

 if(!PHDATA::ROOT)
 { aw_message("data_base not opened !!");
 return;
 }
 AW_root *aw_root = aww->get_root();

 PH_TRANSFORMATION trans = PH_TRANSFORMATION_NONE;
 // transformation = aw_root->awar("phyl/correction/transformation")->read_string();
 transformation=strdup(PH_TRANSFORMATION_JUKES_CANTOR_STRING);

 //  double alpha = aw_root->awar("phyl/alpha")->read_float();
 double alpha = 1.0;
 cancel = aw_root->awar("phyl/cancel/chars")->read_string();
 printf("\ntransformation: %s",transformation);
 printf("\ncancel: %s\n",cancel);
 if (!strcmp(transformation,PH_TRANSFORMATION_JUKES_CANTOR_STRING))
     trans = PH_TRANSFORMATION_JUKES_CANTOR;
 else if (!strcmp(transformation,PH_TRANSFORMATION_KIMURA_STRING))
     trans = PH_TRANSFORMATION_KIMURA;
 else if (!strcmp(transformation,PH_TRANSFORMATION_TAJIMA_NEI_STRING))
     trans = PH_TRANSFORMATION_TAJIMA_NEI;
 else if (!strcmp(transformation,PH_TRANSFORMATION_TAJIMA_NEI_PAIRWISE_STRING))
     trans = PH_TRANSFORMATION_TAJIMA_NEI_PAIRWISE;
 else if (!strcmp(transformation,PH_TRANSFORMATION_BANDELT_STRING))
     trans = PH_TRANSFORMATION_BANDELT;
 else if (!strcmp(transformation,PH_TRANSFORMATION_BANDELT_JC_STRING))
     trans = PH_TRANSFORMATION_BANDELT_JC;
 else if (!strcmp(transformation,PH_TRANSFORMATION_BANDELT2_STRING))
     trans = PH_TRANSFORMATION_BANDELT2;
 else if (!strcmp(transformation,PH_TRANSFORMATION_BANDELT2_JC_STRING))
     trans = PH_TRANSFORMATION_BANDELT2_JC;

 GB_ERROR error = PHDATA::ROOT->calculate_matrix(".-",alpha,trans);

 if (error) aw_message(error);
 delete transformation;
 delete cancel;
 if(!error) ph_view_matrix_cb(aww);  // call callback directly to see what you did
}

void ph_save_matrix_cb(AW_window *aww)
{           // save the matrix
    if (!PHDATA::ROOT) return;
    PHDATA::ROOT->print();
    char *filename = aww->get_root()->awar("tmp/phyl/save_matrix/file_name")->read_string();
    GB_ERROR error = PHDATA::ROOT->save(filename);
    delete filename;
    if (error) aw_message(error);
    aww->hide();
}

AW_window *create_save_matrix_window(AW_root *aw_root, char *base_name)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "SAVE_MATRIX", "SAVE MATRIX TO FILE");
    aws->load_xfig("sel_box.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("save");aws->callback(ph_save_matrix_cb);
    aws->create_button("SAVE","SAVE","S");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("cancel");
    aws->create_button("CANCEL","CANCEL","C");

    awt_create_selection_box((AW_window *)aws,base_name);

    return (AW_window *)aws;
}


AW_window *awt_create_select_cancel_window(AW_root *aw_root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "CANCEL_SELECT", "CANCEL SELECT");
    aws->load_xfig("phylo/cancel.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("cancel");
    aws->create_input_field("phyl/cancel/chars",12);

    return (AW_window *)aws;
}


AW_window *create_matrix_window(AW_root *aw_root)
{ AW_window_simple *aws = new AW_window_simple;
 aws->init( aw_root, "DISTANCE_TABLE", "DistanceTable");
 aws->load_xfig("phylo/matrix.fig");
 aws->button_length( 10 );

 aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
 aws->create_button("CLOSE","CLOSE","C");

 aws->at("point_opts");
 aws->create_option_menu("phyl/matrix/point","'.'","0");
 aws->insert_option("forget pair for distancematrix","0",0);
 aws->insert_option("use distancetable","0",1);
 aws->update_option_menu();

 aws->at("minus_opts");
 aws->create_option_menu("phyl/matrix/minus","'-'","0");
 aws->insert_option("forget pair for distancematrix","0",0);
 aws->insert_option("use distancetable","0",1);
 aws->update_option_menu();

 aws->at("rest_opts");
 aws->create_option_menu("phyl/matrix/rest", "ambiguity codes","0");
 aws->insert_option("forget pair for distancematrix","0",0);
 aws->insert_option("use distancetable","0",1);
 aws->update_option_menu();

 aws->at("lower_opts");
 aws->create_option_menu("phyl/matrix/lower","lowercase chars","0");
 aws->insert_option("forget pair for distancematrix","0",0);
 aws->insert_option("use distancetable","0",1);
 aws->update_option_menu();

 return (AW_window *)aws;
}

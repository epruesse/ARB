#include <stdio.h>
#include <stdlib.h>	// system
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>
#include <iostream.h>
#include <string.h>

extern GBDATA *gb_main;

void aed_start_naligning(AW_window *aw) {
    AW_root *root = aw->get_root();
    char	*buffer;
    int	i,j;

    void *strstruct = GBS_stropen(1000);
    GBS_strcat(strstruct,"xterm -sl 1000 -sb -e sh -c 'LD_LIBRARY_PATH=\"");
    GBS_strcat(strstruct,GB_getenv("LD_LIBRARY_PATH"));
    GBS_strcat(strstruct,"\";export LD_LIBRARY_PATH;for i in ");
    if ( root->awar("naligner/what")->read_int() ) {
        GB_transaction dummy(gb_main);
        for (GBDATA *gb_species = GBT_first_marked_species(gb_main);
             gb_species;
             gb_species = GBT_next_marked_species(gb_species)){
            char *name = GBT_read_name(gb_species);
            GBS_strcat(strstruct,"\"");
            GBS_strcat(strstruct,name);
            GBS_strcat(strstruct,"\" ");
            delete name;
        }
    }else{
        char *species_name = root->awar( AWAR_SPECIES_NAME )->read_string();
        GBS_strcat(strstruct,"\"");
        GBS_strcat(strstruct,species_name);
        GBS_strcat(strstruct,"\" ");
        delete species_name;
    }


    GBS_strcat(strstruct,"; do arb_naligner");


    if ( root->awar("naligner/against")->read_int() ) {
        GBS_strcat(strstruct," -PARB_PT_SERVER");
        GBS_intcat(strstruct,	root->awar( "naligner/pt_server" )->read_int()	);
    }else{
        GBS_strcat(strstruct," \"-f");
        char *family = root->awar("naligner/sagainst")->read_string();
        GBS_strcat(strstruct,family);
        delete family;
        GBS_strcat(strstruct,"\"");
    }
    GBS_strcat(strstruct," \"-s$i\" ");

    if (root->awar( "naligner/mark_profile" )->read_int() ) GBS_strcat(strstruct," -mf");
    if (root->awar( "naligner/unmark_sequence" )->read_int() ) GBS_strcat(strstruct," -us");

    GBS_strcat(strstruct," -minf"); 	GBS_intcat(strstruct,root->awar("naligner/minf")->read_int());
    GBS_strcat(strstruct," -maxf"); 	GBS_intcat(strstruct,root->awar("naligner/maxf")->read_int());
    GBS_strcat(strstruct," -minw"); 	GBS_floatcat(strstruct,root->awar("naligner/minw")->read_float());
    GBS_strcat(strstruct," -maxew"); 	GBS_floatcat(strstruct,root->awar("naligner/maxew")->read_float());
    GBS_strcat(strstruct," -ib"); 	GBS_intcat(strstruct,root->awar("naligner/det/ib")->read_int());
    GBS_strcat(strstruct," -ic"); 	GBS_intcat(strstruct,root->awar("naligner/det/ic")->read_int());

    GBS_strcat(strstruct," -cl"); 	GBS_floatcat(strstruct,	root->awar("naligner/det/cl")->read_float());




    GBS_strcat(strstruct," -cm"); 	GBS_floatcat(strstruct,root->awar("naligner/det/cm")->read_float());
    GBS_strcat(strstruct," -ch"); 	GBS_floatcat(strstruct,root->awar("naligner/det/ch")->read_float());
    GBS_strcat(strstruct," -mgf"); 	GBS_floatcat(strstruct,root->awar("naligner/igap_panelty")->read_float());
    GBS_strcat(strstruct," -mma1");
    GBS_strcat(strstruct," -msub");
    for (i=0;i<5;i++){
        for (j=0;j<5;j++) {
            char var[100];
            if ( i==4 || j==4 ) {		/* gap panelty */
                if (i==4 && j==4) {
                    GBS_floatcat(strstruct,0.0);
                }else{
                    GBS_floatcat(strstruct,root->awar("naligner/gap_panelty")->read_float());
                    GBS_chrcat(strstruct,',');
                }
            }else{
                if (i<j) sprintf(var,"naligner/%c%c","acgt-"[i],"acgt-"[j]);
                else	sprintf(var,"naligner/%c%c","acgt-"[j],"acgt-"[i]);
                GBS_floatcat(strstruct,root->awar(var)->read_float());
                if (i<4 || j<4) GBS_chrcat(strstruct,',');
            }
        }
    }
    GBS_strcat(strstruct," || echo \"Aligner failed\";done;");


    GBS_strcat(strstruct,"echo press \"(return)\" to close window;read a' &");
    buffer = GBS_strclose(strstruct);
    system(buffer);
    printf("%s\n",buffer);
    delete buffer;
}


void create_naligner_variables(AW_root *root,AW_default db1)
{
    root->awar_int(     "naligner/what", 0 ,    db1);

    root->awar_int(     "naligner/against", 0 ,    db1);
    root->awar_string(  "naligner/sagainst", "" ,    db1);
    root->awar_int(     "naligner/pt_server", -1 ,    db1);

    root->awar_int(     "naligner/mark_profile", 1 ,       db1);
    root->awar_int(     "naligner/unmark_sequence", 0 ,       db1);


    root->awar_float(   "naligner/aa", 0.0 ,    db1);
    root->awar_float(   "naligner/ac", 3.0 ,    db1);
    root->awar_float(   "naligner/ag", 1.0 ,    db1);
    root->awar_float(   "naligner/at", 3.0 ,    db1);
    root->awar_float(   "naligner/cc", 0.0 ,    db1);
    root->awar_float(   "naligner/cg", 3.0 ,    db1);
    root->awar_float(   "naligner/ct", 1.0 ,    db1);
    root->awar_float(   "naligner/gg", 0.0 ,    db1);
    root->awar_float(   "naligner/gt", 3.0 ,    db1);
    root->awar_float(   "naligner/tt", 0.0 ,    db1);
    root->awar_float(   "naligner/gap_panelty", 5.0	 ,    db1);
    root->awar_float(   "naligner/igap_panelty", 0.2 ,    db1);
    root->awar_int(	"naligner/minf", 3 ,    db1);
    root->awar_int(	"naligner/maxf", 30 ,    db1);


    root->awar_float(	"naligner/minw", .7 ,    db1);
    root->awar_float(	"naligner/maxew", .2 ,    db1);

    root->awar_float(	"naligner/det/cl", .25 ,    db1);
    root->awar_float(	"naligner/det/cm", .5 ,    db1);
    root->awar_float(	"naligner/det/ch", .8 ,    db1);
    root->awar_int(	"naligner/det/ib", 5 ,    db1);
    root->awar_int(	"naligner/det/ic", 5 ,    db1);

}

void ed_nalign_save(AW_window *aww)
{
    AW_root *root = aww->get_root();
    root->save_default("naligner/aa");
}

AW_window *create_expert_naligner_window(AW_root *root)
{
    const	int	mwidth = 5;
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "ALIGNER_V2_EXPERT2", "ALIGNER V2.0 EXPERT 2");
    aws->load_xfig("ed_al_ex.fig");

    aws->at("close");
    aws->callback     ( (AW_CB0)AW_POPDOWN  );
    aws->create_button( "CLOSE", "CLOSE", "C" );

    aws->at("save");
    aws->callback(ed_nalign_save);
    aws->create_button("SAVE", "SAVE","S");

    aws->at("minw");
    aws->create_input_field("naligner/minw",mwidth);

    aws->at("maxew");
    aws->create_input_field("naligner/maxew",mwidth);

    aws->at("ib");
    aws->create_input_field("naligner/det/ib",mwidth);

    aws->at("ic");
    aws->create_input_field("naligner/det/ic",mwidth);

    aws->at("cl");
    aws->create_input_field("naligner/det/cl",mwidth);

    aws->at("cm");
    aws->create_input_field("naligner/det/cm",mwidth);

    aws->at("ch");
    aws->create_input_field("naligner/det/ch",mwidth);

    return (AW_window *)aws;
}

AW_window *create_special_naligner_window(AW_root *root, AW_CL cd2)
{
    AW_window_simple *aws = new AW_window_simple;
    const	int	mwidth = 3;
    aws->init( root, "ALIGNER_V2_EXPERT", "ALIGNER V2.0 EXPERT");
    aws->load_xfig("ed_al_sp.fig");

    AWUSE(cd2);
    aws->label_length(22);

    aws->at("close");
    aws->callback     ( (AW_CB0)AW_POPDOWN  );
    aws->create_button( "CLOSE", "CLOSE", "C" );

    aws->at("minr");
    aws->create_input_field( "naligner/minf", 6);

    aws->at("maxr");
    aws->create_input_field( "naligner/maxf", 6);

    aws->at("aa");
    aws->create_input_field("naligner/aa",mwidth);
    aws->at("ac");
    aws->create_input_field("naligner/ac",mwidth);
    aws->at("ag");
    aws->create_input_field("naligner/ag",mwidth);
    aws->at("at");
    aws->create_input_field("naligner/at",mwidth);

    aws->at("ca");
    aws->create_input_field("naligner/ac",mwidth);
    aws->at("cc");
    aws->create_input_field("naligner/cc",mwidth);
    aws->at("cg");
    aws->create_input_field("naligner/cg",mwidth);
    aws->at("ct");
    aws->create_input_field("naligner/ct",mwidth);

    aws->at("ga");
    aws->create_input_field("naligner/ag",mwidth);
    aws->at("gc");
    aws->create_input_field("naligner/cg",mwidth);
    aws->at("gg");
    aws->create_input_field("naligner/gg",mwidth);
    aws->at("gt");
    aws->create_input_field("naligner/gt",mwidth);

    aws->at("ta");
    aws->create_input_field("naligner/at",mwidth);
    aws->at("tc");
    aws->create_input_field("naligner/ct",mwidth);
    aws->at("tg");
    aws->create_input_field("naligner/gt",mwidth);
    aws->at("tt");
    aws->create_input_field("naligner/tt",mwidth);

    aws->at("gap");
    aws->create_input_field("naligner/gap_panelty",4);

    aws->at("igap");
    aws->create_input_field("naligner/igap_panelty",4);

    aws->at("expert");
    aws->callback((AW_CB1)AW_POPUP,(AW_CL)create_expert_naligner_window);
    aws->create_button("EXPERT_OPTIONS", "EXPERT2","E");

    aws->at("save");
    aws->callback(ed_nalign_save);
    aws->create_button("SAAVE", "SAVE","S");

    return (AW_window *)aws;
}

AW_window *create_naligner_window( AW_root *root, AW_CL cd2 ) {

    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "ALIGNER_V2","ALIGNER V2.0");
    aws->load_xfig("awt/align.fig");

    AWUSE(cd2);

    aws->label_length( 10 );
    aws->button_length( 10 );

    aws->at( "close" );
    aws->callback     ( (AW_CB0)AW_POPDOWN  );
    aws->create_button( "CLOSE", "CLOSE", "O" );

    aws->at( "help" );
    aws->callback     ( AW_POPUP_HELP, (AW_CL) "ne_align_seq.hlp"  );
    aws->create_button( "HELP", "HELP" );

    aws->at( "align" );
    aws->callback     ( aed_start_naligning );
    aws->highlight();
    aws->create_button( "GO", "GO", "G");

    aws->at( "expert" );
    aws->callback     ( (AW_CB1)AW_POPUP,(AW_CL)create_special_naligner_window );
    aws->create_button( "OPTIONS", "PARAMETERS", "E");

    aws->at("what");
    aws->create_toggle_field("naligner/what","Align","A");
    aws->insert_toggle("Selected Species:","S",0);
    aws->insert_default_toggle("Marked Species","M",1);
    aws->update_toggle_field();

    aws->at( "swhat" );
    aws->create_input_field( AWAR_SPECIES_NAME, 2);

    aws->at("against");
    aws->create_toggle_field("naligner/against","Reference","g");
    aws->insert_toggle("Species by name","S",0);
    aws->insert_default_toggle("Auto search by pt_server","S",1);
    aws->update_toggle_field();

    aws->at( "sagainst" );
    aws->create_input_field( "naligner/sagainst", 2);

    aws->label_length( 25 );

    aws->at( "pt_server" );
    aws->label("PT_SERVER:");
    awt_create_selection_list_on_pt_servers(aws, "naligner/pt_server", AW_TRUE);


//     aws->create_option_menu( "naligner/pt_server", "PT_SERVER:" , "" );
//     {
//         int i;
//         char *server;
//         const char *file;
//         char search_for[256];
//         char choice[256];
//         char *fr;

//         for (i=0;i<1000;i++) {
//             sprintf(search_for,"ARB_PT_SERVER%i",i);
//             // @@@ FIXME: PT_SERVER_CHOICE

//             server = GBS_read_arb_tcp(search_for);
//             if (!server) break;
//             fr = server;
//             file = server;				/* i got the machine name of the server */
//             if (*file) file += strlen(file)+1;	/* now i got the command string */
//             if (*file) file += strlen(file)+1;	/* now i got the file */
//             if (strrchr(file,'/')) file = strrchr(file,'/')-1;
//             static char empty[] = "";
//             if (!(server = strtok(server,":"))) server = empty;
//             sprintf(choice,"%s: %s",server,file+2);
//             aws->insert_option( choice, "", i );
//             delete fr;
//         }
//     }
//     aws->insert_default_option( "select a PT_SERVER", "d", -1);
//     aws->update_option_menu();

    aws->at( "mark" );
    aws->label_length(40);
    aws->label("Mark sequences found by the pt_server");
    aws->create_toggle( "naligner/mark_profile");

    return (AW_window *)aws;
}


//  ==================================================================== //
//                                                                       //
//    File      : awt_config_manager.hxx                                 //
//    Purpose   :                                                        //
//    Time-stamp: <Wed Jan/09/2002 19:04 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in January 2002          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef AWT_CONFIG_MANAGER_HXX
#define AWT_CONFIG_MANAGER_HXX

typedef char *(*AWT_store_config_to_string)(AW_window *aww, AW_CL cl1, AW_CL cl2);
typedef void (*AWT_load_config_from_string)(AW_window *aww, const char *stored_string, AW_CL cl1, AW_CL cl2);

void AWT_insert_config_manager(AW_window *aww, AW_default default_file_, const char *id, AWT_store_config_to_string store, AWT_load_config_from_string load, AW_CL cl1, AW_CL cl2);

// helper functions to store/restore awars to/from string :
void  AWT_reset_configDefinition(AW_root *aw_root);
void  AWT_add_configDefinition(const char *awar_name, const char *config_name, int counter = -1);
char *AWT_store_configDefinition();
void  AWT_restore_configDefinition(const char *s);

#else
#error awt_config_manager.hxx included twice
#endif // AWT_CONFIG_MANAGER_HXX


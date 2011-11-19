//  ==================================================================== //
//                                                                       //
//    File      : mp_proto.hxx                                           //
//    Purpose   : Provide some prototypes                                //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in May 2003              //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef MP_PROTO_HXX
#define MP_PROTO_HXX

void MP_Comment(AW_window *aww, AW_CL com);

void MP_gen_quality(AW_root *awr,AW_CL cd1,AW_CL cd2);

void MP_new_sequence(AW_window *aww);

BOOL MP_is_probe(char *seq);

void MP_cache_sonden(AW_window *aww);
void MP_cache_sonden2(AW_root *aww);

void MP_mark_probes_in_tree(AW_window *aww);

void MP_gen_singleprobe(AW_root *awr,AW_CL cd1,AW_CL cd2);
void MP_modify_selected(AW_root *awr,AW_CL cd1,AW_CL cd2);

extern char *glob_old_seq;

#else
#error mp_proto.hxx included twice
#endif // MP_PROTO_HXX

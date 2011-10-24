// =============================================================== //
//                                                                 //
//   File      : ED4_objspec.cxx                                   //
//   Purpose   : hierarchy object specification                    //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2011   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ed4_class.hxx"

ED4_objspec::ED4_objspec(ED4_properties static_prop_, ED4_level level_, ED4_level allowed_children_, ED4_level handled_level_, ED4_level restriction_level_, float justification_)
    : static_prop(static_prop_),
      level(level_),
      allowed_children(allowed_children_),
      handled_level(handled_level_),
      restriction_level(restriction_level_),
      justification(justification_)
{
    
}



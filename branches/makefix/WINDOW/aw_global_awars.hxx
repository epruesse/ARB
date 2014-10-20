//  ==================================================================== //
//                                                                       //
//    File      : aw_global_awars.hxx                                    //
//    Purpose   : define awars available in ALL arb applications         //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in January 2003          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef AW_GLOBAL_AWARS_HXX
#define AW_GLOBAL_AWARS_HXX

#define AWAR_WWW_BROWSER            "www/browse_cmd" // how to call the users browser
#define AWAR_AW_FOCUS_FOLLOWS_MOUSE "focus/follow"   // shall focus follow mouse
#define AWAR_ARB_TREE_RENAMED       "tmp/tree_rename" // contains "oldname=newname" whenever a tree gets renamed
                                                      // (newname is NO_TREE_SELECTED if a tree has been deleted)

#else
#error aw_global_awars.hxx included twice
#endif // AW_GLOBAL_AWARS_HXX


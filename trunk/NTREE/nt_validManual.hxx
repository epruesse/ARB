#ifndef NT_VALID_MANUAL_HXX
#define NT_VALID_MANUAL_HXX

struct selectValidNameStruct{
    GBDATA            *gb_main;
    AW_window         *aws;
    AW_root           *awr;
    AW_selection_list *validNamesList;
    const char        *initials;
    //    AW_selection_list *con_id;
    //    char              *seqType;
};



#define AWAR_SELECTED_VALNAME       "tmp/validNames/selectedName"
// #define AWAR_INPUT_INITIALS         "tmp/validNames/inputValidNameInitials"
#define AWAR_INPUT_INITIALS         "tmp/validNames/inputInitials"
AW_window *NT_searchManuallyNames(AW_root *aw_root   /* , AW_CL cl_ntw */);
void       NT_createValidNamesAwars(AW_root *aw_root, AW_default aw_def);

#endif

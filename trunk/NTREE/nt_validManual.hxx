#ifndef NT_VALID_MANUAL_HXX
#define NT_VALID_MANUAL_HXX

struct selectValidNameStruct{
    GBDATA            *gb_main;
    AW_window         *aws;
    AW_root           *awr;
    AW_selection_list *validNamesList;
    //    AW_selection_list *con_id;
    //    char              *seqType;
};



#define AWAR_SELECTED_VALNAME       "tmp/validNames/selectedName"
AW_window *NT_searchManuallyNames(AW_root *aw_root   /* , AW_CL cl_ntw */);
void       NT_createValidNamesAwars(AW_root *aw_root, AW_default aw_def);

#endif

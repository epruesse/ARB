#ifndef NT_VALID_NAMES_HXX
#define NT_VALID_NAMES_HXX


typedef vector<string> NAMELIST;
void NT_importValidNames(AW_window*, AW_CL, AW_CL);
void NT_insertValidNames(AW_window*, AW_CL, AW_CL);
void NT_deleteValidNames(AW_window*, AW_CL, AW_CL);
void NT_suggestValidNames(AW_window*, AW_CL, AW_CL);
AW_window *NT_searchManuallyNames(AW_root *aw_root/*, AW_CL cl_ntw*/);




#endif

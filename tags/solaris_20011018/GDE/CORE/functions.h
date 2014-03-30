#ifndef P_
#if defined(__STDC__) || defined(__cplusplus)
# define P_(s) s
#else
# define P_(s) ()
#endif
#endif


/* BasicDisplay.c */
Panel BasicDisplay P_((NA_Alignment *DataSet));
void bailout P_((void));
int GenMenu P_((int type));
int MakeNAADisplay P_((void));
NA_DisplayData *SetNADData P_((NA_Alignment *aln, Canvas Can, Canvas NamCan));
int DummyRepaint ();
int DrawNANames P_((Display *dpy, Window xwin));
int RepaintNACan ();
int SetNACursor P_((NA_DisplayData *NAdd, Canvas can, Xv_window win, Window xwin, Display *dpy, GC gc));
int UnsetNACursor P_((NA_DisplayData *NAdd, Canvas can, Xv_window win, Window xwin, Display *dpy, GC gc));
int ResizeNACan P_((Canvas canvas, int wd, int ht));
int QuitGDE P_((void));

/* BuiltIn.c */
int Open P_((Menu mnu, Menu_item mnuitm));
int SaveAs P_((Menu mnu, Menu_item mnuitm));
int SaveFormat P_((Panel_item item, Event *event));
int SaveAsFileName P_((Panel_item item, Event *event));
int act_on_it_lf P_((char filename[], Xv_opaque data));
int act_on_it_sf P_((void));
int OpenFileName P_((Panel_item item, Event *event));
int ChangeDisplay P_((Panel_item item, Event *event));
int SetScale P_((Panel_item item, Event *event));
int ChColor P_((Panel_item item, Event *event));
int ChFontSize P_((Panel_item item, Event *event));
int ChDisplayDone P_((void));
int SetProtection P_((Panel_item item, Event *event));
int Prot P_((Panel_item item, Event *event));
int SelectAll P_((Panel_item item, Event *event));
int SelectBy P_((Panel_item item, Event *event));
int SelectByName P_((Panel_item item, Event *event));
int Group P_((Panel_item item, Event *event));
int RemoveFromGroup P_((NA_Sequence *element));
int AdjustGroups P_((NA_Alignment *aln));
int Ungroup P_((Panel_item item, Event *event));
int MergeGroups P_((NA_Sequence *el1, NA_Sequence *el2));
int New P_((void));
int ModAttr P_((Menu mnu, Menu_item mnuitm));
Notify_value SaveComments P_((Notify_client client, Destroy_status status));
int ChAttr P_((Panel_item item, Event *event));
int ModAttrDone P_((void));
int ChEditMode P_((Panel_item item, Event *event));
int ChEditDir P_((Panel_item item, Event *event));
int ChDisAttr P_((Panel_item item, Event *event));
int ChAttrType P_((Panel_item item, Event *event));
int SwapElement P_((NA_Alignment *aln, int e1, int e2));
int CompressAlign P_((Panel_item item, Event *event));
int RevSeqs P_((Panel_item item, Event *event));
int CompSeqs P_((Panel_item item, Event *event));
int SetFilename P_((Panel_item item, Event *event));
int CaseChange P_((Panel_item item, Event *event));
int OrigDir P_((NA_Sequence *seq));

/* ChooseFile.c */
Frame load_file P_((Frame Parentframe, int x, int y, Xv_opaque passdata));
int fl_open_btn_lf P_((Panel_item item, Event *event));
int fl_up_dir_btn P_((Panel_item item, Event *event));
Panel_setting fl_dir_typed P_((Panel_item item, Event *event));
int lf_cancel_btn P_((Panel_item item, Event *event));
int fl_readln P_((FILE *file, char *buf));
int fl_make_list P_((void));
int fl_set_dirtext P_((Panel_item fl_DirText));
int fl_checkdir P_((char *dirname));
void fl_show_list_lf P_((Canvas canvas, Xv_Window paint_window, Rectlist *repaint_area));
void fl_list_select_lf P_((Xv_window paint_window, Event *event));
int fl_view_h P_((void));
Notify_value fl_free_mem P_((Notify_client client, Destroy_status status));

/* CutCopyPaste.c */
int EditCut P_((Panel_item item, Event *event));
int EditCopy P_((Panel_item item, Event *event));
int EditPaste P_((Panel_item item, Event *event));
int Regroup P_((NA_Alignment *alignment));
int EditSubCut P_((Panel_item item, Event *event));
int SubCutViolate P_((void));
int EditSubPaste P_((Panel_item item, Event *event));
int EditSubCopy P_((Panel_item item, Event *event));
int TestSelection P_((void));

/* DrawNA.c */
int DrawNAColor P_((Canvas can, NA_DisplayData *NAdd, Window xwin, int left, int top, int indx, int lpos, int rpos, Display *dpy, GC gc, int mode, int inverted));

/* Edit.c */
int NAEvents P_((Xv_window win, Event *event, Notify_arg arg));
int InsertViolate P_((NA_Alignment *aln, NA_Sequence *seq, NA_Base *insert, int cursor_x, int len));
int InsertNA P_((NA_Sequence *seq, NA_Base *insert, int len, int pos));
int FetchNA P_((NA_Sequence *seq, unsigned int dir, int len, int pos));
int DeleteNA P_((NA_Alignment *aln, int seqnum, int len, int offset));
int DeleteViolate P_((NA_Alignment *aln, NA_Sequence *this_seq, int len, int offset));
int RedrawAllNAViews P_((int seqnum, int start));
int ResetPos P_((NA_DisplayData *ddata));
int Beep P_((void));
int Keyclick P_((void));
int putelem P_((NA_Sequence *a, int b, NA_Base c));
int putcmask P_((NA_Sequence *a, int b, int c));
int getelem P_((NA_Sequence *a, int b));
int isagap P_((NA_Sequence *a, int b));
int SubSelect P_((NA_Alignment *aln, int shift_down, int x1, int y1, int x2, int y2));

/* EventHandler.c */
void HandleMenus P_((Menu m, Menu_item mi));
int HandleMenuItem P_((Panel_item item, Event *event));
int DO P_((void));
char *ReplaceFile P_((char *Action, GfileFormat file));
char *ReplaceArgs P_((char *Action, GmenuItemArg arg));
int DONT P_((Panel_item item, Event *event));
int FrameDone P_((Frame this_frame));
int NANameEvents P_((Xv_window win, Event *event, Notify_arg arg));
int DoMeta P_((int Code));
int HandleMeta P_((int curmenu, int curitem));
int HELP P_((Panel_item item, Event *event));

/* FileIO.c */
int LoadData P_((char *filename));
int LoadFile P_((char *filename, NA_Alignment *dataset, int type, int format));
int ErrorOut P_((int code, char *string));
char *Calloc P_((int count, int size));
char *Realloc P_((char *block, int size));
int Cfree P_((char *block));
char *String P_((char *string));
int FindType P_((char *name, int *dtype, int *ftype));
int AppendNA P_((NA_Base *buffer, int len, NA_Sequence *seq));
int Ascii2NA P_((char *buffer, int len, int matrix[16 ]));
int WriteNA_Flat P_((NA_Alignment *aln, char *filename, int method, int maskable));
int Warning P_((char *s));
int InitNASeq P_((NA_Sequence *seq, int type));
int ReadCMask P_((char *filename));
int ReadNA_Flat P_((char *filename, char *dataset, int type));
int WriteStatus P_((NA_Alignment *aln, char *filename, int method));
int ReadStatus P_((char *filename));
int NormalizeOffset P_((NA_Alignment *aln));
int WriteCMask P_((NA_Alignment *aln, char *filename, int method, int maskable));

/* Free.c */
int FreeNASeq P_((NA_Sequence *seq));
int FreeNAAln P_((NA_Alignment *aln));
int FreeNADD P_((NA_DisplayData *nadd));

/* Genbank.c */
int ReadGen P_((char *filename, NA_Alignment *dataset, int type));

/* HGLfile.c */
int ReadGDE P_((char *filename, NA_Alignment *dataset, int type));
int WriteGDE P_((NA_Alignment *aln, char *filename, int method, int maskable));
int StripSpecial P_((char *string));
int RemoveQuotes P_((char *string));
void SeqNorm P_((NA_Sequence *seq));
char *uniqueID P_((void));
int OverWrite P_((NA_Sequence *this, NA_Alignment *aln));

/* ParseMenu.c */
int ParseMenu P_((void));
int Find P_((char *target, char *key));
int Find2 P_((char *target, char *key));
int Error P_((char *msg));
int getline P_((FILE *file, char string[]));
int crop P_((char input[], char head[], char tail[]));

/* Scroll.c */
int InitEditSplit P_((Xv_Window oldview, Xv_Window newview, int pos));
Notify_value EditCanScroll P_((Notify_client client, Event *event, Notify_arg arg, Notify_event_type type));
void JumpTo P_((Xv_window view, int x, int y));
int RepaintAll P_((int Names));
int DestroySplit P_((Xv_window view));

/* arbdb_io.c */
int Arbdb_get_curelem P_((NA_Alignment *dataset));
int ReadArbdb P_((char *filename, NA_Alignment *dataset, int type));
int WriteArbdb P_((NA_Alignment *aln, char *filename, int method, int maskable));
int Updata_Arbdb P_((Panel_item item, Event *event));

/* main.c */
int main P_((int argc, char **argv));

#undef P_
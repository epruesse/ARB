	FILE *infile,*tempfile;
	double xmax,ymax,xmin,ymin;
	double xscale,yscale,xoffset,yoffset;
	int seqlen,modified,seqnum,nuc_sel1,nuc_sel2,select_state;
	int constraint_state,redo,ddepth,ddepth1,Mxdepth,sho_con;
	int WINDOW_SIZE;
	char *seqname;
	DataSet dataset;
	Base *baselist;

	Menu File,Edit,Style;
        struct pixfont *screenfont,*fonts[20];
        Frame page,file_dialog,print_dialog;
	Canvas pagecan;
	Pixwin *pagewin;

#ifndef awtc_export_hxx_included
#define awtc_export_hxx_included

AW_window *open_AWTC_export_window(AW_root *awr,GBDATA *gb_main);

#if 0
GB_ERROR AWTC_aligner(GBDATA *gb_main,		// main data
		      GB_CSTR reference,	// name of reference species (==NULL -> use next relative for (each) sequence)
		      int pt_server_id,		// pt_server to search for next relative
		      GB_CSTR alignment,	// name of alignment to use (==NULL -> use default)
		      GB_CSTR toalign,		// name of species to align (==NULL -> align all marked species)
		      int mirrorAllowed,	// !=0 -> AWTC_aligner is authorized to mirror 'toalign'
		      int temporary,		// ==1 -> create only temporary aligment report into alignment (else resident)
		      int firstColumn,		// first column of range to be aligned (0..len-1)
		      int lastColumn);		// last column of range to be aligned (0..len-1, -1=(len-1))
#endif


#endif

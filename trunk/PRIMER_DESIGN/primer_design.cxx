#include <cmath>
#include <string>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>

#include "primer_design.hxx"
#include "PRD_Design.hxx"
#include "PRD_SequenceIterator.hxx"

extern GBDATA *gb_main;

using std::string;

//  ---------------------------------------------
//      static double get_estimated_memory()
//  ---------------------------------------------
static double get_estimated_memory(AW_root *root) {
    int    bases  = root->awar( AWAR_PRIMER_DESIGN_LEFT_LENGTH )->read_int() + root->awar( AWAR_PRIMER_DESIGN_RIGHT_LENGTH )->read_int();
    int    length = root->awar( AWAR_PRIMER_DESIGN_LENGTH_MAX )->read_int();
    double mem    = bases*length*0.9*(sizeof(Node)+16);
    return mem;
}

/////////////////////////////////////////////////////////////////////////////////////////////// primer_design_event_update_memory
//
//

void primer_design_event_update_memory( AW_window *aww ) {
    AW_root *root = aww->get_root();
    double   mem  = get_estimated_memory(root);

    if (mem > 1073741824) {
      mem = mem / 1073741824;
      root->awar( AWAR_PRIMER_DESIGN_APROX_MEM )->write_string(GBS_global_string("%.1f TB",mem));
    } else if (mem > 1048576) {
      mem = mem / 1048576;
      root->awar( AWAR_PRIMER_DESIGN_APROX_MEM )->write_string(GBS_global_string("%.1f MB",mem));
    } else if (mem > 1024) {
      mem = mem / 1024;
      root->awar( AWAR_PRIMER_DESIGN_APROX_MEM )->write_string(GBS_global_string("%.1f KB",mem));
    } else {
      root->awar( AWAR_PRIMER_DESIGN_APROX_MEM )->write_string(GBS_global_string("%.0f bytes",mem));
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////// create_primer_design_variables
//
//
void create_primer_design_variables( AW_root *aw_root, AW_default aw_def, AW_default global )
{
  aw_root->awar_int( AWAR_PRIMER_DESIGN_LEFT_POS,		    0, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_LEFT_LENGTH,		  100, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_RIGHT_POS,		 1000, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_RIGHT_LENGTH,		  100, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_LENGTH_MIN,		   10, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_LENGTH_MAX,		   20, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_DIST_MIN,		 1050, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_DIST_MAX,		 1200, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_GCRATIO_MIN,		   10, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_GCRATIO_MAX,		   50, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_TEMPERATURE_MIN,	   30, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_TEMPERATURE_MAX,	   80, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_ALLOWED_MATCH_MIN_DIST,     0, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_EXPAND_IUPAC,		    1, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_MAX_PAIRS,		   25, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_GC_FACTOR,		   50, aw_def);
  aw_root->awar_int( AWAR_PRIMER_DESIGN_TEMP_FACTOR,		   50, aw_def);

  aw_root->awar_string( AWAR_PRIMER_DESIGN_APROX_MEM,		   "", aw_def);

  aw_root->awar_string( AWAR_PRIMER_TARGET_STRING,                 "", global);
}

/////////////////////////////////////////////////////////////////////////////////////////////// create_primer_design_result_window
//
//

static AW_window_simple  *pdrw    = 0;
static AW_selection_list *pdrw_id = 0;

void create_primer_design_result_window( AW_window *aww )
{
  if ( !pdrw ) {
    pdrw = new AW_window_simple;
    pdrw->init( aww->get_root(), "PRD_RESULT", "Primer Design RESULT", 0, 400 );
    pdrw->load_xfig( "pd_reslt.fig" );

    pdrw->at( "close" );
    pdrw->callback( (AW_CB0)AW_POPDOWN );
    pdrw->create_button( "CLOSE", "CLOSE", "C" );

    pdrw->at( "help" );
    pdrw->callback( AW_POPUP_HELP, (AW_CL)"primerdesignresult.hlp" );
    pdrw->create_button( "HELP", "HELP", "H" );

    pdrw->at( "result" );
    pdrw_id = pdrw->create_selection_list( AWAR_PRIMER_TARGET_STRING, NULL, "", 40, 5 );
    pdrw->set_selection_list_suffix( pdrw_id, "primer" );

    pdrw->at( "save" );
    pdrw->callback( AW_POPUP, (AW_CL)create_save_box_for_selection_lists, (AW_CL)pdrw_id );
    pdrw->create_button( "SAVE", "SAVE", "S" );

    pdrw->at( "print" );
    pdrw->callback( create_print_box_for_selection_lists, (AW_CL)pdrw_id );
    pdrw->create_button( "PRINT", "PRINT", "P" );
  }

  pdrw->show();
}


/////////////////////////////////////////////////////////////////////////////////////////////// create_primer_design_window
//
//
AW_window *create_primer_design_window( AW_root *root,AW_default def )
{
  GB_ERROR error = 0;
  {
    GB_transaction  dummy( gb_main );
    char           *selected_species = root->awar( AWAR_SPECIES_NAME )->read_string();
    GBDATA         *gb_species       = GBT_find_species( gb_main,selected_species );

    if ( !gb_species ) {
      error = "You have to select a species!";
      aw_message( error );
    }
  }

  AWUSE( def );
  AW_window_simple *aws = new AW_window_simple;
  aws->init( root, "PRIMER_DESIGN","PRIMER DESIGN", 10, 10 );

  aws->load_xfig("prd_main.fig" );

  aws->callback( (AW_CB0)AW_POPDOWN );                     aws->at( "close" );   aws->create_button( "CLOSE","CLOSE","C" );
  aws->callback( AW_POPUP_HELP,(AW_CL)"primer_new.hlp" );  aws->at( "help" );    aws->create_button( "HELP","HELP","H" );
  aws->callback( primer_design_event_init );               aws->at( "init" );    aws->create_button( "INIT","INIT","I" );
  aws->callback( primer_design_event_go );                 aws->at( "design" );  aws->create_button( "GO","GO","G" );
  aws->highlight();


  aws->at( "minleft" );   aws->create_input_field( AWAR_PRIMER_DESIGN_LEFT_POS,    7 );
  aws->callback( primer_design_event_update_memory );
  aws->at( "maxleft" );	  aws->create_input_field( AWAR_PRIMER_DESIGN_LEFT_LENGTH, 9 );
  aws->at( "minright" );  aws->create_input_field( AWAR_PRIMER_DESIGN_RIGHT_POS,    7 );
  aws->callback( primer_design_event_update_memory );
  aws->at( "maxright" );  aws->create_input_field( AWAR_PRIMER_DESIGN_RIGHT_LENGTH, 9 );
  aws->at( "minlen" );    aws->create_input_field( AWAR_PRIMER_DESIGN_LENGTH_MIN, 7 );
  aws->callback( primer_design_event_update_memory );
  aws->at( "maxlen" );    aws->create_input_field( AWAR_PRIMER_DESIGN_LENGTH_MAX, 7 );
  aws->at( "mindist" );   aws->create_input_field( AWAR_PRIMER_DESIGN_DIST_MIN, 7 );
  aws->at( "maxdist" );   aws->create_input_field( AWAR_PRIMER_DESIGN_DIST_MAX, 7 );
  aws->at( "mingc" );     aws->create_input_field( AWAR_PRIMER_DESIGN_GCRATIO_MIN, 7 );
  aws->at( "maxgc" );     aws->create_input_field( AWAR_PRIMER_DESIGN_GCRATIO_MAX, 7 );
  aws->at( "mintemp" );   aws->create_input_field( AWAR_PRIMER_DESIGN_TEMPERATURE_MIN, 7 );
  aws->at( "maxtemp" );   aws->create_input_field( AWAR_PRIMER_DESIGN_TEMPERATURE_MAX, 7 );

  aws->at( "allowed_match" );  aws->create_input_field( AWAR_PRIMER_DESIGN_ALLOWED_MATCH_MIN_DIST, 7);
  aws->at( "expand_IUPAC" );   aws->create_toggle( AWAR_PRIMER_DESIGN_EXPAND_IUPAC );
  aws->at( "max_pairs" );      aws->create_input_field( AWAR_PRIMER_DESIGN_MAX_PAIRS, 7 );
  aws->callback( primer_design_event_check_gc_factor );
  aws->at( "GC_factor" );      aws->create_input_field( AWAR_PRIMER_DESIGN_GC_FACTOR, 7 );
  aws->callback( primer_design_event_check_temp_factor );
  aws->at( "temp_factor" );    aws->create_input_field( AWAR_PRIMER_DESIGN_TEMP_FACTOR, 7 );

  aws->at( "aprox_mem" );      aws->create_input_field( AWAR_PRIMER_DESIGN_APROX_MEM, 11 );

  return aws;
}

inline int prd_aw_status(const char *s) { return aw_status(s); }
inline int prd_aw_status(double d) { return aw_status(d); }


/////////////////////////////////////////////////////////////////////////////////////////////// primer_design_event_go
//
//
void primer_design_event_go(AW_window *aww) {
    AW_root  *root     = aww->get_root();
    GB_ERROR  error    = 0;
    char     *sequence = 0;
    long int  length   = 0;

    if ((get_estimated_memory(root)/1024.0) > GB_get_physical_memory()) {
        if (aw_message("ARB may crash due to memory problems.", "Continue, Abort") == 1) {
            return;
        }
    }

    {
        GB_transaction  dummy(gb_main);
        char           *selected_species = root->awar(AWAR_SPECIES_NAME)->read_string();
        GBDATA         *gb_species       = GBT_find_species(gb_main,selected_species);

        if ( !gb_species ) {
            error = "you have to select a species!";
        }
        else {
            const char *alignment = GBT_get_default_alignment(gb_main);
            GBDATA     *gb_seq    = GBT_read_sequence(gb_species, alignment);

            if (!gb_seq) {
                error = GBS_global_string("Selected species has no sequence data in alignment '%s'", alignment);
            }
            else {
                sequence = GB_read_string(gb_seq);
                length   = GB_read_count(gb_seq);
            }
        }
    }

    if ( !error ) {
        aw_openstatus("Search PCR primer pairs");
        PrimerDesign *PD =
            new PrimerDesign(sequence, length,
                             Range(root->awar(AWAR_PRIMER_DESIGN_LEFT_POS)->read_int(),  root->awar(AWAR_PRIMER_DESIGN_LEFT_LENGTH)->read_int()),
                             Range(root->awar(AWAR_PRIMER_DESIGN_RIGHT_POS)->read_int(), root->awar(AWAR_PRIMER_DESIGN_RIGHT_LENGTH)->read_int()),
                             Range(root->awar(AWAR_PRIMER_DESIGN_LENGTH_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_LENGTH_MAX)->read_int()),
                             Range(root->awar(AWAR_PRIMER_DESIGN_DIST_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_DIST_MAX)->read_int()),
                             Range(root->awar(AWAR_PRIMER_DESIGN_GCRATIO_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_GCRATIO_MAX)->read_int()),
                             Range(root->awar(AWAR_PRIMER_DESIGN_TEMPERATURE_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_TEMPERATURE_MAX)->read_int()),
                             root->awar(AWAR_PRIMER_DESIGN_ALLOWED_MATCH_MIN_DIST)->read_int(),
                             (bool)root->awar(AWAR_PRIMER_DESIGN_EXPAND_IUPAC)->read_int(),
                             root->awar(AWAR_PRIMER_DESIGN_MAX_PAIRS)->read_int(),
                             (float)root->awar(AWAR_PRIMER_DESIGN_GC_FACTOR)->read_int()/100,
                             (float)root->awar(AWAR_PRIMER_DESIGN_TEMP_FACTOR)->read_int()/100
                             );

        PD->set_status_callbacks(prd_aw_status, prd_aw_status);

        try {
#ifdef DEBUG
            PD->run(PrimerDesign::PRINT_PRIMER_PAIRS);
#else
            PD->run(0);
#endif
        }
        catch (string& s) {
            error = GBS_global_string(s.c_str());
        }
        catch (...) {
            error = "Unknown error (maybe out of memory ? )";
        }
        if ( !error ) error = PD->get_error();

        if ( !error ) {
            if ( !pdrw_id ) create_primer_design_result_window(aww);
            gb_assert( pdrw_id );

            // create result-list:
            pdrw->clear_selection_list( pdrw_id );
            int max_primer_length   = PD->get_max_primer_length();
            int max_position_length = int(std::log10(double (PD->get_max_primer_pos())))+1;
            int max_length_length   = int(std::log10(double(PD->get_max_primer_length())))+1;

            if ( max_position_length < 3 ) max_position_length = 3;
            if ( max_length_length   < 3 ) max_length_length   = 3;

            pdrw->insert_selection(pdrw_id,
                                   GBS_global_string(" Rating | %-*s %-*s %-*s G/C Tmp | %-*s %-*s %-*s G/C Tmp",
                                                     max_primer_length,   "Left primer",
                                                     max_position_length, "Pos",
                                                     max_length_length,   "Len",
                                                     max_primer_length,   "Right primer",
                                                     max_position_length, "Pos",
                                                     max_length_length,   "Len"),
                                   "" );

            int r;

            for ( r = 0; ; ++r) {
                const char *primers = 0;
                const char *result  = PD->get_result( r, primers, max_primer_length, max_position_length, max_length_length );
                if ( !result ) break;
                pdrw->insert_selection( pdrw_id, result, primers );
            }

            pdrw->insert_default_selection( pdrw_id, (r ? "**** End of list" : "**** There are no results"), "" );

            pdrw->update_selection_list( pdrw_id );
            pdrw->show();
        }
    }
    if ( sequence ) free( sequence );
    if ( error ) aw_message( error );

    aw_closestatus();
}


/////////////////////////////////////////////////////////////////////////////////////////////// primer_design_event_check_temp_factor
//
//
void primer_design_event_check_temp_factor( AW_window *aww ) {
    AW_root *root = aww->get_root();

    int temp = root->awar( AWAR_PRIMER_DESIGN_TEMP_FACTOR )->read_int();
    if ( temp > 100 ) temp = 100;
    if ( temp <   0 ) temp =   0;
    root->awar( AWAR_PRIMER_DESIGN_TEMP_FACTOR )->write_int( temp );
    root->awar( AWAR_PRIMER_DESIGN_GC_FACTOR )->write_int( 100-temp );
}

/////////////////////////////////////////////////////////////////////////////////////////////// primer_design_event_check_gc_factor
//
//
void primer_design_event_check_gc_factor( AW_window *aww ) {
  AW_root *root = aww->get_root();

  int gc = root->awar( AWAR_PRIMER_DESIGN_GC_FACTOR )->read_int();
  if ( gc > 100 ) gc = 100;
  if ( gc <   0 ) gc =   0;
  root->awar( AWAR_PRIMER_DESIGN_GC_FACTOR )->write_int( gc );
  root->awar( AWAR_PRIMER_DESIGN_TEMP_FACTOR )->write_int( 100-gc );
}

/////////////////////////////////////////////////////////////////////////////////////////////// primer_design_event_init
//
//
void primer_design_event_init( AW_window *aww ) {
  AW_root         *root     = aww->get_root();
  GB_ERROR         error    = 0;
  char            *sequence = 0;


  GB_transaction  dummy(gb_main);
  char           *selected_species = root->awar( AWAR_SPECIES_NAME)->read_string();
  GBDATA         *gb_species       = GBT_find_species( gb_main, selected_species );
  GBDATA         *gb_seq           = 0;

  if ( !gb_species ) {
      error = "you have to select a species!";
  }
  else {
      const char *alignment = GBT_get_default_alignment( gb_main );
      gb_seq                = GBT_read_sequence( gb_species, alignment );
      if (!gb_seq) {
          error = GB_export_error("species has no data in alignment '%s'", alignment);
      }
  }

  if (gb_seq) {
      SequenceIterator *i;
      PRD_Sequence_Pos  length;
      PRD_Sequence_Pos  left_min, left_max;
      PRD_Sequence_Pos  right_min, right_max;
      long int          dist_min, dist_max;
      long int          length_min, length_max;

      sequence = GB_read_string(gb_seq);
      length   = strlen(sequence);

      // find reasonable parameters
      // left pos (first base from start)

      long int left_len = root->awar(AWAR_PRIMER_DESIGN_LEFT_LENGTH)->read_int();
      if (left_len == 0 || left_len<0) left_len = 100;

      i                     = new SequenceIterator( sequence, 0, SequenceIterator::IGNORE, left_len, SequenceIterator::FORWARD );
      i->nextBase();                                                  // find first base from start
      left_min              = i->pos;                                 // store pos. of first base
      while (i->nextBase() != SequenceIterator::EOS ) ;               // step to 'left_len'th base from start
      left_max              = i->pos;                                 // store pos. of 'left_len'th base
      root->awar(AWAR_PRIMER_DESIGN_LEFT_POS)->write_int(left_min);
      root->awar(AWAR_PRIMER_DESIGN_LEFT_LENGTH)->write_int(left_len);

      long int right_len = root->awar(AWAR_PRIMER_DESIGN_RIGHT_LENGTH)->read_int();
      if (right_len == 0 || right_len<0) right_len = 100;

      // right pos ('right_len'th base from end)
      i->restart( length, 0, right_len, SequenceIterator::BACKWARD );
      i->nextBase();				                      // find last base
      right_max             = i->pos;                                 // store pos. of last base
      while (i->nextBase() != SequenceIterator::EOS ) ;               // step to 'right_len'th base from end
      right_min             = i->pos;                                 // store pos of 'right_len'th base from end
      root->awar(AWAR_PRIMER_DESIGN_RIGHT_POS)->write_int(right_min);
      root->awar(AWAR_PRIMER_DESIGN_RIGHT_LENGTH)->write_int(right_len);

      // primer distance
      if ( right_min >= left_max ) {                                  // non-overlapping ranges
          i->restart(left_max, right_min, SequenceIterator::IGNORE, SequenceIterator::FORWARD);
          long int bases_between  = 0;                                // count bases from right_min to left_max
          while (i->nextBase()   != SequenceIterator::EOS) ++bases_between;
          dist_min = bases_between;                                   // take bases between as min distance
      }
      else {							      // overlapping ranges
          dist_min = right_min - left_min +1;
      }
      dist_max = right_max - left_min;
      root->awar(AWAR_PRIMER_DESIGN_DIST_MIN)->write_int(dist_min);
      root->awar(AWAR_PRIMER_DESIGN_DIST_MAX)->write_int(dist_max);

      // primer length
      length_min = root->awar(AWAR_PRIMER_DESIGN_LENGTH_MIN)->read_int();
      length_max = root->awar(AWAR_PRIMER_DESIGN_LENGTH_MAX)->read_int();
      if ( length_max > 100 ) length_max = 100;
      if ( length_min >= length_max ) length_min = length_max >> 2;
      root->awar(AWAR_PRIMER_DESIGN_LENGTH_MIN)->write_int(length_min);
      root->awar(AWAR_PRIMER_DESIGN_LENGTH_MAX)->write_int(length_max);

      // GC-ratio/temperature - factors
      root->awar(AWAR_PRIMER_DESIGN_GC_FACTOR)->write_int(50);
      root->awar(AWAR_PRIMER_DESIGN_TEMP_FACTOR)->write_int(50);

      // update mem-info
      primer_design_event_update_memory(aww);

      delete i;
      free( sequence );

#ifdef DEBUG
      printf ( "primer_design_event_init : left_min   %7li\n",left_min );
      printf ( "primer_design_event_init : left_max   %7li\n",left_max );
      printf ( "primer_design_event_init : right_min  %7li\n",right_min );
      printf ( "primer_design_event_init : right_max  %7li\n",right_max );
      printf ( "primer_design_event_init : length_min %7li\n",length_min );
      printf ( "primer_design_event_init : length_max %7li\n",length_max );
      printf ( "primer_design_event_init : dist_min   %7li\n",dist_min );
      printf ( "primer_design_event_init : dist_max   %7li\n\n",dist_max );
#endif
  }

  if (error) {
      aw_message(error);
  }
}

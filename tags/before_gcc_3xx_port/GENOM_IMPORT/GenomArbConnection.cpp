#include <GenomArbConnection.h>

GenomArbConnection::GenomArbConnection(void)
{

}

GenomArbConnection::~GenomArbConnection(void)
{

}

GenomArbConnection::setEmblFlatFile(GenomEmbl & genomembl,GBDATA *gb_main)
{
  genomembl.prepareFlatFile();
  typeOfDatabase = 1;
  GBDATA        *gb_species_data  = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
  char          *new_species_name = AWTC_makeUniqueShortName("genom", gb_species_data);
  GBDATA        *gb_species       = GBT_create_species(gb_main, new_species_name);

  char *content;

  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "accesion_number", GB_STRING), genomembl.getAC()->c_str());
  GB_write_string( GB_search(gb_species, "sequence_version", GB_STRING), genomembl.getSV()->c_str());
  GB_write_string( GB_search(gb_species, "date_of_creation", GB_STRING), genomembl.getDTCreation()->c_str());
  GB_write_string( GB_search(gb_species, "date_of_last_update", GB_STRING), genomembl.getDTLastUpdate()->c_str());
  GB_write_string( GB_search(gb_species, "description", GB_STRING), genomembl.getDE()->c_str());
  GB_write_string( GB_search(gb_species, "key_words", GB_STRING), genomembl.getKW()->c_str());
  GB_write_string( GB_search(gb_species, "organism_species", GB_STRING), genomembl.getOS()->c_str());
  GB_write_string( GB_search(gb_species, "organism_classification", GB_STRING), genomembl.getOC()->c_str());
  GB_write_string( GB_search(gb_species, "or_garnelle", GB_STRING), genomembl.getOG()->c_str());

  GenomReferenceEmbl * tref;
  int trn = 0;
  while((tref = genomembl.getReference()) !== NULL)
  {
    trn = tref.getRN();
    GB_write_string( GB_search(gb_species, "reference_"+trn+"_comment", GB_STRING), tref.getRC()->c_str());
    GB_write_long( GB_search(gb_species, "reference_"+trn+"_position_begin", GB_LONG), tref.getRPBegin());
    GB_write_long( GB_search(gb_species, "reference_"+trn+"_position_end", GB_LONG), tref.getRPEnd());
    GB_write_string( GB_search(gb_species, "reference_"+trn+"_cross_reference", GB_STRING), tref.getRX)->c_str());
    GB_write_string( GB_search(gb_species, "reference_"+trn+"_group", GB_STRING), tref.getRG)->c_str());
    GB_write_string( GB_search(gb_species, "reference_"+trn+"_authors", GB_STRING), tref.getRA)->c_str());
    GB_write_string( GB_search(gb_species, "reference_"+trn+"_title", GB_STRING), tref.getRT)->c_str());
    GB_write_string( GB_search(gb_species, "reference_"+trn+"_location", GB_STRING), tref.getRL)->c_str());
  }
  GB_write_string( GB_search(gb_species, "database_cross_reference", GB_STRING), genomembl.getDX()->c_str());
  GB_write_string( GB_search(gb_species, "constructed", GB_STRING), genomembl.getCO()->c_str());
  GB_write_string( GB_search(gb_species, "free_text_comments", GB_STRING), genomembl.getCC()->c_str());

  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
  GB_write_string( GB_search(gb_species, "identification", GB_STRING), genomembl.getID()->c_str());
}
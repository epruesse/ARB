#ifndef PS_PG_SPECMAP_HXX
#define PS_PG_SPECMAP_HXX

// -----------------------------------------
//      mapping shortname <-> SpeciesID

static Name2IDMap __NAME2ID_MAP;
static ID2NameMap __ID2NAME_MAP;
static bool       __MAPS_INITIALIZED = false;

static GB_ERROR PG_initSpeciesMaps(GBDATA *pb_main) {

  GB_transaction ta(pb_main);

  ps_assert(!__MAPS_INITIALIZED);

  // look for existing mapping in pb-db:
  GBDATA *pb_mapping = GB_entry(pb_main, "species_mapping");
  if (!pb_mapping) {  // error
    GB_export_error("No species mapping");
  }
  else {
    // retrieve mapping from string
    const char *mapping = GB_read_char_pntr(pb_mapping);
    if (!mapping) return GB_export_error("Can't read mapping");

    while (mapping[0]) {
      const char *comma     = strchr(mapping, ',');   if (!comma) break;
      const char *semicolon = strchr(comma, ';');     if (!semicolon) break;
      string      name(mapping, comma-mapping);
      comma+=1;
      string idnum(comma, semicolon-comma);
      SpeciesID   id        = atoi(idnum.c_str());

      __NAME2ID_MAP[name] = id;
      __ID2NAME_MAP[id]   = name;

      mapping = semicolon+1;
    }
  }

  __MAPS_INITIALIZED = true;
  return 0;
}

SpeciesID PG_SpeciesName2SpeciesID(const string& shortname) {
  ps_assert(__MAPS_INITIALIZED); // you didn't call PG_initSpeciesMaps
  return __NAME2ID_MAP[shortname];
}

inline const string& PG_SpeciesID2SpeciesName(SpeciesID num) {
  ps_assert(__MAPS_INITIALIZED); // you didn't call PG_initSpeciesMaps
  return __ID2NAME_MAP[num];
}

static int PG_NumberSpecies() {
    return __ID2NAME_MAP.size();
}

#else
#error ps_pg_specmap.hxx included twice
#endif // PS_PG_SPECMAP_HXX

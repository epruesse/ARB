/*!
 * \file arbdb_data_retriever.cxx
 *
 * \date 08.02.2011
 * \author Tilo Eissler
 *
 * Contributors: Chris Hodges, Jörg Böhnel
 */

#include <stdexcept>

#include "adGene.h"

#include "arbdb_data_retriever.h"

/*!
 * \brief Private default constructor
 */
ArbdbDataRetriever::ArbdbDataRetriever() :
        AbstractDataRetriever(), m_mutex(), m_arbdb_mutex(), m_retrieve_features(
                false), m_db_name(), m_alpha_specs(NULL), m_open(false), m_shell(
                NULL), m_initialized_shell(false), m_main_db(NULL), m_sai_data(
                NULL), m_alignment_name(NULL), m_species_data(NULL), m_current_species(
                NULL), m_current_count(0), m_ignored_count(0) {
}

/*!
 * \brief Private copy constructor
 */
ArbdbDataRetriever::ArbdbDataRetriever(const ArbdbDataRetriever& /*adr*/) {
}

/*!
 * \brief Private assignment operator
 */
ArbdbDataRetriever& ArbdbDataRetriever::operator=(
        const ArbdbDataRetriever& adr) {
    if (this != &adr) {
        // we don't use it, so we don't implement it!
    }
    return *this;
}

/*!
 * \brief Private default constructor
 *
 * Accesses ALL sequence entries in database
 *
 * \param db_name Name (including path) of database
 *
 * \exception std::invalid_argument Thrown if AbstractAlphabetSpecifics pointer is empty
 */
ArbdbDataRetriever::ArbdbDataRetriever(const std::string& db_name,
        AbstractAlphabetSpecifics* alpha_specs) :
        AbstractDataRetriever(alpha_specs), m_mutex(), m_arbdb_mutex(), m_retrieve_features(
                false), m_db_name(db_name), m_alpha_specs(alpha_specs), m_open(
                false), m_shell(NULL), m_initialized_shell(false), m_main_db(
                NULL), m_sai_data(NULL), m_alignment_name(NULL), m_species_data(
                NULL), m_current_species(NULL), m_current_count(0), m_ignored_count(
                0) {
    if (!m_alpha_specs) {
        throw std::invalid_argument(
                "ArbdbDataRetriever(...) AbstractAlphabetSpecifics pointer is empty!");
    }
}

/*!
 * \brief Destructor
 */
ArbdbDataRetriever::~ArbdbDataRetriever() {
    try {
        closeConnection();
    } catch (...) {
        // do nothing, destructor must not throw!
    }
}

/*!
 * \brief Returns true if connection is open
 *
 * \return bool
 */
bool ArbdbDataRetriever::connected() const {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    return m_open;
}

/*!
 * \brief Open connection to database
 *
 * \exception runtime_error Thrown if open fails or no entry available
 */
void ArbdbDataRetriever::openConnection() {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    if (!GB_shell::in_shell()) {
#ifdef DEBUG
        printf("ArbdbDataRetriever::openConnection() init GB_shell.\n");
#endif
        m_shell = new GB_shell();
        m_initialized_shell = true;
    }
    GB_set_verbose();
    /* open the database */
    if (!(m_main_db = GB_open(m_db_name.data(), "r"))) {
        std::runtime_error(
                "ArbdbDataRetriever::openConnection(): Failed to open database!");
    }
    m_open = true;
    GB_begin_transaction(m_main_db);
    /* open the species data */
    if (!(m_species_data = GB_find(m_main_db, "species_data", SEARCH_CHILD))) {
        std::runtime_error(
                "ArbdbDataRetriever::openConnection(): Database is empty!");
    }
    m_current_species = GBT_first_species_rel_species_data(m_species_data);
    /* add the extended data container */
    m_sai_data = GBT_get_SAI_data(m_main_db);
    m_alignment_name = GBT_get_default_alignment(m_main_db);

    if (m_include_features) {
        // do this only if build settings include feature flag is true:
        // Identify the database type:
        GBDATA *gb_genom_db = GB_entry(m_main_db, "genom_db");
        if ((gb_genom_db == NULL) || (GB_read_int(gb_genom_db) == 0)) {
            // gene db
            m_retrieve_features = false;
#ifdef DEBUG
            printf("Don't retrieve features.\n");
#endif
        } else {
            // genome db, so get features (called 'genes' in ARB)
            m_retrieve_features = true;
#ifdef DEBUG
            printf("Retrieve features.\n");
#endif
        }
#ifdef DEBUG
    } else {
        printf("Don't retrieve features at all.\n");
#endif
    }
}

/*!
 * \brief Close database connection
 */
void ArbdbDataRetriever::closeConnection() {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    if (m_open) {
        GB_commit_transaction(m_main_db);
        m_open = false;
        if (m_main_db) {
            GB_close(m_main_db);
#ifdef DEBUG
            printf("ArbdbDataRetriever::closeConnection() Closed DB.\n");
#endif
        }
        m_main_db = NULL;
        m_sai_data = NULL;
        if (m_alignment_name) {
            free(m_alignment_name);
            m_alignment_name = NULL;
        }
        m_species_data = NULL;
        m_current_species = NULL;
        m_current_count = 0;
        if (m_initialized_shell && m_shell) {
#ifdef DEBUG
            printf("ArbdbDataRetriever::closeConnection() Delete GB_shell.\n");
#endif
            delete m_shell;
            m_shell = NULL;
#ifdef DEBUG
            if (GB_shell::in_shell()) {
                printf(
                        "ArbdbDataRetriever::closeConnection() GB_shell still available :-(\n");
            } else {
                printf(
                        "ArbdbDataRetriever::closeConnection() GB_shell gone :-)\n");
            }
#endif
        }
    }
}

/*!
 * \brief Returns true if there is at least one entry in db
 *
 * \warning Returns false if not connected, no exception is thrown
 *
 * \return bool
 */
bool ArbdbDataRetriever::containsEntries() const {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    if (m_current_species) {
        return true;
    }
    return false;
}

/*!
 * \brief Returns number of sequence entries available in database
 *
 * \return unsigned long
 *
 * \exception runtime_error Thrown if database connection is closed
 */
ULONG ArbdbDataRetriever::countEntries() const {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    if (m_open) {
        return GBT_get_species_count(m_main_db);
    }
    throw std::runtime_error(
            "ArbdbDataRetriever::countEntries() Not connected to database.");
}

/*!
 * \brief Get all sequence entries available in the database
 *
 * \warning Overwrites the count of the already retrieved and sets
 *    the current entry to end list (= NULL)!
 *
 * \exception runtime_error Thrown if database connection is closed
 */
struct List* ArbdbDataRetriever::getEntries() const {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    if (m_open) {
        struct List *return_list = (struct List *) malloc(sizeof(struct List));
        if (return_list) {
            NewList(return_list);
            struct PTPanEntry *ps;
            ULONG num_species = 0;
            for (GBDATA *gb_species = GBT_first_species_rel_species_data(
                    m_species_data); gb_species;
                    gb_species = GBT_next_species(gb_species)) {
                // unlock before calling private method
                ps = retrieveEntry(gb_species);
                // lock again afterwards
                ps->pe_Node.ln_Pri = num_species;
                AddTail(return_list, &ps->pe_Node);
                num_species++;
            }
            m_current_count = num_species;
            m_current_species = NULL;
            return return_list;
        }
    }
    throw std::runtime_error(
            "ArbdbDataRetriever::getEntries() Not connected to database.");
}

/*!
 * \brief Returns true if this database (may) contain features
 *
 * It is still possible that a specific entry has no features!
 *
 * \return bool
 */
bool ArbdbDataRetriever::canContainFeatures() const {
    return true;
}

/*!
 * \brief Reset entry iterator back to first entry
 *
 * \exception runtime_error Thrown if database connection is closed
 */
void ArbdbDataRetriever::resetIterator() {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    if (m_open) {
        m_current_species = GBT_first_species_rel_species_data(m_species_data);
        m_current_count = 0;
    } else {
        throw std::runtime_error(
                "ArbdbDataRetriever::resetIterator() Not connected to database.");
    }
}

/*!
 * \brief Get next sequence entry in database
 *
 * \warning Caller must free the reference entry!
 *
 * \return struct PTPanEntry* Pointer to PTPanEntry, may be NULL if no next entry available
 *
 * \exception runtime_error Thrown if database connection is closed
 */
struct PTPanEntry* ArbdbDataRetriever::nextEntry() const {
    GBDATA *my_species = NULL;
    {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        if (m_open) {
            if (m_current_species) {
                my_species = m_current_species;
                {
                    boost::lock_guard<boost::mutex> lock(m_arbdb_mutex);
                    m_current_species = GBT_next_species(m_current_species);
                }
                m_current_count++;
            } else {
                return NULL;
            }
        } else {
            throw std::runtime_error(
                    "ArbdbDataRetriever::nextEntry() Not connected to database "
                            "or no further entries available.");
        }
    }
    if (my_species) {
        return retrieveEntry(my_species);
    }
    return NULL;
}

/*!
 * \brief Return true if there is another sequence entry in the database
 *
 * \return bool
 *
 * \exception runtime_error Thrown if database connection is closed
 */
bool ArbdbDataRetriever::hasNextEntry() const {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    if (m_open) {
        if (m_current_species) {
            return true;
        }
        return false;
    }
    throw std::runtime_error(
            "ArbdbDataRetriever::hasNextEntry() Not connected to database.");
}

/*!
 * \brief Returns the count of the already retrieved
 *
 * \return ULONG
 */
ULONG ArbdbDataRetriever::returnedEntryCount() const {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    return m_current_count - m_ignored_count;
}

/*!
 * \brief Returns number of ignored values
 *
 * \return ULONG
 */
ULONG ArbdbDataRetriever::ignoredEntryCount() const {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    return m_ignored_count;
}

/*!
 * \brief Check underlying data for a reference entry
 *
 * This may be e.g. E.coli in case of 16s RNA or whatever
 *
 * \return bool If reference entry is available
 *
 * \exception runtime_error Thrown if database connection is closed
 */
bool ArbdbDataRetriever::containsReferenceEntry() const {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    if (m_open) {
        STRPTR defaultref = GBT_get_default_ref(m_main_db);
        if (defaultref) {
            GBDATA *gb_extdata = GBT_find_SAI_rel_SAI_data(m_sai_data,
                    defaultref);
            free(defaultref);
            if (gb_extdata) {
                GBDATA *gb_data = GBT_read_sequence(gb_extdata,
                        m_alignment_name);
                if (gb_data) {
                    return true;
                }
            }
        }
        return false;
    }
    throw std::runtime_error(
            "ArbdbDataRetriever::containsReferenceEntry() Not connected to database.");
}

/*!
 * \brief Return reference entry (if available)
 *
 * This may be e.g. E.coli in case of 16s RNA or whatever
 *
 * \warning Caller must free the reference entry!
 *
 * \return ... If reference entry is available
 *
 * \exception runtime_error Thrown if database connection is closed
 */
struct PTPanReferenceEntry* ArbdbDataRetriever::getReferenceEntry() const {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    if (m_open) {
        STRPTR defaultref = GBT_get_default_ref(m_main_db);
        GBDATA *gb_extdata = GBT_find_SAI_rel_SAI_data(m_sai_data, defaultref);
        free(defaultref);

        /* prepare ecoli sequence */
        if (gb_extdata) {
            GBDATA *gb_data = GBT_read_sequence(gb_extdata, m_alignment_name);
            if (gb_data) {
                struct PTPanReferenceEntry* pre =
                        (struct PTPanReferenceEntry*) malloc(
                                sizeof(struct PTPanReferenceEntry));
                if (pre) {

                    ULONG abspos = 0;
                    STRPTR srcseq;
                    ULONG *posptr;

                    /* load sequence */
                    pre->pre_ReferenceSeqSize = GB_read_string_count(gb_data);
                    pre->pre_ReferenceSeq = GB_read_string(gb_data);

                    /* calculate look up table to speed up ecoli position calculation */
                    pre->pre_ReferenceBaseTable = (ULONG *) calloc(
                            pre->pre_ReferenceSeqSize + 1, sizeof(ULONG));
                    if (pre->pre_ReferenceBaseTable) {
                        srcseq = pre->pre_ReferenceSeq;
                        posptr = pre->pre_ReferenceBaseTable;
                        while (*srcseq) {
                            *posptr++ = abspos;
                            if (m_alpha_specs->seq_code_valid_table[(INT) *srcseq++]) {
                                abspos++;
                            }
                        }
                        *posptr = abspos;
                        return pre;
                    }
                }
            }
        }
        throw std::runtime_error("ArbdbDataRetriever::getReferenceEntry() "
                "No reference entry available or out of memory.");
    }
    throw std::runtime_error(
            "ArbdbDataRetriever::getReferenceEntry() Not connected to database.");
}

/*!
 * \brief Private method to retrieve single sequence entry data
 *
 * May return NULL if an error occurred
 */
struct PTPanEntry* ArbdbDataRetriever::retrieveEntry(GBDATA *gb_species) const {
    STRPTR seq_data = NULL;
    struct PTPanEntry *ps;
    {
        boost::lock_guard<boost::mutex> lock(m_arbdb_mutex);
        /* get name */
        GBDATA *gb_name = GB_find(gb_species, "name", SEARCH_CHILD);
        if (!gb_name) {
            m_ignored_count++;
            return NULL; /* huh? couldn't find the name of the sequence entry? */
        }
        STRPTR spname = GB_read_string(gb_name);

        /* get alignments */
        GBDATA *gb_ali = GB_find(gb_species, m_alignment_name, SEARCH_CHILD);
        if (!gb_ali) {
            free(spname);
            m_ignored_count++;
            return NULL; /* too bad, no alignment information found */
        }
        GBDATA *gb_data = GB_find(gb_ali, "data", SEARCH_CHILD);
        if (!gb_data) {
            fprintf(stderr, "Species '%s' has no data in '%s'\n", spname,
                    m_alignment_name);
            free(spname);
            m_ignored_count++;
            return NULL;
        }

        /* okay, cannot fail now anymore, allocate a PTPanEntry structure */
        ps = (struct PTPanEntry *) calloc(1, sizeof(struct PTPanEntry));

        /* write name and long name into the structure */
        ps->pe_Name = spname;
        gb_name = GB_find(gb_species, "full_name", SEARCH_CHILD);
        if (gb_name) {
            ps->pe_FullName = GB_read_string(gb_name);
        } else {
            ps->pe_FullName = strdup(ps->pe_Name);
        }

        ps->pe_SeqDataSize = GB_read_string_count(gb_data); // load alignment data
        seq_data = GB_read_string(gb_data);

        if (m_retrieve_features) {
#ifdef DEBUG
            printf("retrieving features for "); // TODO REMOVE ME
#endif
            ps->pe_FeatureContainer =
                    PTPanFeatureContainer::createSpecialMode();

            GB_ERROR error = 0;
#ifdef DEBUG
            ULONG feature_count = 0;
#endif
            PTPanFeature *feature;
            for (GBDATA *gb_gene = GEN_first_gene(gb_species);
                    gb_gene && !error; gb_gene = GEN_next_gene(gb_gene)) {
                const char *gene_name = GBT_read_name(gb_gene);
                char *copy_gene_name = GBS_global_string_copy("%s", gene_name);
                feature = PTPanFeature::createPtpanFeature();
                feature->pf_name = copy_gene_name; // TODO do we really need to copy this??
#ifdef DEBUG
                feature_count++;
#endif
                GEN_position *location = GEN_read_position(gb_gene);
                if (location) {
                    GEN_sortAndMergeLocationParts(location);
                    feature->pf_num_pos = location->parts;
                    feature->pf_pos =
                            PTPanPositionPair::createPtpanPositionPair(
                                    feature->pf_num_pos);
                    for (UWORD p = 0; p < feature->pf_num_pos; ++p) {
                        feature->pf_pos[p].pop_start = location->start_pos[p]
                                - 1;
                        feature->pf_pos[p].pop_end = location->stop_pos[p] - 1;
                        feature->pf_pos[p].complement =
                                location->complement[p] == 0 ? false : true;
                    }
                    // GEN_free_position(location);
                    free(location->start_pos); // rest is allocated together with start_pos
                    free(location);
                } else {
                    // TODO error
                }
                ps->pe_FeatureContainer->add(feature);
            }
#ifdef DEBUG
            printf("%s has %lu features (%lu)\n", ps->pe_Name, feature_count,
                    ps->pe_FeatureContainer->size());
#endif
        }
    }
    // compress sequence and store values in PTPanEntry
    if (m_alpha_specs->compressSequenceWithDotsAndHyphens(seq_data, ps)) {
        // printf("data corrupt!\n");
        // TODO exception or what else??
    }
    free(seq_data);

    return ps;
}


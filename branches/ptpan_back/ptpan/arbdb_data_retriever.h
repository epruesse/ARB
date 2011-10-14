/*!
 * \file arbdb_data_retriever.h
 *
 * \date 08.02.2011
 * \author Tilo Eissler
 *
 * Contributors: Chris Hodges, Jörg Böhnel
 */

#ifndef PTPAN_ARBDB_DATA_RETRIEVER_H_
#define PTPAN_ARBDB_DATA_RETRIEVER_H_

#include <string>

#include <boost/thread/mutex.hpp>

#include "abstract_data_retriever.h"

#include <arbdbt.h>

/*!
 * \brief The ARBDB data retriever returns the sequence entries of an
 * ARB database. (Note: in ARB, sequence entries are called 'species')
 *
 * \warning When using ArbdbDataRetriever, GB_shell object must be present, otherwise it will fail!
 * Just declare and define 'GB_shell m_shell;' before initializing  ArbdbDataRetriever!
 */
class ArbdbDataRetriever: public AbstractDataRetriever {
public:
    ArbdbDataRetriever(const std::string& db_name,
            AbstractAlphabetSpecifics* alpha_specs);
    virtual ~ArbdbDataRetriever();

    virtual bool connected() const;
    virtual void openConnection();
    virtual void closeConnection();

    virtual bool canContainFeatures() const;

    virtual bool containsEntries() const;
    virtual ULONG countEntries() const;
    virtual struct List* getEntries() const; // caller must clean this up!!

    virtual void resetIterator();
    virtual struct PTPanEntry* nextEntry() const;
    virtual bool hasNextEntry() const;
    virtual ULONG returnedEntryCount() const;
    virtual ULONG ignoredEntryCount() const;

    virtual bool containsReferenceEntry() const;
    virtual struct PTPanReferenceEntry* getReferenceEntry() const;

private:
    // user must provide some information, so default constructor is private
    // to avoid problems
    ArbdbDataRetriever();
    ArbdbDataRetriever(const ArbdbDataRetriever& adr);
    ArbdbDataRetriever& operator=(const ArbdbDataRetriever& adr);

    struct PTPanEntry* retrieveEntry(GBDATA *gb_species) const;

    mutable boost::mutex m_mutex;
    mutable boost::mutex m_arbdb_mutex;

    std::string m_db_name;
    AbstractAlphabetSpecifics *m_alpha_specs;
    bool m_retrieve_features;
    bool m_open;
    GB_shell *m_shell;
    bool m_initialized_shell;

    GBDATA *m_main_db; /* ARBDB interface */
    GBDATA *m_sai_data; /* Required to obtain reference sequence (E.Coli)*/
    STRPTR m_alignment_name; /* Name of the alignment in DB */
    GBDATA *m_species_data; /* DB pointer to entry data */
    mutable GBDATA *m_current_species; /* pointer to current species(= entry) (for iteration) */
    mutable ULONG m_current_count; /* actual count of retrieved entries */
    mutable ULONG m_ignored_count; /* count of ignored entries */
};

#endif /* PTPAN_ARBDB_DATA_RETRIEVER_H_ */

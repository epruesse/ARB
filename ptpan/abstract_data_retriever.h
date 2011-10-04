/*!
 * \file abstract_data_retriever.h
 *
 * \date 07.02.2011
 * \author Tilo Eissler
 *
 * Contributors: Chris Hodges, Jörg Böhnel
 */

#ifndef PTPAN_ABSTRACT_DATA_RETRIEVER_H_
#define PTPAN_ABSTRACT_DATA_RETRIEVER_H_

#include "abstract_alphabet_specifics.h"

/*!
 * \brief Class providing the basic interfaces ...
 *
 * TODO
 *
 * When implementing this class, an own constructor must
 * be provided to hand in the data source information.
 */
class AbstractDataRetriever {
protected:
 AbstractDataRetriever();
 AbstractDataRetriever(AbstractAlphabetSpecifics *specifics);

 AbstractAlphabetSpecifics *mp_alphabet_specifics;
 bool m_include_features;

public:
 virtual ~AbstractDataRetriever() = 0;

 // need to have some control over opening and closing of db connection
 virtual bool connected() const = 0;
 virtual void openConnection() = 0;
 virtual void closeConnection() = 0;

 virtual bool canContainFeatures() const = 0;
 virtual void setIncludeFeatures(bool include);
 virtual bool getIncludeFeatures() const;

 virtual bool containsEntries() const = 0;
 virtual unsigned long countEntries() const = 0;
 virtual struct List* getEntries() const = 0; // caller must clean this up!!

 /*!
  * \brief ...
  *
  * \warning Caller must free the reference entry!
  */
 virtual void resetIterator() = 0;
 virtual struct PTPanEntry* nextEntry() const = 0;
 virtual bool hasNextEntry() const = 0;
 virtual ULONG returnedEntryCount() const = 0;

 /*!
  * \brief Check underlying data for a reference entry
  *
  * This may be e.g. E.coli in case of 16s RNA or whatever
  *
  * \return bool If reference entry is available
  */
 virtual bool containsReferenceEntry() const = 0;

 /*!
  * \brief Return reference entry (if available)
  *
  * This may be e.g. E.coli in case of 16s RNA or whatever
  *
  * \warning Caller must free the reference entry!
  *
  * \return struct PTPanReferenceEntry* If reference entry is available
  */
 virtual struct PTPanReferenceEntry* getReferenceEntry() const = 0;

};

#endif /* PTPAN_ABSTRACT_DATA_RETRIEVER_H_ */

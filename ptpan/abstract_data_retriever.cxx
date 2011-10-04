/*!
 * \file abstract_data_retriever.cxx
 *
 * \date 07.02.2011
 * \author Tilo Eissler
 *
 * Contributors: Chris Hodges, Jörg Böhnel
 */

#include <stdexcept>

#include "abstract_data_retriever.h"

/*!
 * \brief constructor
 *
 * \warning Does not free AbstractAlphabetSpecifics when destroying object!
 *
 * \param specifics Pointer to AbstractAlphabetSpecifics
 *
 * \exception
 */
AbstractDataRetriever::AbstractDataRetriever(
        AbstractAlphabetSpecifics *specifics) :
        mp_alphabet_specifics(specifics), m_include_features(false) {
    if (!mp_alphabet_specifics) {
        throw std::invalid_argument(
                "AbstractDataRetriever(specifics): specifics pointer is empty!");
    }
}

/*!
 * \brief Private default constructor
 */
AbstractDataRetriever::AbstractDataRetriever() {
}

/*!
 * \brief destructor
 *
 * \warning Does not free AbstractAlphabetSpecifics when destroying object!
 */
AbstractDataRetriever::~AbstractDataRetriever() {
}

/*!
 * \brief Set flag to include feature information (genes etc.)
 *
 * \param include
 */
void AbstractDataRetriever::setIncludeFeatures(bool include) {
    m_include_features = include;
}

/*!
 * \brief Return include feature flag value
 *
 * Default value is false after construction!
 *
 * \return bool
 */
bool AbstractDataRetriever::getIncludeFeatures() const {
    return m_include_features;
}

/*!
 * \file ptpan_build_settings.h
 *
 * \date 12.02.2011
 * \author Tilo Eissler
 */

#ifndef PTPAN_BUILD_SETTINGS_H_
#define PTPAN_BUILD_SETTINGS_H_

#include <string>

#include "ptpan.h"
#include "abstract_alphabet_specifics.h"

/*!
 * \brief ... TODO
 */
class PtpanBuildSettings {
    PtpanBuildSettings(const PtpanBuildSettings&);
    PtpanBuildSettings& operator = (const PtpanBuildSettings&);
public:
    PtpanBuildSettings(AbstractAlphabetSpecifics *as, const std::string& name,
                       const std::string&         path     = "");
    virtual ~PtpanBuildSettings();

    ULONG setPruneLength(ULONG length);
    void setMemorySize(ULONG memory_size);
    void setMaxPrefixLength(ULONG max_prefix_length);
    void setIndexPath(const std::string& index_path);
    void setIndexName(const std::string& index_name);
    void setThreadNumber(ULONG num_threads);
    void setIncludeFeatures(bool include);
    void setVerbose(bool verbose);
    void setForce(bool force);
    void setPartitionRatio(double ratio);

    ULONG getPruneLength() const;
    ULONG getMemorySize() const;
    ULONG getMaxPrefixLength() const;
    const std::string getIndexPath() const;
    const std::string getIndexName() const;
    ULONG getIndexNameLength() const;
    ULONG getMaxPrefixLookupSize() const;
    ULONG getThreadNumber() const;
    bool getIncludeFeatures() const;
    bool isVerbose() const;
    bool isForce() const;
    double getPartitionRatio() const;

private:
    PtpanBuildSettings();
    AbstractAlphabetSpecifics *m_as;

    ULONG m_prune_length;
    ULONG m_memory_size;
    ULONG m_max_prefix_len;

    std::string m_path;
    std::string m_index_name;

    ULONG m_max_prefix_lookup_size;

    ULONG m_num_threads;
    bool m_include_features;
    bool m_verbose;
    bool m_force;

    double m_ratio;
};

#endif /* PTPAN_BUILD_SETTINGS_H_ */

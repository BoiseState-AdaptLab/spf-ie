#ifndef SPFIE_SPFCOMPUTATION_HPP
#define SPFIE_SPFCOMPUTATION_HPP

#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "iegenlib.h"

namespace spf_ie {

struct IEGenStmtInfo;

/*!
 * \struct SPFComputation
 *
 * \brief SPF representation of a computation (group of statements), ready for
 * further use.
 */
struct SPFComputation {
    //! Print out all the information represented in this Computation for
    //! debug purposes
    void printInfo();

    //! Data spaces accessed in the computation
    std::unordered_set<std::string> dataSpaces;
    //! Map of statement names -> the statement's corresponding information
    std::map<std::string, IEGenStmtInfo> stmtsInfoMap;
};

/*!
 * \struct IEGenStmtInfo
 *
 * \brief Information attached to a statement, as IEGenLib mathematical
 * objects.
 */
struct IEGenStmtInfo {
    IEGenStmtInfo(
        std::string stmtSourceCode, std::string iterationSpaceStr,
        std::string executionScheduleStr,
        std::vector<std::pair<std::string, std::string>> dataReadsStrs,
        std::vector<std::pair<std::string, std::string>> dataWritesStrs)
        : stmtSourceCode(stmtSourceCode) {
        iterationSpace = std::make_unique<iegenlib::Set>(iterationSpaceStr);
        executionSchedule =
            std::make_unique<iegenlib::Relation>(executionScheduleStr);
        for (const auto& readInfo : dataReadsStrs) {
            dataReads.push_back(
                {readInfo.first,
                 std::make_unique<iegenlib::Relation>(readInfo.second)});
        }
        for (const auto& writeInfo : dataWritesStrs) {
            dataWrites.push_back(
                {writeInfo.first,
                 std::make_unique<iegenlib::Relation>(writeInfo.second)});
        }
    };

    //! Source code of the statement, for debugging purposes
    std::string stmtSourceCode;
    //! Iteration space of a statement
    std::unique_ptr<iegenlib::Set> iterationSpace;
    //! Execution schedule of a single statement
    std::unique_ptr<iegenlib::Relation> executionSchedule;
    //! Read dependences of a statement, pairing data space name to relation
    std::vector<std::pair<std::string, std::unique_ptr<iegenlib::Relation>>>
        dataReads;
    //! Write dependences of a statement, pairing data space name to relation
    std::vector<std::pair<std::string, std::unique_ptr<iegenlib::Relation>>>
        dataWrites;
};

}  // namespace spf_ie

#endif

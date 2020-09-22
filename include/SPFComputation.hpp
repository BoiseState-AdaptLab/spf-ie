#ifndef SPFIE_SPFCOMPUTATION_HPP
#define SPFIE_SPFCOMPUTATION_HPP

#include <map>
#include <string>
#include <unordered_set>

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
    IEGenStmtInfo() : iterationSpace("{}"), executionSchedule("{[]->[]}"){};

    //! Source code of the statement, for debugging purposes
    std::string stmtSourceCode;
    //! Iteration space of a statement
    iegenlib::Set iterationSpace;
    //! Execution schedule of a single statement
    iegenlib::Relation executionSchedule;
    //! Read dependences of a statement
    std::vector<iegenlib::Relation> dataReads;
    //! Write dependences of a statement
    std::vector<iegenlib::Relation> dataWrites;
};

}  // namespace spf_ie

#endif

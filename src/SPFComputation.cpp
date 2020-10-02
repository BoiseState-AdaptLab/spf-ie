#include "SPFComputation.hpp"

#include <sstream>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Utils.hpp"
#include "clang/AST/Expr.h"
#include "llvm/Support/raw_ostream.h"

namespace spf_ie {

/* SPFComputation */

void SPFComputation::printInfo() {
    std::ostringstream dataSpacesOutput;
    dataSpacesOutput << "{";
    for (auto it = dataSpaces.begin(); it != dataSpaces.end(); ++it) {
        if (it != dataSpaces.begin()) {
            dataSpacesOutput << ", ";
        }
        dataSpacesOutput << *it;
    }
    dataSpacesOutput << "}\n";

    std::ostringstream stmts;
    std::ostringstream iterSpaces;
    std::ostringstream execSchedules;
    std::ostringstream dataReads;
    std::ostringstream dataWrites;
    for (auto it = stmtsInfoMap.begin(); it != stmtsInfoMap.end(); ++it) {
        stmts << it->first << ": " << it->second.stmtSourceCode << "\n";
        iterSpaces << it->first << ": "
                   << it->second.iterationSpace.prettyPrintString() << "\n";
        execSchedules << it->first << ": "
                      << it->second.executionSchedule.prettyPrintString()
                      << "\n";
        dataReads << it->first << ":";
        if (it->second.dataReads.empty()) {
            dataReads << " none";
        } else {
            dataReads << "{\n";
            for (auto read_it = it->second.dataReads.begin();
                 read_it != it->second.dataReads.end(); ++read_it) {
                dataReads << "    " << read_it->first << ": "
                          << read_it->second.prettyPrintString() << "\n";
            }
            dataReads << "}";
        }
        dataReads << "\n";
        dataWrites << it->first << ":";
        if (it->second.dataWrites.empty()) {
            dataWrites << " none";
        } else {
            dataWrites << "{\n";
            for (auto write_it = it->second.dataWrites.begin();
                 write_it != it->second.dataWrites.end(); ++write_it) {
                dataWrites << "    " << write_it->first << ": "
                           << write_it->second.prettyPrintString() << "\n";
            }
            dataWrites << "}";
        }
        dataWrites << "\n";
    }

    llvm::outs() << "Statements:\n" << stmts.str();
    llvm::outs() << "\nIteration spaces:\n" << iterSpaces.str();
    llvm::outs() << "\nExecution schedules:\n" << execSchedules.str();
    llvm::outs() << "\nData spaces: " << dataSpacesOutput.str();
    llvm::outs() << "\nArray reads:\n" << dataReads.str();
    llvm::outs() << "\nArray writes:\n" << dataWrites.str();
    llvm::outs() << "\n";
}

}  // namespace spf_ie

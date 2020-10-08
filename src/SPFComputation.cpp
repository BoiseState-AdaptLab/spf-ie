#include "SPFComputation.hpp"

#include <sstream>
#include <unordered_set>
#include <utility>
#include <vector>

#include "iegenlib.h"

namespace spf_ie {

/* SPFComputation */

void SPFComputation::printInfo() {
    std::ostringstream stmts;
    std::ostringstream iterSpaces;
    std::ostringstream execSchedules;
    std::ostringstream dataReads;
    std::ostringstream dataWrites;
    std::ostringstream dataSpacesOutput;

    for (const auto& it : stmtsInfoMap) {
        stmts << it.first << ": " << it.second.stmtSourceCode << "\n";
        iterSpaces << it.first << ": "
                   << it.second.iterationSpace->prettyPrintString() << "\n";
        execSchedules << it.first << ": "
                      << it.second.executionSchedule->prettyPrintString()
                      << "\n";
        dataReads << it.first << ":";
        if (it.second.dataReads.empty()) {
            dataReads << " none";
        } else {
            dataReads << "{\n";
            for (const auto& read_it : it.second.dataReads) {
                dataReads << "    " << read_it.first << ": "
                          << read_it.second->prettyPrintString() << "\n";
            }
            dataReads << "}";
        }
        dataReads << "\n";
        dataWrites << it.first << ":";
        if (it.second.dataWrites.empty()) {
            dataWrites << " none";
        } else {
            dataWrites << "{\n";
            for (const auto& write_it : it.second.dataWrites) {
                dataWrites << "    " << write_it.first << ": "
                           << write_it.second->prettyPrintString() << "\n";
            }
            dataWrites << "}";
        }
        dataWrites << "\n";
    }
    dataSpacesOutput << "{";
    for (const auto& it : dataSpaces) {
        if (it != *dataSpaces.begin()) {
            dataSpacesOutput << ", ";
        }
        dataSpacesOutput << it;
    }
    dataSpacesOutput << "}\n";

    llvm::outs() << "Statements:\n" << stmts.str();
    llvm::outs() << "\nIteration spaces:\n" << iterSpaces.str();
    llvm::outs() << "\nExecution schedules:\n" << execSchedules.str();
    llvm::outs() << "\nData spaces: " << dataSpacesOutput.str();
    llvm::outs() << "\nArray reads:\n" << dataReads.str();
    llvm::outs() << "\nArray writes:\n" << dataWrites.str();
    llvm::outs() << "\n";
}

}  // namespace spf_ie

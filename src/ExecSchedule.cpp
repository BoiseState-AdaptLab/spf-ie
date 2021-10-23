#include "ExecSchedule.hpp"

#include <string>
#include "Utils.hpp"

namespace spf_ie {

/* ExecSchedule */

ExecSchedule::ExecSchedule(const ExecSchedule &other) {
  scheduleTuple = other.scheduleTuple;
}

void ExecSchedule::pushValue(const ScheduleVal &value) {
  scheduleTuple.push_back(std::make_shared<ScheduleVal>(value));
}

ScheduleVal ExecSchedule::popValue() {
  ScheduleVal value = *scheduleTuple.back();
  scheduleTuple.pop_back();
  return value;
}

void ExecSchedule::advanceSchedule() {
  if (scheduleTuple.empty() || scheduleTuple.back()->valueIsVar) {
    scheduleTuple.push_back(std::make_shared<ScheduleVal>(0));
  } else {
    std::shared_ptr<ScheduleVal> top = scheduleTuple.back();
    scheduleTuple.pop_back();
    scheduleTuple.push_back(std::make_shared<ScheduleVal>(top->num + 1));
  }
}

void ExecSchedule::skipToPosition(unsigned int newPosition) {
  std::shared_ptr<ScheduleVal> top = scheduleTuple.back();
  if (top->valueIsVar) {
    Utils::printErrorAndExit("Cannot skip to position " + std::to_string(newPosition) +
        ", because top of stack is not a number.");
  }
  scheduleTuple.pop_back();
  scheduleTuple.push_back(std::make_shared<ScheduleVal>(newPosition));
}

/* ScheduleVal */

ScheduleVal::ScheduleVal(std::string var) : var(var), valueIsVar(true) {}

ScheduleVal::ScheduleVal(int num) : num(num), valueIsVar(false) {}

}  // namespace spf_ie

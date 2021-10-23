#ifndef SPFIE_EXECSCHEDULE_HPP
#define SPFIE_EXECSCHEDULE_HPP

#include <memory>
#include <string>
#include <vector>

namespace spf_ie {

struct ScheduleVal;

/*!
 * \struct ExecSchedule
 *
 * \brief An execution schedule tuple, plus a few utilities for it
 */
struct ExecSchedule {
  ExecSchedule() = default;

  //! Copy constructor
  ExecSchedule(const ExecSchedule &other);

  //! Add a value to the end of the schedule tuple
  void pushValue(const ScheduleVal &value);

  //! Remove the value at the end of the schedule tuple
  //! \return the removed value
  ScheduleVal popValue();

  //! Move statement number forward in the execution schedule
  void advanceSchedule();

  //! Jump the top of the schedule to the given position
  void skipToPosition(unsigned int newPosition);

  //! Get the dimension of the execution schedule
  int getDimension() const { return scheduleTuple.size(); }

  //! Actual execution schedule ordering tuple
  std::vector<std::shared_ptr<ScheduleVal>> scheduleTuple;
};

/*!
 * \struct ScheduleVal
 *
 * \brief An entry of an execution schedule, which may be a variable or
 * simply a number.
 */
struct ScheduleVal {
  explicit ScheduleVal(std::string var);
  explicit ScheduleVal(int num);

  std::string var;
  int num{};
  //! Whether this ScheduleVal contains a variable
  bool valueIsVar;
};

}  // namespace spf_ie

#endif

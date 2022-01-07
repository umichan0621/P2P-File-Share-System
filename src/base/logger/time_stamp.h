#pragma once
#include <stdint.h>
#include <string>
#include <inttypes.h>


class TimeStamp {
public:
//
// Constucts an invalid TimeStamp.
//
  TimeStamp()
  : m_microSecondsSinceEpoch(0){
  }

//
// Constucts a TimeStamp at specific time
//
// @param microSecondsSinceEpoch
  explicit TimeStamp(int64_t microSecondsSinceEpochArg)
  : m_microSecondsSinceEpoch(microSecondsSinceEpochArg){
  }

  int64_t microSecondsSinceEpoch() const { return m_microSecondsSinceEpoch; }

//
// Get time of now.
//
  std::string toString() const;

  bool valid() const { return m_microSecondsSinceEpoch > 0; }

  static TimeStamp now();
  static TimeStamp invalid() { return TimeStamp(); }
  static TimeStamp addTime(TimeStamp timestamp, double seconds)
  {
    int64_t delta = static_cast<int64_t>(seconds * TimeStamp::kMicroSecondsPerSecond);
    return TimeStamp(timestamp.microSecondsSinceEpoch() + delta);
  }
  
  void swap(TimeStamp& other)
  {
    std::swap(m_microSecondsSinceEpoch, other.m_microSecondsSinceEpoch);
  }
  
  static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
  int64_t m_microSecondsSinceEpoch;
};

inline bool operator<(TimeStamp lhs, TimeStamp rhs)
{
  return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(TimeStamp lhs, TimeStamp rhs)
{
  return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}


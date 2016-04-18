#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>
#include <time.h>
#include <sys/time.h>

class TimeVal {
 public:
  TimeVal(uint32_t sec = 0, uint32_t usec = 0);
  TimeVal(const timeval& time);
  TimeVal& SetNow();

  timeval Value() const;
  uint32_t Seconds()  const;
  uint32_t USeconds() const;

  bool operator ==(const TimeVal& other) const;
   bool operator !=(const TimeVal& other) const;
  bool operator <(const TimeVal& other) const;
  bool operator >(const TimeVal& other) const;
  TimeVal operator-(const TimeVal& other) const;
  TimeVal operator+(const TimeVal& other) const;

  static int32_t MsDiff(const TimeVal& lv, const TimeVal& rv);
  static TimeVal Now();

 private:
  timeval tv_;
};

#endif  // _UTILS_H

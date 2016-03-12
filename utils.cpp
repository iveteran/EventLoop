#include "utils.h"

TimeVal::TimeVal(uint32_t sec, uint32_t usec)
{
  tv_.tv_sec = sec;
  tv_.tv_usec = usec;
}

TimeVal::TimeVal(const timeval& time) : tv_(time)
{ }

TimeVal& TimeVal::SetNow()
{
  gettimeofday(&tv_, NULL);
  return *this;
}

timeval TimeVal::Value() const{ return tv_; }
uint32_t TimeVal::Seconds() const { return tv_.tv_sec; }
uint32_t TimeVal::USeconds() const { return tv_.tv_usec; }

bool TimeVal::operator==(const TimeVal& other) const {
  return tv_.tv_sec == other.Seconds() && tv_.tv_usec == other.USeconds();
}

bool TimeVal::operator!=(const TimeVal& other) const {
  return tv_.tv_sec != other.Seconds() || tv_.tv_usec != other.USeconds();
}

bool TimeVal::operator<(const TimeVal& other) const {
  return (tv_.tv_sec < other.Seconds()) || (tv_.tv_sec == other.Seconds() && tv_.tv_usec < other.USeconds());
}

bool TimeVal::operator>(const TimeVal& other) const {
  return (tv_.tv_sec > other.Seconds()) || (tv_.tv_sec == other.Seconds() && tv_.tv_usec > other.USeconds());
}

TimeVal TimeVal::operator-(const TimeVal& other) const {
  if (*this < other) return TimeVal(0, 0);
  uint32_t diff_sec = tv_.tv_usec;
  uint32_t diff_usec = tv_.tv_usec;
  if (diff_usec < other.USeconds()) {
    // 借位
    diff_sec -= 1;
    diff_usec += 1000000;
  }
  diff_sec -= other.Seconds();
  diff_usec -= other.USeconds();
  return TimeVal(diff_sec, diff_usec);
}

TimeVal TimeVal::operator+(const TimeVal& other) const {
  uint32_t diff_sec = tv_.tv_sec + other.Seconds();      // XXX: integer overflow???
  uint32_t diff_usec = tv_.tv_usec + other.USeconds();
  diff_sec += diff_usec / 1000000;
  diff_usec %= 1000000;
  //printf("diff_sec: %d, diff_usec: %d\n", diff_sec, diff_usec);
  return TimeVal(diff_sec, diff_usec);
}

int32_t TimeVal::MsDiff(const TimeVal& lv, const TimeVal& rv) {
  return (((int64_t)lv.Seconds()) * 1000 + lv.USeconds() / 1000) - (((int64_t)rv.Seconds()) * 1000 + rv.USeconds() / 1000);
}

TimeVal TimeVal::Now() { TimeVal time; return time.SetNow(); }

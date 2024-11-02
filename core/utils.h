#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>
#include <string>

using std::string;

namespace evt_loop {

const int DUMP_MAX_BYTES = 256;

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

string DumpHex(const string& data, size_t max_bytes = 0);
string DumpHex(const char* data, size_t size, size_t max_bytes);
string DumpHexWithChars(const string& data, size_t max_bytes = 0);
string DumpHexWithChars(const char* data, size_t size, size_t max_bytes = 0);

}  // namespace evt_loop

#endif  // _UTILS_H

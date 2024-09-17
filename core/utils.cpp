#include <sstream>
#include "utils.h"

namespace evt_loop {

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

void SocketAddrToIPAddress(const struct sockaddr_in& sock_addr, IPAddress& ip_addr)
{
  char buffer[INET_ADDRSTRLEN] = {0};
  inet_ntop(sock_addr.sin_family, (void*)&sock_addr.sin_addr, buffer, sizeof(buffer));
  ip_addr.ip_.assign(buffer);
  ip_addr.port_ = sock_addr.sin_port;
}

void SocketAddrToIPAddress(const struct sockaddr_in6& sock_addr, IPAddress& ip_addr)
{
  char buffer[INET6_ADDRSTRLEN] = {0};
  inet_ntop(sock_addr.sin6_family, (void*)&sock_addr.sin6_addr, buffer, sizeof(buffer));
  ip_addr.ip_.assign(buffer);
  ip_addr.port_ = sock_addr.sin6_port;
}

bool is_visable_char(char c)
{
    return c >= 0x20 && c <= 0x7e;
}

string DumpHex(const string& data, size_t max_bytes)
{
  size_t bytes_to_dump = (max_bytes == 0 || max_bytes > data.size()) ? data.size() : max_bytes;
  size_t i = 0;
  std::stringstream ss;
  for (; i < bytes_to_dump; i++) {
      char strbuf[4];
      snprintf(strbuf, sizeof(strbuf), "%02X", data[i]);
      ss << strbuf;
      if (i != 0 && i != data.size()-1 && (i + 1) % 16 == 0)
          ss << '\n';
      else
          ss << ' ';
      if (i != 0 && (i + 1) % 8 == 0 && (i + 1) % 16 != 0)
          ss << ' ';
  }

  return ss.str();
}

string DumpHex(const string& data, const char* tag, size_t max_bytes)
{
  std::stringstream ss;
  ss << tag << ": \n";
  size_t bytes_to_dump = (max_bytes == 0 || max_bytes > data.size()) ? data.size() : max_bytes;
  size_t i = 0;
  size_t j = 0;
  size_t k = 0;
  const size_t LINE_BYTES = 16;
  for (; i < bytes_to_dump; i+=j) {
    size_t rest_bytes = bytes_to_dump-i;
    for (j=0; j<rest_bytes && j<LINE_BYTES; j++) {
      char strbuf[4];
      snprintf(strbuf, sizeof(strbuf), "%02X", data[i+j]);
      ss << strbuf;
    }
    ss << "  ";
    if (rest_bytes < LINE_BYTES) {
      for (size_t n=0; n<LINE_BYTES-rest_bytes; n++) {
        ss << "   ";  // 3 blanks
      }
    }

    for (k=0; k<rest_bytes && k<LINE_BYTES; k++) {
      int pos = i+k;
      if (is_visable_char(data[pos])) {
        ss << data[pos];
      } else if (data[pos] == '\n') {
        ss << "\\n";
      } else if (data[pos] == '\r') {
        ss << "\\r";
      } else {
        ss << '.';
      }
    }
  }

  return ss.str();
}

}  // namespace evt_loop

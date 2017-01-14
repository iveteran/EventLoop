#ifndef _URL_REQUEST_H
#define _URL_REQUEST_H

#include <map>
#include <curl/curl.h>
#include "fd_handler.h"
#include "el.h"

using namespace evt_loop;

namespace url_api {

const int ERR_SUCCESS = 0;
const int ERR_TIMEOUT = 1;

enum { DEFAULT_TIMEOUT = 10/*seconds*/ };

class URLRequest;
class URLRequestReactor;

typedef std::function<void (URLRequest*, int errcode)> CompletionCallback;
typedef std::function<size_t (char* data, size_t size)> OnReadDataCallback;

class URLRequest : public IOEvent
{
  friend URLRequestReactor;

  public:
  URLRequest(URLRequestReactor* url_reactor);
  ~URLRequest();
  bool Init(const char* url, const CompletionCallback& cb, uint32_t timeout = DEFAULT_TIMEOUT);
  CURL* GetCURL() { return curl_; }
  uint32_t GetTimeout() const { return timeout_; }
  void EnableVerbose() { curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1); }
  const string& GetResult() const { return result_data_; }
  const char* GetLastError() const { return errstr_; }

  protected:
  time_t GetCreateTime() const { return ctime_; }
  void SetSocketEvent(curl_socket_t sockfd, int action);
  void RemoveSocketEvent();

  void OnComplete();
  void OnTimeout();
  void OnEvents(uint32_t events);

  protected:
  URLRequestReactor*    url_reactor_;
  CURL*                 curl_;
  uint32_t              ctime_;
  uint32_t              timeout_;
  CompletionCallback    completion_cb_;
  std::string           result_data_;
  char                  errstr_[256];
};

class HTTPRequest : public URLRequest
{
  public:
  HTTPRequest(URLRequestReactor* url_reactor);
  bool Init(const char* url, const CompletionCallback& cb, uint32_t timeout = DEFAULT_TIMEOUT);

  protected:
  static size_t WriteData_Callback(void *content, size_t size, size_t nmemb, void *userp);
};

class SMTPRequest : public URLRequest
{
  public:
  SMTPRequest(URLRequestReactor* url_reactor);
  ~SMTPRequest();
  int Init(const char* url, const CompletionCallback& cb, uint32_t timeout = DEFAULT_TIMEOUT);
  bool SetLogin(const char* user, const char* pass);
  bool SetMail(const char* from, const char* to, const char* subject, const char* content, const char* cc = NULL);
  void EnableSSL() { curl_easy_setopt(curl_, CURLOPT_USE_SSL, CURLUSESSL_TRY); }

  protected:
  static size_t ReadData_Callback(void *write_ptr, size_t size, size_t nmemb, void *userp);

  private:
  std::queue<string> mail_items_;
  struct curl_slist *mail_recipients_;
};

class URLRequestReactor
{
  friend URLRequest;

  public:
  URLRequestReactor();
  ~URLRequestReactor();
  bool Init();

  HTTPRequest* NewHTTPRequest(const char* url, const CompletionCallback& cb, uint32_t timeout = DEFAULT_TIMEOUT);
  SMTPRequest* NewSMTPRequest(const char* url, const CompletionCallback& cb, uint32_t timeout = DEFAULT_TIMEOUT);
  void RemoveRequest(URLRequest* curl);

  CURLM* GetCURLM() { return curlm_; }
  URLRequest* GetURLRequest(CURL *curl);

  protected:
  void StopCURLTimer() { curl_timer_.Stop(); }
  void CheckRequestStatus();

  void OnCURLTimeout(TimerEvent* timer);
  void OnCheckRequestTimeout(TimerEvent* timer);
  static int StartCURLTimer(CURLM *curlm, long timeout_ms, void *userp);
  static int HandleSocketEvent(CURL *curl, curl_socket_t s, int action, void *userp, void *socketp);

  private:
  CURLM*            curlm_;
  PeriodicTimer     curl_timer_;
  PeriodicTimer     timeout_timer_;
  std::map<CURL*, URLRequest*>  request_map_;
  std::list<URLRequest*>        request_queue_;
};

}  // namespace url_api

#endif  // _URL_REQUEST_H

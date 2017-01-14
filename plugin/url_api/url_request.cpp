#include "url_request.h"

namespace url_api {

/**
 * URLRequest
 */
URLRequest::URLRequest(URLRequestReactor* url_reactor) : url_reactor_(url_reactor), ctime_(Now())
{
  curl_ = curl_easy_init();
}
URLRequest::~URLRequest()
{
  RemoveSocketEvent();
}
bool URLRequest::Init(const char* url, const CompletionCallback& cb, uint32_t timeout)
{
  completion_cb_ = cb;
  timeout_ = timeout;

  curl_easy_setopt(curl_, CURLOPT_URL, url);
  curl_multi_add_handle(url_reactor_->GetCURLM(), curl_);
  return true;
}
void URLRequest::SetSocketEvent(curl_socket_t sockfd, int action)
{
  //printf("%s: socket: %d, action: %d\n", __FUNCTION__, sockfd, action);
  uint32_t events = (action & CURL_POLL_IN ? IOEvent::READ : 0) | ( action & CURL_POLL_OUT ? IOEvent::WRITE : 0);
  if (FD() < 0) {
    WatchEvents(sockfd, events);
    curl_multi_assign(url_reactor_->GetCURLM(), sockfd, this);
  } else {
    UpdateEvents(events);
  }
}
void URLRequest::RemoveSocketEvent()
{
  curl_multi_assign(url_reactor_->GetCURLM(), FD(), NULL);
}
void URLRequest::OnComplete()
{
  char *request_url;
  curl_easy_getinfo(curl_, CURLINFO_EFFECTIVE_URL, &request_url);
  printf("%s DONE\n", request_url);

  completion_cb_(this, ERR_SUCCESS);

  curl_multi_remove_handle(url_reactor_, curl_);
  curl_easy_cleanup(curl_);
}
void URLRequest::OnTimeout()
{
  char *request_url;
  curl_easy_getinfo(curl_, CURLINFO_EFFECTIVE_URL, &request_url);
  printf("%s TIMEOUT(%ld)\n", request_url, Now() - ctime_);

  strncpy(errstr_, "timeout", sizeof(errstr_));
  completion_cb_(this, ERR_TIMEOUT);

  curl_multi_remove_handle(url_reactor_, curl_);
  curl_easy_cleanup(curl_);
}
void URLRequest::OnEvents(uint32_t events)
{
  int running_handles;
  int action = 0;

  url_reactor_->StopCURLTimer();

  if(events & IOEvent::READ)
    action |= CURL_CSELECT_IN;
  if(events & IOEvent::WRITE)
    action |= CURL_CSELECT_OUT;

  curl_multi_socket_action(url_reactor_->GetCURLM(), FD(), action, &running_handles);

  url_reactor_->CheckRequestStatus();
}

/**
 * HTTPRequest
 */
HTTPRequest::HTTPRequest(URLRequestReactor* url_reactor) : URLRequest(url_reactor)
{
}
bool HTTPRequest::Init(const char* url, const CompletionCallback& cb, uint32_t timeout)
{
  URLRequest::Init(url, cb, timeout);
  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, HTTPRequest::WriteData_Callback);
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);
  return true;
}
size_t HTTPRequest::WriteData_Callback(void *content, size_t size, size_t nmemb, void *userp)
{
  HTTPRequest* url_request = (HTTPRequest*)userp;
  size_t realsize = size * nmemb;
  url_request->result_data_.append((char*)content, realsize);
  return realsize;
}

/**
 * SMTPRequest
 */
SMTPRequest::SMTPRequest(URLRequestReactor* url_reactor) : URLRequest(url_reactor), mail_recipients_(NULL)
{
}
SMTPRequest::~SMTPRequest()
{
  curl_slist_free_all(mail_recipients_);
}
int SMTPRequest::Init(const char* url, const CompletionCallback& cb, uint32_t timeout)
{
  URLRequest::Init(url, cb, timeout);
  curl_easy_setopt(curl_, CURLOPT_READFUNCTION, &SMTPRequest::ReadData_Callback);
  curl_easy_setopt(curl_, CURLOPT_READDATA, this);
  curl_easy_setopt(curl_, CURLOPT_UPLOAD, 1L);
  return 0;
}
bool SMTPRequest::SetLogin(const char* user, const char* pass)
{
  if (user == NULL || pass == NULL) return false;
  curl_easy_setopt(curl_, CURLOPT_USERNAME, user);
  curl_easy_setopt(curl_, CURLOPT_PASSWORD, pass);
  return true;
}
bool SMTPRequest::SetMail(const char* from, const char* to, const char* subject, const char* content, const char* cc)
{
  if (from == NULL || to == NULL || subject == NULL || content == NULL) return false;

  curl_easy_setopt(curl_, CURLOPT_MAIL_FROM, from);
  mail_items_.push(string("From: ") + from + "\r\n");

  mail_recipients_ = curl_slist_append(mail_recipients_, to);
  mail_items_.push(string("To: ") + to + "\r\n");
  if (cc) {
    mail_recipients_ = curl_slist_append(mail_recipients_, cc);
    mail_items_.push(string("CC: ") + cc + "\r\n");
  }
  curl_easy_setopt(curl_, CURLOPT_MAIL_RCPT, mail_recipients_);

  mail_items_.push(string("Subject: ") + subject + "\r\n");
  mail_items_.push("\r\n");

  mail_items_.push(string(content) + "\r\n");
  return true;
}
size_t SMTPRequest::ReadData_Callback(void *write_ptr, size_t size, size_t nmemb, void *userp)
{
  //printf("[SMTPRequest::ReadData_Callback]\n");
  if (size == 0 || nmemb == 0 || (size * nmemb) < 1)
    return 0;

  std::queue<string>& mail_items = ((SMTPRequest *)userp)->mail_items_;
  if (! mail_items.empty()) {
    string& item = mail_items.front();
    //printf("upload email item: %s\n", item.c_str());
    size_t item_size = item.size();
    memcpy(write_ptr, item.c_str(), item_size);
    mail_items.pop();
    return item_size;
  }
  return 0;
}

/**
 * URLRequestReactor
 */
URLRequestReactor::URLRequestReactor() :
  curl_timer_(std::bind(&URLRequestReactor::OnCURLTimeout, this, std::placeholders::_1)),
  timeout_timer_(std::bind(&URLRequestReactor::OnCheckRequestTimeout, this, std::placeholders::_1))
{
  curl_global_init(CURL_GLOBAL_ALL);
  curlm_ = curl_multi_init();

  TimeVal tv(1, 0);
  timeout_timer_.SetInterval(tv);
}
URLRequestReactor::~URLRequestReactor()
{
  curl_multi_cleanup(curlm_);
  curl_timer_.Stop();
  timeout_timer_.Stop();
}
bool URLRequestReactor::Init()
{
  curl_multi_setopt(curlm_, CURLMOPT_SOCKETFUNCTION, &URLRequestReactor::HandleSocketEvent);
  curl_multi_setopt(curlm_, CURLMOPT_SOCKETDATA, this);
  curl_multi_setopt(curlm_, CURLMOPT_TIMERFUNCTION, &URLRequestReactor::StartCURLTimer);
  curl_multi_setopt(curlm_, CURLMOPT_TIMERDATA, this);
  return true;
}
void URLRequestReactor::OnCURLTimeout(TimerEvent* timer)
{
  int running_handles;
  curl_multi_socket_action(curlm_, CURL_SOCKET_TIMEOUT, 0, &running_handles);
  CheckRequestStatus();
}
void URLRequestReactor::OnCheckRequestTimeout(TimerEvent* timer)
{
  while (!request_queue_.empty()) {
    URLRequest* request = request_queue_.front();
    if (Now() - request->GetCreateTime() > request->GetTimeout()) {
      request->OnTimeout();
      RemoveRequest(request);
    } else {
      break;
    }
  }
}
HTTPRequest* URLRequestReactor::NewHTTPRequest(const char* url, const CompletionCallback& cb, uint32_t timeout)
{
  HTTPRequest* request = new HTTPRequest(this);
  request->Init(url, cb, timeout);
  request_map_.insert(std::make_pair(request->GetCURL(), request));
  request_queue_.push_back(request);
  if (!timeout_timer_.IsRunning())
    timeout_timer_.Start();
  return request;
}
SMTPRequest* URLRequestReactor::NewSMTPRequest(const char* url, const CompletionCallback& cb, uint32_t timeout)
{
  SMTPRequest* request = new SMTPRequest(this);
  request->Init(url, cb, timeout);
  request_map_.insert(std::make_pair(request->GetCURL(), request));
  request_queue_.push_back(request);

  if (!timeout_timer_.IsRunning())
    timeout_timer_.Start();
  return request;
}
void URLRequestReactor::RemoveRequest(URLRequest* url_request)
{
  request_map_.erase(url_request->GetCURL());
  request_queue_.remove(url_request);
  delete url_request;

  if (request_queue_.empty() && timeout_timer_.IsRunning())
    timeout_timer_.Stop();
}
URLRequest* URLRequestReactor::GetURLRequest(CURL* curl) {
  auto iter = request_map_.find(curl);
  return iter != request_map_.end() ? iter->second : NULL;
}
int URLRequestReactor::StartCURLTimer(CURLM* curlm, long timeout_ms, void *userp)
{
  URLRequestReactor* url_reactor = (URLRequestReactor*)userp;
  if (timeout_ms < 0) {
    url_reactor->curl_timer_.Stop();
  } else if(timeout_ms == 0) {
    int running_handles;
    curl_multi_socket_action(url_reactor->GetCURLM(), CURL_SOCKET_TIMEOUT, 0, &running_handles);
    url_reactor->CheckRequestStatus();
  } else {
    TimeVal timeval(0, timeout_ms * 1000);
    url_reactor->curl_timer_.SetInterval(timeval);
    url_reactor->curl_timer_.Start();
  }
  return 0;
}
int URLRequestReactor::HandleSocketEvent(CURL* curl, curl_socket_t s, int action, void *userp, void *socketp)
{
  URLRequestReactor* url_reactor = (URLRequestReactor*)userp;
  URLRequest* url_request = url_reactor->GetURLRequest(curl);

  if (action == CURL_POLL_REMOVE) {
    url_request->RemoveSocketEvent();
  } else {
    if (url_request) {
      url_request->SetSocketEvent(s, action);
    } else {
      fprintf(stderr, "%s: fail to found url request\n", __FUNCTION__);
    }
  }

  return 0;
}
void URLRequestReactor::CheckRequestStatus()
{
  CURLMsg *message;
  int pending;

  while((message = curl_multi_info_read(curlm_, &pending))) {
    switch(message->msg) {
      case CURLMSG_DONE:
        {
          URLRequest* url_request = GetURLRequest(message->easy_handle);
          url_request->OnComplete();
          RemoveRequest(url_request);
        }
        break;
      default:
        fprintf(stderr, "CURLMSG default\n");
        break;
    }
  }
}

}  // namespace url_api

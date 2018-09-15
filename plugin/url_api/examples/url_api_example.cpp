#include <iostream>
#include "url_request.h"

using namespace std;
using namespace url_api;

void on_result(URLRequest* request, int errcode)
{
  if (errcode == ERR_SUCCESS)
  {
    const string& result = request->GetResult();
    cout << "Success" << endl;
    cout << result << endl;
  }
  else
  {
    cout << "Error: " << request->GetLastError() << endl;
  }
}

int main()
{
    URLRequestReactor url_reactor;
    url_reactor.Init();

    HTTPRequest* http_request_1 = url_reactor.NewHTTPRequest("www.baidu.com", on_result);
    http_request_1->EnableVerbose();

    HTTPRequest* http_request_2 = url_reactor.NewHTTPRequest("www.cnbeta.com", on_result);
    http_request_2->EnableVerbose();

    HTTPRequest* http_post = url_reactor.NewHTTPRequest("www.cnbeta.com", on_result);
    http_post->SetPostFields("aa=11&bb=22");
    KVMap header_map;
    header_map.insert(std::make_pair("Myheader", "Myheader Value"));
    http_post->SetHeaderFields(header_map);
    http_post->EnableVerbose();

    SMTPRequest* smtp_request = url_reactor.NewSMTPRequest("smtp://smtp.163.com", on_result);
    smtp_request->SetLogin("you_test_examples_123@163.com", "test_123");
    smtp_request->SetMail("you_test_examples_123@163.com", "32111580@qq.com", "test by url_api", "test, mail content");
    smtp_request->EnableSSL();
    smtp_request->EnableVerbose();

    EV_Singleton->StartLoop();
}

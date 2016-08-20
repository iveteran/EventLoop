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

    SMTPRequest* smtp_request = url_reactor.NewSMTPRequest("smtp://smtp.163.com", on_result);
    smtp_request->SetLogin("j_p_ing@163.com", "ruishi123");
    smtp_request->SetMail("j_p_ing@163.com", "yufb116689@hanslaser.com", "test by url_api", "test, mail content");
    smtp_request->EnableSSL();
    smtp_request->EnableVerbose();

    EV_Singleton->StartLoop();
}

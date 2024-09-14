#include "signal_handler.h"

namespace evt_loop
{

SignalManager *SignalManager::instance_ = NULL;

void HandleSignal(int signo) {
  std::set<SignalEvent *> events = SignalManager::Instance()->sig_events_[signo];
  for (std::set<SignalEvent *>::iterator iter = events.begin(); iter != events.end(); ++iter) {
    (*iter)->OnEvents(signo);
  }
}

int SignalManager::AddEvent(SignalEvent *e) {
  auto& sig_hdlr_set = sig_events_[e->Signal()];
  if (sig_hdlr_set.empty()) {
    struct sigaction action;
    action.sa_handler = HandleSignal;
    action.sa_flags = SA_RESTART;
    sigemptyset(&action.sa_mask);
    sigaction(e->Signal(), &action, NULL);
  }
  sig_hdlr_set.insert(e);

  return 0;
}

int SignalManager::DeleteEvent(SignalEvent *e) {
  auto& sig_hdlr_set = sig_events_[e->Signal()];
  sig_hdlr_set.erase(e);
  if (sig_hdlr_set.empty()) {
    sigaction(e->Signal(), NULL, NULL);
  }
  return 0;
}

int SignalManager::UpdateEvent(SignalEvent *e) {
  /*
  // FIXME: below has no effect
  sig_events_[e->Signal()].erase(e);
  sig_events_[e->Signal()].insert(e);
  */
  return 0;
}

SignalHandler::SignalHandler(SIGNO signo, const OnSignalCallback& cb) :
    SignalEvent(signo), signal_cb_(cb) {
  SignalManager::Instance()->AddEvent(this);
}
SignalHandler::~SignalHandler() {
  SignalManager::Instance()->DeleteEvent(this);
}

}  // namespace evt_loop

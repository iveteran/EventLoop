#ifndef _FLOW_STATE_H
#define _FLOW_STATE_H

namespace evt_loop {

enum class State : uint8_t {
    Ready,
    Running,
    Suspended,
    Done,
    Exception,
    Goto,
    Break,
};

}  // ns evt_loop

#endif // _FLOW_STATE_H

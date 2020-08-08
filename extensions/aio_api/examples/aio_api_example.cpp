#include <iostream>
#include "aio_wrapper.h"
#include "el.h"

using namespace evt_loop;

int main()
{
    bool success;

    auto rd_done_cb = [&](int status, const string& data) {
        printf(">>> read done, status: %d, size: %ld\n", status, data.size());

        bool success = AIO.write("test_data/words_copy.dat", O_RDWR | O_CREAT, data, [&](int status) {
                printf(">>> write done, status: %d\n", status);
                });
        if (!success) {
            printf("error: write failed\n");
        }
    };

    success = AIO.read("test_data/words.dat", O_RDONLY, rd_done_cb);
    if (!success) {
        printf("error: read failed\n");
    }

    std::string wt_data("xxxxx-yyyyyyy");
    success = AIO.write("test_data/wt_data.dat", O_RDWR | O_CREAT, wt_data, [&](int status) {
            printf(">>> write done, status: %d\n", status);
            });
    if (!success) {
        printf("error: write_request failed\n");
    }

#if 1
    SignalHandler sh(SignalEvent::INT, [&](SignalHandler* sh, uint32_t signo) {
            printf("Shutdown!\n"); EV_Singleton->StopLoop();
            });
    EV_Singleton->StartLoop();
#else
    AIO.test();
#endif

    return 0;
}

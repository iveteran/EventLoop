#include <iostream>
#include "eventloop/logger.h"

using namespace evt_loop;

int main()
{
    Logger logger(std::cout);
    //bool colorful = true;
    //Logger logger("logfile.txt", colorful);

    // Example usage of the logger
    string mystr = "stdout: Program started";
    logger.log(LogLevel::WARN, mystr);
    logger.log(LogLevel::ERROR, "stdout: occured error");
    logger.log(LogLevel::DEBUG, "stdout: Debugging information #2 -> {}", 12.12);      // Got: Debugging information #2 -> 12.12

    logger.switch_to_file("logfile_2.txt");

    logger.log(LogLevel::INFO, "logfile2: Program \"{}, {}\" started.", 0b11, "hello");  // Got: Program "3, hello" started
    logger.log(LogLevel::DEBUG, "logfile2: {{}} {\\} Debugging { } information {}.", 12, "world");  // Got: {12} {} Debugging { } information world.

    logger.switch_to_stdout();
    logger.log(LogLevel::CRITICAL, "stdout: Critical error.");
    logger.log(LogLevel::WARN, "stdout: Program end");

    el_logger->debug("default logger: test");
    el_logger->switch_to_file("logfile_3.txt");
    el_logger->warn("logfile3: default logger: test");

    return 0;
}

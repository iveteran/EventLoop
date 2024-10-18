#ifndef _FIND_PRIME_NUMBERS
#define _FIND_PRIME_NUMBERS

#include <vector>

struct PrimeParams {
    std::pair<int, int> range;
};

struct PrimeResult {
    std::vector<int> prime_numbers;
};

void* find_prime_numbers(void* args);
void handle_prime_result(void* result);

#endif  // _FIND_PRIME_NUMBERS

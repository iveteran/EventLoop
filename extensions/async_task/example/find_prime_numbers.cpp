#include <stdio.h>
#include "find_prime_numbers.h"

// 判断是否为素数
bool isprime(int x) {
	if (x < 2) return false;//不是素数

	for (int i = 2; i < x; i++) {
		if (x % i == 0) { //能被其他数整除
			return false;
        }
    }
	return true;
}

void* find_prime_numbers(void* args) {
    auto params = (PrimeParams*)args;
    auto result = new PrimeResult;
    int begin = params->range.first;
    int end = params->range.second;
    printf("[find_prime_numbers] range: [%d, %d]\n", begin, end);
    int total = 0;
    for (int x=begin; x<=end; x++) {
        bool is_prime = isprime(x);
        //printf("[find_prime_numbers] %d: %s\n", x, (is_prime ? "yes" : "no"));
        if (is_prime) {
            //printf("[find_prime_numbers] %d\n", x);
            total++;
            result->prime_numbers.push_back(x);
        }
        if (x % 10000 == 0) {
            printf("[find_prime_numbers] total prime between %d and %d: %d\n", begin, x, total);
        }
    }
    printf("total: %d\n", total);
	return result;
}

void handle_prime_result(void* result)
{
    auto primesResult = (PrimeResult*)result;
    auto total = primesResult->prime_numbers.size();
    printf("[handle_prime_result] found prime total: %ld\n", total);
    delete primesResult;
}

#ifdef UNITTEST
int main() {
    PrimeParams params;
    params.range.first = 100;
    params.range.second = 100000;

    void* result = find_prime_numbers((void*)&params);
    handle_prime_result(result);

    return 0;
}
#endif // UNITTEST

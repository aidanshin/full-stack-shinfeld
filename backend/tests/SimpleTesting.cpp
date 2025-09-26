#include <iostream>
#include <ctime>
#include <thread>

int main() {

    time_t t = time(NULL);

    std::this_thread::sleep_for (std::chrono::seconds(5));

    time_t now = time(NULL);

    time_t temp = 10;
    std::cout << (std::difftime(now, t) > temp) << std::endl;

    return 0;
}

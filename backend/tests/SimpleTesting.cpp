#include <iostream>
// #include <ctime>
// #include <thread>
#include <random>

int main(int argc, const char* argv[]) {

    double probability = std::stod(argv[1]);

    std::random_device rd;
    std::mt19937 gen(rd());

    std::bernoulli_distribution dist(probability);

    for (int i = 0; i < 10; i++) {
        std::cout << dist(gen) << std::endl;
    }

    return 0;
}

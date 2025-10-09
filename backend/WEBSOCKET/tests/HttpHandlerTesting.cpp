#include "HttpHandler.hpp"
#include "Logger.hpp"
#include <fstream>

int main() {

    Logger::setPriority(LogLevel::TRACE);
    Logger::enableFileOutput("logs/HttpHandlerTesting.log");

    std::ifstream ifs("tests/sample_request.txt", std::ifstream::in);

    std::string buffer;
    if(ifs) {
        std::string buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        std::cout << buffer << std::endl;

        HttpHandler handler;
        if(!handler.validateRequest(buffer)) {ifs.close(); return 0;}
        std::cout << "\nValid Request\n" << std::endl;
        std::string response = handler.generateResponse();
        std::cout << response << std::endl;
    }

    ifs.close();

    return 0;
}
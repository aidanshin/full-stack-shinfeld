#include "Client.hpp"
#include "Logger.hpp"

int main() {

    Logger::setPriority(LogLevel::TRACE);
    Logger::enableFileOutput();

    uint16_t temp16 = 0;
    uint32_t temp32 = 0;
    uint8_t temp8 = 0;
    std::vector<uint8_t> data{};

    Client client(temp16, temp32, temp32, temp32, temp32, temp8);

    client.openFile();
    
    client.closeFile();

    std::string temp = "HELLO I AM AIDAN SHINFELD.";

    data.insert(data.begin(),temp.begin(), temp.end());

    client.writeFile(data);

    client.closeFile();

    return 0;
}

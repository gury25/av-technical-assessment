#include <iostream>
#include <fstream>
#include <string>

// Question 1: This is an extension task that requires you to decode sensor data from a CAN log file.
// CAN (Controller Area Network) is a communication standard used in automotive applications (including Redback cars)
// to allow communication between sensors and controllers.
//
// Your Task: Using the definition in the Sensors.dbc file, extract the "WheelSpeedRR" values
// from the candump.log file. Parse these values correctly and store them in an output.txt file with the following format:
// (<UNIX_TIME>): <DECODED_VALUE>
// eg:
// (1705638753.913408): 1234.5
// (1705638754.915609): 6789.0
// ...
// The above values are not real numbers; they are only there to show the expected data output format.
// You do not need to use any external libraries. Use the resources below to understand how to extract sensor data.
// Hint: Think about manual bit masking and shifting, data types required,
// what formats are used to represent values, etc.
// Resources:
// https://www.csselectronics.com/pages/can-bus-simple-intro-tutorial
// https://www.csselectronics.com/pages/can-dbc-file-database-intro

#include <sstream>
#include <iomanip>
#include <cstdint>

int main() {
    std::ifstream logFile("candump.log");
    std::ofstream outputFile("output.txt");
    
    if (!logFile.is_open()) {
        std::cerr << "Error opening candump.log" << std::endl;
        return 1;
    }
    
    if (!outputFile.is_open()) {
        std::cerr << "Error opening output.txt for writing" << std::endl;
        return 1;
    }
    
    std::string line;
    while (std::getline(logFile, line)) {

        size_t openParen = line.find('(');
        size_t closeParen = line.find(')');
        if (openParen == std::string::npos || closeParen == std::string::npos) {
            continue;
        }
        std::string timestamp = line.substr(openParen + 1, closeParen - openParen - 1);

        size_t hashPos = line.find('#');
        if (hashPos == std::string::npos) {
            continue;
        }
        
        size_t msgIdStart = line.rfind(' ', hashPos);
        if (msgIdStart == std::string::npos) {
            continue;
        }
        msgIdStart++;
        
        std::string msgIdStr = line.substr(msgIdStart, hashPos - msgIdStart);
        uint16_t msgId = std::stoi(msgIdStr, nullptr, 16);

        if (msgId != 0x705) {
            continue;
        }

        std::string hexData = line.substr(hashPos + 1);

        if (hexData.length() < 16) {  
            continue;
        }
        
        uint8_t bytes[8];
        for (int i = 0; i < 8; i++) {
            bytes[i] = std::stoi(hexData.substr(i * 2, 2), nullptr, 16);
        }

        int16_t rawValue = (int16_t)(bytes[4] | (bytes[5] << 8));

        double wheelSpeedRR = rawValue * 0.1;
        
        outputFile << "(" << timestamp << "): " << wheelSpeedRR << std::endl;
    }
    
    logFile.close();
    outputFile.close();
    
    std::cout << "Decoding complete. Results written to output.txt" << std::endl;
    return 0;
}


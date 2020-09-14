/*
 * main.cpp
 *
 *  Created on: 17 sie 2020
 *      Author: Jakub Poœpiech
 * 
 * Simple presentation how app works.
 * Press escape to stop demo.
 */
#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <string>
#include <windows.h>

#include "Serial.h"
#include "MovingAverageFilter.h"
#include "MedianFilter.h"

int main() {
    // data transfer interval in my Arduino board is set to 500ms that is why main thread
    // wakes up every half a second.
    std::chrono::milliseconds sleepTime(500);
    int bufferSize = 10;
    //std::shared_ptr<Serial> serialReader = std::make_shared<Serial>("COM3", bufferSize);
    bool exit = false;
    std::vector<std::pair<SerialPortDataAnalyzer*, std::ofstream>> analyzerVector;

    std::ofstream rawDataFile("RawData.txt");


    analyzerVector.push_back(std::pair<SerialPortDataAnalyzer*, std::ofstream>(new MedianFilter("COM3", bufferSize, 2), "MedianFilter.txt"));
    analyzerVector.push_back(std::pair<SerialPortDataAnalyzer*, std::ofstream>(new MovingAverageFilter(analyzerVector[0].first->getSerialPortReader(), 2), "MovingAverageFilter.txt"));
    std::pair<std::time_t, std::double_t> resultPair;

    while (!exit) {
        // All analyzers should provide the same raw data, so I can choose anyone to get it.

        if (!analyzerVector.empty()) {
            SerialPortDataAnalyzer* chosenAnalyzer = analyzerVector[0].first;
            resultPair = chosenAnalyzer->getRawData();
            if (resultPair.first == -1) {
                std::cout << "Raw data from analyzer not available" << std::endl;
            }
            else {
                // probably most simple way to present time in readable form, altough definitely not the best,
                // but for presentation purpose is enough
                std::string dataDate = std::asctime(std::localtime(&resultPair.first));
                if (dataDate.length() >= 2)
                    dataDate.back() = '\0';

                rawDataFile << dataDate << " , " << resultPair.second << std::endl;
            }
        }

        for (std::pair<SerialPortDataAnalyzer*, std::ofstream>& analyzerPair : analyzerVector) {
            SerialPortDataAnalyzer* selectedAnalyzer = analyzerPair.first;
            resultPair = selectedAnalyzer->getProcessedData();
            if (resultPair.first == -1) {
                std::cout << "Processed data from analyzer not available" << std::endl;
            }
            else {
                std::string dataDate = std::asctime(std::localtime(&resultPair.first));
                if (dataDate.length() >= 2)
                    dataDate.back() = '\0';

                analyzerPair.second  << dataDate << " , " << resultPair.second << std::endl;
            }
        }
        

        if (GetAsyncKeyState(VK_ESCAPE))
        {
            exit = true;
        }
        std::this_thread::sleep_for(sleepTime);
    }

    for (std::pair<SerialPortDataAnalyzer*, std::ofstream>& analyzerPair : analyzerVector) {
        delete analyzerPair.first;
        analyzerPair.second.close();
    }
    analyzerVector.clear();
    rawDataFile.close();

    system("pause");
    return 0;
}



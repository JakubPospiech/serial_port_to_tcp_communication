/*
 * MedianFilter.cpp
 *
 *  Created on: 19 sie 2020
 *      Author: Jakub Poœpiech
 */

#include <iostream>
#include <set>
#include "MedianFilter.h"

MedianFilter::MedianFilter(const std::shared_ptr<Serial>& serialReader, unsigned int filterWindow)
    :SerialPortDataAnalyzer(serialReader)
    ,rawValueLegit(false)
    ,processedValueLegit(false)
    ,filterWindowWidth(2*filterWindow + 1) {
    if (this->registerToSerialReader(this) == false) {
        std::cout << "Error: registering to serial reader failed." << std::endl;
    }
}

MedianFilter::MedianFilter(const std::string& serialName, unsigned int bufferSize, unsigned int filterWindow)
    :SerialPortDataAnalyzer(serialName, bufferSize)
    ,rawValueLegit(false)
    ,processedValueLegit(false)
    ,filterWindowWidth(2*filterWindow + 1) {
    if (this->registerToSerialReader(this) == false) {
        std::cout << "Error: registering to serial reader failed." << std::endl;
    }
}

MedianFilter::~MedianFilter() {
    this->deregisterFromSerialReader(this);
}

std::pair<std::time_t, double> MedianFilter::getRawData() {
    std::scoped_lock dataLock(this->dataMutex);

    return this->rawValueLegit ? this->currentRawValue : std::pair<std::time_t, double>{ -1,0 };
}

std::pair<std::time_t, double> MedianFilter::getProcessedData() {
    std::scoped_lock dataLock(this->dataMutex);

    return this->processedValueLegit ? this->currentProcessedValue : std::pair<std::time_t, double>{ -1,0 };
}

void MedianFilter::fetchNewData(const std::pair<std::time_t, std::string>& data) {

    if (data.second == "ERROR" || data.second == "CLOSED" || data.second == "INITIALIZING") {
        // When no numeric value is provided all the values stop being legitimate and filter window is cleared.
        this->rawValueLegit = false;
        this->processedValueLegit = false;

        std::cout << "No data received - serial port reader is in " << data.second << " state." << std::endl;

        std::scoped_lock dataLock(this->dataMutex);
        this->valuesInFilterWindow.clear();
        this->currentRawValue = std::pair<std::time_t, double>{ -1,0 };
        this->currentProcessedValue = std::pair<std::time_t, double>{ -1,0 };

    }
    else {
        try {
            double newValue = std::stod(data.second); // If there is anything wrong with data, exception will be thrown there

            std::scoped_lock dataLock(this->dataMutex);
            this->currentRawValue.first = data.first;
            this->currentRawValue.second = newValue;

            if (this->rawValueLegit == false) {
                this->rawValueLegit = true;
            }

            if (this->valuesInFilterWindow.size() == this->filterWindowWidth) {
                this->valuesInFilterWindow.emplace(this->valuesInFilterWindow.begin(), data.first, newValue);
                this->valuesInFilterWindow.pop_back();
                this->processData();
            }
            else {
                this->valuesInFilterWindow.emplace(this->valuesInFilterWindow.begin(), data.first, newValue);
                if (this->valuesInFilterWindow.size() == this->filterWindowWidth) {
                    this->processData();
                    this->processedValueLegit = true;
                }
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error during processing data from serial port - wrong value format or value out of range" << std::endl;
        }
    }
}

void MedianFilter::processData() {
     std::set<double> sortedValues;

     // That method of sorting values may be inefficient but is very quick to implement.
     // For relatively small filtering windows performance penalty should be negligible.
     for(std::pair<std::time_t, double> vectorValue : this->valuesInFilterWindow) {
         sortedValues.insert(vectorValue.second);
     }

     double medianValue = *std::next(sortedValues.begin(), (this->filterWindowWidth) / 2); // middle value is median
     // Median filter cannot filter latest received value, it will always have little delay.
     this->currentProcessedValue.first = this->valuesInFilterWindow[(this->filterWindowWidth)/2].first;
     this->currentProcessedValue.second = medianValue;
 }

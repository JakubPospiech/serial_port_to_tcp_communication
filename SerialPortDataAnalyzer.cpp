/*
 * SerialPortDataAnalyzer.cpp
 *
 *  Created on: 18 sie 2020
 *      Author: Jakub Poœpiech
 */

#include "SerialPortDataAnalyzer.h"

SerialPortDataAnalyzer::SerialPortDataAnalyzer(const std::shared_ptr<Serial>& serialReader) {
    this->serialPortReader = serialReader;
}

SerialPortDataAnalyzer::SerialPortDataAnalyzer(const std::string& serialName, unsigned int bufferSize) {
    this->serialPortReader = std::make_shared<Serial>(serialName, bufferSize);
}

SerialPortDataAnalyzer::~SerialPortDataAnalyzer() {
    // For now base destructor does not need to do any specific cleanup.
}

std::shared_ptr<Serial> SerialPortDataAnalyzer::getSerialPortReader() {
    return this->serialPortReader;
}

bool SerialPortDataAnalyzer::registerToSerialReader(SerialPortDataAnalyzer* analyzer) {
    return this->serialPortReader->registerDataAnalyzer(analyzer);
}

void SerialPortDataAnalyzer::deregisterFromSerialReader(SerialPortDataAnalyzer* analyzer) {
    this->serialPortReader->deregisterDataAnalyzer(analyzer);
}


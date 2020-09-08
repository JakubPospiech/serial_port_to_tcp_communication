/*
 * Serial.h
 *
 *  Based on existing example of serial port communication from:
 * https://playground.arduino.cc/Interfacing/CPPWindows/
 *
 * Object of that class reads data from serial port and forwards it to registered data analyzers.
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include <string>
#include <windows.h>
#include <stdlib.h>
#include <mutex>
#include <thread>
#include <vector>
#include <ctime>
#include <memory>
#include <atomic>
#include <algorithm>
#include <condition_variable>

class SerialPortDataAnalyzer;

class Serial
{
private:
    // Serial comm handler
    HANDLE hSerial;

    // Connection status
    bool connected;

    // Information about whether reader is active or not.
    std::atomic<bool> readerActive;

    // Get various information about the connection
    COMSTAT status;

    // Keep track of last error
    DWORD errors;

    // Hold information about size of the buffer
    unsigned int nbOfCharsRead;

    // Buffer for reading
    char* charBuffer;

    // Vector for storing pointers to registered analyzers
    std::vector<SerialPortDataAnalyzer*> registeredAnalyzers;

    // Last read value with timestamp, value updated only through Serial::doReading().
    // Second parameter besides raw values can be also "ERROR", "CLOSED", or "INITIALIZING",
    // analyzers should handle that properly.
    std::pair<std::time_t, std::string> lastReading;

    // Mutex for data receiving and reading data from serial port.
    std::mutex dataMutex;

    // Mutex for sending new data to analyzers
    std::mutex registeredAnalyzersMutex;

    // Variable used to notify about some events
    std::condition_variable readerNotifier;

    // Ptr to thread that is reading values from serial port
    std::unique_ptr<std::thread> readingThreadPtr;

    // Ptr to thread that is sending new reading to analyzers
    std::unique_ptr<std::thread> sendThreadPtr;

    friend class SerialPortDataAnalyzer;
 
public:
    // Initialize Serial communication with the given COM port using set buffer size.
    // Note that buffer size MUST be identical to size of messages that are sent through
    // serial port. Otherwise values will be desynced.
    Serial(const std::string& portDesc, unsigned int bufferSize);

    // Close the connection
    ~Serial();

    // Get last read data. Check lastReading description to know possible data values.
    std::pair<std::time_t, std::string> getData();

    // Check if we are actually connected
    bool IsConnected();
private:
    // Registers data analyzer.
    // Returns - true on success, false otherwise
    bool registerDataAnalyzer(SerialPortDataAnalyzer* analyzerToRegister);

    // Deregisters data analyzer.
    void deregisterDataAnalyzer(SerialPortDataAnalyzer* analyzerToDeregister);

    // Constantly sends new data to registered analyzers.
    // Check lastReading description to know possible data values.
    void sendDataToAnalyzers();

    // Does constant reading of values sent through serial port.
    void doReading();

};

#endif /* SERIAL_H_ */

#include "Serial.h"
#include "SerialPortDataAnalyzer.h"

#include <iostream>

Serial::Serial(const std::string& portDesc, unsigned int bufferSize) {
    //We're not yet connected
    this->connected = false;

    //No errors yet.
    this->errors = 0;

    this->nbOfCharsRead = bufferSize;

    this->charBuffer = new char[bufferSize];

    this->lastReading = std::pair<std::time_t, std::string>{ -1, "INITIALIZING" };

    //Try to connect to the given port through CreateFile
    this->hSerial = CreateFile(portDesc.c_str(),
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    //Check if the connection was successfull
    if(this->hSerial==INVALID_HANDLE_VALUE) {
        //If not success full display an Error
        if(GetLastError()==ERROR_FILE_NOT_FOUND) {

            //Print Error if neccessary
            std::cout << "ERROR: Handle was not attached. Reason: "<< portDesc.c_str() << " not available." << std::endl;

        }
        else {
            std::cout << "ERROR!!!" << std::endl;
        }
    }
    else {
        //If connected we try to set the comm parameters
        DCB dcbSerialParams = {0};

        //Try to get the current
        if (!GetCommState(this->hSerial, &dcbSerialParams)) {
            //If impossible, show an error
            std::cout <<  "failed to get current serial parameters!" << std::endl;
        }
        else {
            //Define serial connection parameters for the arduino board
            dcbSerialParams.BaudRate=CBR_9600;
            dcbSerialParams.ByteSize=8;
            dcbSerialParams.StopBits=ONESTOPBIT;
            dcbSerialParams.Parity=NOPARITY;
            //Setting the DTR to Control_Enable ensures that the Arduino is properly
            //reset upon establishing a connection
            dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

             //Set the parameters and check for their proper application
             if(!SetCommState(hSerial, &dcbSerialParams)) {
                 std::cout << "ALERT: Could not set Serial Port parameters" << std::endl;
             }
             else {
                 //If everything went fine we're connected and active
                 this->connected = true;
                 this->readerActive = true;
                 //Flush any remaining characters in the buffers
                 PurgeComm(this->hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

                 this->sendThreadPtr = std::make_unique<std::thread>([this] {this->doReading(); });
                 this->readingThreadPtr = std::make_unique<std::thread>([this] {this->sendDataToAnalyzers(); }); 
                 std::cout << "Serial reader created succesfully" << std::endl;
             }
        }
    }

}

Serial::~Serial() {

    // Destructor called - reader is not active anymore.
    this->readerActive = false;

    {
        std::scoped_lock analyzerLock(this->registeredAnalyzersMutex);      

        // In theory that vector should be empty already (registered analyzers also keep
        // shared_ptr to serial object so they will keep them alive as long as they want).
        if (this->registeredAnalyzers.empty() == false) {
            for (SerialPortDataAnalyzer* analyzerPointer : this->registeredAnalyzers) {
                analyzerPointer->fetchNewData(std::pair<std::time_t, std::string>{ -1, "CLOSED" });
            }
        }
    }

    readerNotifier.notify_all();

    readingThreadPtr->join();
    sendThreadPtr->join();
    
    //Check if we are connected before trying to disconnect
    if(this->connected) {
        //We're no longer connected
        this->connected = false;
        //Close the serial handler
        CloseHandle(this->hSerial);
    }

    delete[] this->charBuffer;

    std::cout << "Serial reader deleted" << std::endl;
}

std::pair<std::time_t, std::string> Serial::getData() {

    std::scoped_lock dataLock(this->dataMutex);

    return this->lastReading;
 }

bool Serial::registerDataAnalyzer(SerialPortDataAnalyzer* analyzerToRegister) {

    if ((analyzerToRegister != nullptr) && (this->readerActive == true)) {
        bool analyzerAlreadyRegistered = false;

        std::scoped_lock analyzerLock(this->registeredAnalyzersMutex);

        for (SerialPortDataAnalyzer* analyzerPtr : this->registeredAnalyzers) {
            if (analyzerPtr == analyzerToRegister) {
                analyzerAlreadyRegistered = true;
            }
        }

        if (analyzerAlreadyRegistered) {
            return false;
        }
        else {
            this->registeredAnalyzers.push_back(analyzerToRegister);
            return true;
        }
    }
    else
        return false;
}

void Serial::deregisterDataAnalyzer(SerialPortDataAnalyzer* analyzerToDeregister) {
    std::scoped_lock analyzerLock(this->registeredAnalyzersMutex);

    this->registeredAnalyzers.erase(std::remove(this->registeredAnalyzers.begin(), this->registeredAnalyzers.end(),
                                                analyzerToDeregister),
                                    this->registeredAnalyzers.end());
}

void Serial::sendDataToAnalyzers() {
    std::unique_lock<std::mutex> dataLock(this->dataMutex, std::defer_lock);
    std::unique_lock<std::mutex> analyzerLock(this->registeredAnalyzersMutex);

    while (this->readerActive) {
        // Wait for new data, if reader is active and there is no registered analyzers wait again.
        this->readerNotifier.wait(analyzerLock);

        if (!this->registeredAnalyzers.empty()) {
            // If there are analyzers registered send them new data.
            dataLock.lock();
            for (SerialPortDataAnalyzer* analyzerPtr : this->registeredAnalyzers) {
                analyzerPtr->fetchNewData(this->lastReading);
            }
            dataLock.unlock();
        }
    }
}

void Serial::doReading() {
    //Number of bytes we'll have read
    DWORD bytesRead;

    std::unique_lock<std::mutex> dataLock(this->dataMutex, std::defer_lock);

    while (this->readerActive) {
        //Use the ClearCommError function to get status info on the Serial port
        ClearCommError(this->hSerial, &this->errors, &this->status);


        //Try to read the required number of chars, and return effect
        if (ReadFile(this->hSerial, this->charBuffer, this->nbOfCharsRead, &bytesRead, NULL))
        {
            

            if (bytesRead == this->nbOfCharsRead) {
                dataLock.lock();
                this->lastReading = std::pair<std::time_t, std::string>(std::time(nullptr), std::string(this->charBuffer));
                dataLock.unlock();
            }
            else {
                // Error during read will result in reader becoming inactive, otherwise thread would most likely
                // loop without any block flooding everything with error results.
                this->readerActive = false;

                dataLock.lock();
                this->lastReading = std::pair<std::time_t, std::string>{ -1, "ERROR" };
                dataLock.unlock();
            }
                
            
        }
        else {
            // Error during read will result in reader becoming inactive, otherwise thread would most likely
            // loop without any block flooding everything with error results.
            this->readerActive = false;

            dataLock.lock();            
            // If nothing has been read, or that an error was detected return error pair
            this->lastReading = std::pair<std::time_t, std::string>{ -1, "ERROR" };
            dataLock.unlock();
        }
        readerNotifier.notify_all();
    }

}

bool Serial::IsConnected()
{
    // Simply return whether reader is active (there is no way
    // to recover from inactivity right now, so from the outside
    // being active and being connected means the same).
    return this->readerActive;
}




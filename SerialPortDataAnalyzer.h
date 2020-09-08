/*
 * SerialPortDataAnalyzer.h
 *
 *  Created on: 18 sie 2020
 *      Author: Jakub Poœpiech
 * 
 * Base class for data analyzers, all created analyzers should inherit from that class.
 */
#ifndef SERIALPORTDATAANALYZER_H_
#define SERIALPORTDATAANALYZER_H_

#include <ctime>
#include <memory>
#include <string>
#include <utility>

#include "Serial.h"

class SerialPortDataAnalyzer {
public:
    // Ctors
    // Initializes object and registers to provided serial port reader.
    SerialPortDataAnalyzer(const std::shared_ptr<Serial>& serialReader);
    // Initializes object, creates and registers to serial port reader from provided serial port name.
    SerialPortDataAnalyzer(const std::string& serialName, unsigned int bufferSize);

    virtual ~SerialPortDataAnalyzer();

    // Returns used serial port reader
    std::shared_ptr<Serial> getSerialPortReader();

    /* Pure virtual methods */
    /**
    * Get latest read with timestamp.
    * returns: Timestamp with value on success, (-1,0) when error occurs or no sufficient data
    *           was yet received.
    */
    virtual std::pair<std::time_t, double> getRawData() = 0;

    /**
    * Get latest possible processed read (in case of that example app - filtered read) with timestamp.
    * returns: Timestamp with value on success, (-1,0) when error occurs or no sufficient data
    *           was yet received.
    */
    virtual std::pair<std::time_t, double> getProcessedData() = 0;


protected:
    // Pointer to Serial object responsible for reading data from serial port.
    std::shared_ptr<Serial> serialPortReader;

    /**
     * Registering data analyzer to serial object. Registration allows Serial
     * object to sent new data to data analyzer through sendNewData() method.
     *
     * NOTE: It is advised to register data analyzers during construction
     * of objects and deregister them during its destruction.
     *
     * param: analyzer - pointer to data analyzer.
     */
    bool registerToSerialReader(SerialPortDataAnalyzer* analyzer);

    /**
     * Deregistering data analyzer from serial object.
     * ALL registered data analyzers must be deregistered prior to its destruction!
     *
     * NOTE: It is advised to register data analyzer in object's ctor
     * and deregister them in its dtor.
     *
     * param: analyzer - pointer to data analyzer.
     */
    void deregisterFromSerialReader(SerialPortDataAnalyzer* analyzer);

private:
    friend class Serial;

    /**
     * Method used by Serial class object threads to send latest data to analyzer.
     * Every class should implement way to process that data.
     *
     * param: data - freshly received data from serial port reader.
     */
    virtual void fetchNewData(const std::pair<std::time_t, std::string>& data) = 0;
};

#endif /* SERIALPORTDATAANALYZER_H_ */

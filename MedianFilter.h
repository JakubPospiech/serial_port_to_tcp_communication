/*
 * MedianFilter.h
 *
 *  Created on: 19 sie 2020
 *      Author: Jakub Poœpiech
 * 
 *  MedianFilter applies median filter to received data.
 */

#ifndef MEDIANFILTER_H_
#define MEDIANFILTER_H_

#include <vector>
#include <mutex>
#include <atomic>
#include "SerialPortDataAnalyzer.h"

class MedianFilter: public SerialPortDataAnalyzer {
public:

    /**
     * Creates MedianFilter with serialReader provided.
     *
     * params:
     * serialReader - serial reader object
     * filterWindow - filter window size presented as difference between center and farthest position
     */
    MedianFilter(const std::shared_ptr<Serial>& serialReader, unsigned int filterWindow);

    /**
     * Creates MedianFilter and creates new serial reader.
     *
     * params:
     * serialName - name of serial port serial reader will read from
     * bufferSize - amount of bytes serial reader will try to read
     * filterWindow - filter window size presented as difference between center and farthest position
     */
    MedianFilter(const std::string& serialName, unsigned int bufferSize, unsigned int filterWindow);


    virtual ~MedianFilter();

    /**
     *  Get latest read from serial port with timestamp.
     *  returns: latest raw data with timestamp or (-1,0) when any data have not been received yet,
     *  or error occured.
     */
    virtual std::pair<std::time_t, double> getRawData();

    /**
     *  Get latest processed read from serial port with timestamp.
     *  returns: latest raw data with timestamp or (-1,0) when any data have not been received yet
     *  or error occured.
     */
    virtual std::pair<std::time_t, double> getProcessedData();

private:

    // Latest raw value - this value will always be equal to first value from
    // valuesInFilterWindow (as long as rawValueLegit is true) but is declared
    // separately for clarity.
    std::pair<std::time_t, double> currentRawValue;

    // Latest filtered value.
    std::pair<std::time_t, double> currentProcessedValue;

    // Vector of x latest read values needed to filter data.
    std::vector<std::pair<std::time_t, double>> valuesInFilterWindow;

    // Flags indicating whether results are legitimate already (enough amount of readings was gathered)
    std::atomic<bool> rawValueLegit;
    std::atomic<bool> processedValueLegit;

    // Value storing aimed size of filter length.
    unsigned int filterWindowWidth;

    // Mutex to synchronise access to data (fetchNewData is called from different threads)
    std::mutex dataMutex;

    /**
     * Method used by Serial object to send latest data to analyzer.
     *
     * param: data - freshly received data from serial port reader.
     */
    virtual void fetchNewData(const std::pair<std::time_t, std::string>& data);

    /**
     * That method applies median filtering algorithm to received data.
     * Method is not thread safe, lock mutex before calling.
     */
    void processData();

};

#endif /* MEDIANFILTER_H_ */

#ifndef DONTTOUCHSIGNALS_H_
#define DONTTOUCHSIGNALS_H_

#include <array>
#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>


class DontTouchSignals
{
public:
    DontTouchSignals();
    virtual ~DontTouchSignals();

    // Prevent copying (alternatively, implement copy/asgn duplicating Content_)
    DontTouchSignals &operator=(const DontTouchSignals &) = delete;      // assignment operator
    DontTouchSignals(const DontTouchSignals &DontTouchSignals) = delete; // copy constructor

    int Get(const std::string fileName);
    

    int AddDontTouchSignal(std::string signal_name);
    int RemoveDontTouchSignal(std::string signal_name);

    bool isDontTouch(std::string signalName);

private:
    /*fast access for all the dont touch signals*/
    std::unordered_map<std::string, std::string> dontTouchSignals;

    /*still file*/
    typedef struct 
    {
        std::string id;
        int scanLength;
        std::string scanIn;
        std::string scanOut;
        std::string scanEnable;
        std::string scanMasterClock;
    }scanchain_t;
};

#endif /*DONTTOUCHSIGNALS_H_*/
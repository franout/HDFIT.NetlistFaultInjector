#include "common.h"

#include "DontTouchSignals.h"

DontTouchSignals::DontTouchSignals()
{
    // TODO Auto-generated constructor stub
}

DontTouchSignals::~DontTouchSignals()
{
}

int DontTouchSignals::Get(const std::string fileName)
{

    std::ifstream inputFile(fileName);
    std::string line;
    if (inputFile.is_open())
    {

        if (ParseUtils::has_suffix(fileName, ".stil") || ParseUtils::has_suffix(fileName, ".spf"))
        {

            scanchain_t chain;
            bool insideScanChainBlock = false;

            while (std::getline(inputFile, line))
            {
                std::istringstream lineStream(line);
                std::string keyword, value;

                while (lineStream >> keyword)
                {
                    if (keyword == "ScanChain")
                    {
                        if (insideScanChainBlock)
                        {
                            nfiFatal("Failed to parse nested scan chain the scan chain\n");
                            return false;
                        }

                        insideScanChainBlock = true;
                        if (!(lineStream >> chain.id))
                        {
                            nfiFatal("Failed to parse the scan chain\n");
                            return -1; // Failed to parse ScanChain id
                        }
                    }
                    else if (insideScanChainBlock && keyword == "}")
                    {
                        insideScanChainBlock = false;
                    }
                    else if (insideScanChainBlock)
                    {
                        if (keyword == "ScanLength")
                        {
                            if (!(lineStream >> chain.scanLength))
                            {
                                nfiFatal("Failed to parse ScanLength\n");
                                return -1; //
                            }
                            /*discard the value */
                        }
                        else if (keyword == "ScanIn")
                        {
                            if (!(lineStream >> chain.scanIn))
                            {
                                nfiFatal("Failed to parse ScanIn\n");
                                return -1;
                            }
                            // remove ""
                            // Remove double quotes using erase and remove_if
                            chain.scanIn.erase(std::remove_if(chain.scanIn.begin(), chain.scanIn.end(), [](char c)
                                                              { return c == '"' || c == ';'; }),
                                               chain.scanIn.end());
                            // add to dount touch signals
                            nfiDebug("Adding %s as dont touch signal\n", chain.scanIn.c_str());
                            dontTouchSignals.insert({chain.scanIn, chain.scanIn});
                        }
                        else if (keyword == "ScanOut")
                        {
                            if (!(lineStream >> chain.scanOut))
                            {
                                nfiFatal("Failed to parse ScanOut\n");
                                return -1;
                            }
                            // remove ""
                            // Remove double quotes using erase and remove_if
                            chain.scanOut.erase(std::remove_if(chain.scanOut.begin(), chain.scanOut.end(), [](char c)
                                                               { return c == '"' || c == ';'; }),
                                                chain.scanOut.end());
                            // add to dount touch signals
                            nfiDebug("Adding %s as dont touch signal\n", chain.scanOut.c_str());
                            dontTouchSignals.insert({chain.scanOut, chain.scanOut});
                        }
                        else if (keyword == "ScanEnable")
                        {
                            if (!(lineStream >> chain.scanEnable))
                            {
                                nfiFatal("Failed to parse ScanEnable\n");
                                return -1;
                            }
                            // remove ""
                            // Remove double quotes using erase and remove_if
                            chain.scanEnable.erase(std::remove_if(chain.scanEnable.begin(), chain.scanEnable.end(), [](char c)
                                                                  { return c == '"' || c == ';'; }),
                                                   chain.scanEnable.end());
                            // add to dount touch signals
                            nfiDebug("Adding %s as dont touch signal\n", chain.scanEnable.c_str());
                            dontTouchSignals.insert({chain.scanEnable, chain.scanEnable});
                        }
                        else if (keyword == "ScanMasterClock")
                        {
                            if (!(lineStream >> chain.scanMasterClock))
                            {
                                nfiFatal("Failed to parse ScanMasterClock\n");
                                return -1;
                            }
                            // remove ""
                            // Remove double quotes using erase and remove_if
                            chain.scanMasterClock.erase(std::remove_if(chain.scanMasterClock.begin(), chain.scanMasterClock.end(), [](char c)
                                                                       { return c == '"' || c == ';'; }),
                                                        chain.scanMasterClock.end());
                            // add to dount touch signals
                            nfiDebug("Adding %s as dont touch signal\n", chain.scanMasterClock.c_str());
                            dontTouchSignals.insert({chain.scanMasterClock, chain.scanMasterClock});
                        }
                    }
                }
            }
        }
        else
        {
            /*a txt*/
            // signals names are declared line by line
            while (std::getline(inputFile, line))
            {
                /*no control*/
                nfiDebug("Adding donttouch signal %s\n", line.c_str());
                dontTouchSignals.insert({line, line});
            }
        }

        inputFile.close();
    }
    else
    {
        nfiFatal("Cannot open %s\n", fileName.c_str());
        return -1;
    }

    return 0;
}

int DontTouchSignals::AddDontTouchSignal(const std::string signalName)
{

    if (dontTouchSignals.end() != dontTouchSignals.find(signalName))
    {
        dontTouchSignals.insert({signalName, signalName});
    }
    else
    {
        nfiWarning("%s already present in dont touch signals\n", signalName.c_str());
        return -1;
    }
    return 0;
}

int DontTouchSignals::RemoveDontTouchSignal(const std::string signalName)
{

    if (dontTouchSignals.end() != dontTouchSignals.find(signalName))
    {
        dontTouchSignals.erase(signalName);
    }
    else
    {
        nfiWarning("Cannot remove %s, not present in dont touch signals\n", signalName.c_str());
        return -1;
    }
    return 0;
}

bool DontTouchSignals::isDontTouch(std::string signalName)
{

    if (dontTouchSignals.end() != dontTouchSignals.find(signalName))
    {
        return true;
    }
    else
    {
        nfiWarning("%s does not exist in dont touch signals\n", signalName.c_str());
    }
    return false;
}

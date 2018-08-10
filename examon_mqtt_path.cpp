/*
 * examon_mqtt_path.c
 *
 *  Created on: 09.08.2018
 *      Author: jitschin
 */


#include <string>

extern "C"
{
#include <string.h>
}

class examon_mqtt_path
{
private:
	std::string maskPrefix = "/node/";
	std::string maskPostfix = "/plugin/pmu_pub/chnl";
	std::string base = "";
	std::string cmd = "";
	std::string ergUnits = "";
public:
	examon_mqtt_path(char* examonHost, char* configChannel)
	{
	    base = configChannel + maskPrefix + examonHost + maskPostfix;
	    cmd = base + "/cmd";
	    ergUnits = base + "/data/cpu/0/erg_units";
	}
	std::string topicBase() { return base; }
	std::string topicErgUnits() { return ergUnits; }
	std::string topicCmd() { return cmd; }
	std::string getDataTopic(std::string subtopic) { return base + "/data/" + subtopic; }
    bool isErgUnits(char* comparisonTopic)
    {
    	if(NULL != comparisonTopic && 0 == ergUnits.compare(comparisonTopic))
    	{
    		return true;
    	} else
    	{
    		return false;
    	}
    }
    bool startsWithTopicBase(char* comparisonTopic)
    {
    	if(NULL != comparisonTopic && 0 == strncmp(comparisonTopic, base.c_str(), base.length()))
    	{
    		return true;
    	} else
    	{
    		return false;
    	}
    }
};


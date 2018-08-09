/*
 * examon_metric.c
 *
 *  Created on: 09.08.2018
 *      Author: jitschin
 */

#include <string>
#include <mosquittopp.h>

extern "C"
{
#include <libgen.h>
#include <string.h>
}

enum ACCUMULATION_STRATEGY {
	ACCUMULATION_MAX,
	ACCUMULATION_MIN,
	ACCUMULATION_SUM,
	ACCUMULATION_AVG
};
/*
class examon_mqtt_path
{
	examon_mqtt_path(char* examonHost, )
	{

	}
};
*/
class examon_metric
{
private:
	std::int32_t metricId;
	std::string metricName;
	std::string metricFullTopic;
	double metricValue;
	bool metricIsEnergy;

	double metricTimestamp;
	double metricElapsed;
	std::int64_t metricIterations;
	std::int64_t metricTopicCount;
	double metricAccumulated;
	std::int64_t metricSubIterations;
	ACCUMULATION_STRATEGY accStrategy;

public:

	static std::string topicBaseData;
	static double ergUnit;

	static void setErgUnit(double paramErgUnit) { examon_metric::ergUnit = paramErgUnit; }
	static void setTopicBase(std::string topic) { examon_metric::topicBaseData = topic + "/data"; }
	std::string getFullTopic() { return metricFullTopic.c_str(); }
	examon_metric(std::int32_t id, std::string name)
    {
        metricId = id;
        metricName = name;
        metricFullTopic = topicBaseData + "/" + name;
        char* metricBasename = basename((char*) name.c_str());
        metricIsEnergy = false;
        if(0 == strncmp("erg", metricBasename, 3)
        && 0 != strncmp("erg_units", metricBasename, 9))
        {
        	metricIsEnergy = true;
        }
        metricValue = -1.00;
        metricTimestamp = 0.00;
        metricElapsed = 0.00;
        metricIterations = 0;
        metricTopicCount = 0;
        metricAccumulated = 0.00;
        metricSubIterations = 0;
        /* TODO: make it configurable, also have a heuristic to determine this setting if not explicitly specified */
        accStrategy = ACCUMULATION_AVG;
    }
	bool metricMatches(char* incomingTopic)
	{
        bool topicMatches = false;
		mosqpp::topic_matches_sub(metricFullTopic.c_str(), incomingTopic, &topicMatches);
		return topicMatches;
	}
	void handleMessage(char* incomingTopic, char* incomingPayload, int payloadlen)
	{
		double readValue = -1;
		double readTimestamp = -1;
		int valuesRead = sscanf( incomingPayload, "%lf;%lf", &readValue, &readTimestamp);
		if( 2 == valuesRead )
		{
			double delta = 0.00;
			if(readTimestamp != metricTimestamp)
			{
				delta = readTimestamp - metricTimestamp;
				metricElapsed = delta;

				metricValue = readValue; // - put value in metricValues
				++metricIterations;
				metricSubIterations = 1;
			} else
			{
				++metricSubIterations;
				if(1 == metricIterations)
				{
					++metricTopicCount;
				} else
				{
					// TODO: put all this logic in a separate class for the metric
					//         optional: implement fancy pthread_mutex_locking for it


					// accumulate strategy
					// at this point metricTopicCount contains a sensible number of actually
					//   subscribed to metrics

					// here is the first duplicate, i.e. timestamps are equal
					// so the current preceding value was already written to metricValues[i]

					switch(accStrategy)
					{
					case ACCUMULATION_AVG:
						metricValue += readValue;
						if(metricSubIterations == metricTopicCount)
						{
							metricAccumulated = metricValue / metricTopicCount;
						}
						break;
					case ACCUMULATION_SUM:
						metricValue += readValue;
						if(metricSubIterations == metricTopicCount) metricAccumulated = metricValue;
						break;
					case ACCUMULATION_MIN:
						if(metricValue > readValue) metricValue = readValue;
						if(metricSubIterations == metricTopicCount) metricAccumulated = metricValue;
						break;
					case ACCUMULATION_MAX:
						if(metricValue < readValue) metricValue = readValue;
						if(metricSubIterations == metricTopicCount) metricAccumulated = metricValue;
						break;
					}
				}
				metricTimestamp = readTimestamp;
			}



			printf("read metric %9s: %0.4lf, metricValue= %0.4lf\n", metricName.c_str(), readValue, metricValue);
		}
	}
	bool hasValue()
	{
		if(1 < metricIterations)
		{
			if(!metricIsEnergy || 0 < ergUnit)
			{
				return true;
			}
		}
		return false;
	}
	double getLatestValue()
	{
		double returnValue = 0.00;
		if(1 < metricTopicCount)
		{
			returnValue = metricAccumulated;
		} else
		{
			returnValue = metricValue;
		}
		if(metricIsEnergy)
		{
			if(0 < ergUnit)
			{
				//TODO: heed metricElapsed: also divide through metricElapsed
    			returnValue /= ergUnit;
			}
		}
		return returnValue;
	}
};


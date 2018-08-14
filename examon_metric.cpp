/*
 * examon_metric.c
 *
 *  Created on: 09.08.2018
 *      Author: jitschin
 */

#include <vector>
#include <utility>
#include <scorep/chrono/chrono.hpp>

#include <string>
#include <mosquittopp.h>
#include "examon_mqtt_path.cpp"

extern "C"
{
#include <libgen.h>
#include <string.h>
}

#include "include_once.hpp"


class examon_metric
{
private:
	std::int32_t metricId;
	std::string metricName;
	examon_mqtt_path* channels;
	std::string metricFullTopic;
	double metricValue;

	double metricTimestamp;
	double metricElapsed;
	std::int64_t metricIterations;
	std::int64_t metricTopicCount;
	double metricAccumulated;
	std::int64_t metricSubIterations;
	ACCUMULATION_STRATEGY accStrategy;
	EXAMON_METRIC_TYPE metricType;
	double ergUnit;
	bool gatherData;
	std::vector<std::pair<scorep::chrono::ticks, double>> gatheredData;

	//DEBUG
	std::int64_t outputCounter = 0;
public:

	void setErgUnit(double paramErgUnit) { ergUnit = paramErgUnit; }
	std::string getFullTopic() { return metricFullTopic; }
	std::string getName() { return metricName; }
	void setGatherData(bool stillGather) { gatherData = stillGather; }
	examon_metric(std::int32_t id, std::string name, examon_mqtt_path* param_channels, bool param_gather)
    {
        metricId = id;
        /* configurable accumulation strategy, TODO: also have a heuristic to determine this setting if not explicitly specified */
        accStrategy = ACCUMULATION_AVG;
        int semicolonPos = name.find_first_of(';');
        if(std::string::npos != semicolonPos)
        {
        	accStrategy = parseAccumulationStrategy(name.substr(semicolonPos + 1, std::string::npos));
        	name = name.substr(0, semicolonPos);
        }
        metricName = name;
        channels = param_channels;
        metricFullTopic = channels->getDataTopic(name);
        char* metricBasename = basename((char*) name.c_str());
        metricType = parseMetricType(metricBasename);

        metricValue = -1.00;
        metricTimestamp = 0.00;
        metricElapsed = 0.00;
        metricIterations = 0;
        metricTopicCount = 1;
        metricAccumulated = -1.00;
        metricSubIterations = 0;

        gatherData = param_gather;
    }
	ACCUMULATION_STRATEGY parseAccumulationStrategy(std::string str)
	{
		if(0 == str.compare("avg")
		|| 0 == str.compare("Avg")
		|| 0 == str.compare("AVG")
		|| 0 == str.compare("average")
		|| 0 == str.compare("Average")
		|| 0 == str.compare("AVERAGE"))
		{
			return ACCUMULATION_AVG;
		} else
		if(0 == str.compare("max")
		|| 0 == str.compare("Max")
		|| 0 == str.compare("MAX")
		|| 0 == str.compare("maximum")
		|| 0 == str.compare("Maximum")
		|| 0 == str.compare("MAXIMUM"))
		{
			return ACCUMULATION_MAX;
		} else
		if(0 == str.compare("min")
		|| 0 == str.compare("Min")
		|| 0 == str.compare("MIN")
		|| 0 == str.compare("minimum")
		|| 0 == str.compare("Minimum")
		|| 0 == str.compare("MINIMUM"))
		{
			return ACCUMULATION_MIN;
		} else
		if(0 == str.compare("sum")
		|| 0 == str.compare("Sum")
		|| 0 == str.compare("SUM")
		|| 0 == str.compare("summation")
		|| 0 == str.compare("Summation")
		|| 0 == str.compare("SUMMATION"))
		{
			return ACCUMULATION_SUM;
		}
		return ACCUMULATION_AVG;
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
			if(readTimestamp != metricTimestamp)
			{
				metricElapsed = readTimestamp - metricTimestamp;

				metricValue = readValue; // - put value in metricValues
				++metricIterations;
				metricSubIterations = 1;
				if(gatherData)
				{
					if(1 < metricIterations && 1 == metricTopicCount)
					{
						scorep::chrono::ticks nowTicks = scorep::chrono::measurement_clock::now();
						if(metricType == EXAMON_METRIC_TYPE::ENERGY)
						{
							if(0 < ergUnit)
							{
						        gatheredData.push_back(std::pair<scorep::chrono::ticks, double>(nowTicks, metricValue * ergUnit));
							}
						} else
						{
							gatheredData.push_back(std::pair<scorep::chrono::ticks, double>(nowTicks, metricValue));
						}
					}
				}
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
					printf("Applying accumulation strategy %d\n", accStrategy);
					bool completedCycle = metricSubIterations == metricTopicCount;
					switch(accStrategy)
					{
					case ACCUMULATION_AVG:
						metricValue += readValue;
						if(completedCycle)
						{
							metricAccumulated = metricValue / metricTopicCount;
						}
						break;
					case ACCUMULATION_SUM:
						metricValue += readValue;
						if(completedCycle) metricAccumulated = metricValue;
						break;
					case ACCUMULATION_MIN:
						if(metricValue > readValue) metricValue = readValue;
						if(completedCycle) metricAccumulated = metricValue;
						break;
					case ACCUMULATION_MAX:
						if(metricValue < readValue) metricValue = readValue;
						if(completedCycle) metricAccumulated = metricValue;
						break;
					}
					if(completedCycle && gatherData)
					{
						scorep::chrono::ticks nowTicks = scorep::chrono::measurement_clock::now();
						if(metricType == EXAMON_METRIC_TYPE::ENERGY)
						{
							if(0 < ergUnit)
							{
						        gatheredData.push_back(std::pair<scorep::chrono::ticks, double>(nowTicks, metricAccumulated * ergUnit));
							}
						} else
						{
							gatheredData.push_back(std::pair<scorep::chrono::ticks, double>(nowTicks, metricAccumulated));
						}
					}

				}
			}
			metricTimestamp = readTimestamp;



			printf("read metric %9s: %0.4lf, Timestamp:%14.6lf\n", metricName.c_str(), readValue, readTimestamp);
		}
	}
	bool hasValue()
	{
		if(1 < metricIterations)
		{
			if(metricType != EXAMON_METRIC_TYPE::ENERGY || 0 < ergUnit)
			{
				if(1 < metricTopicCount)
				{
					return -1.00 != metricAccumulated;
				} else
				{
				  return true;
				}
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
		if(metricType == EXAMON_METRIC_TYPE::ENERGY)
		{
			if(0 < ergUnit)
			{
    			returnValue = returnValue * ergUnit;
			}
		}
		if(1023 == ((outputCounter++)&1023)) printf("metricName(%s), returnValue(%lf), metricTopicCount(%ld), metricAccumulated(%lf), metricValue(%lf), metricElapsed(%lf), ergUnit(%lf)\n", metricName.c_str(), returnValue, metricTopicCount, metricAccumulated, metricValue, metricElapsed, ergUnit);
		return returnValue;
	}
	std::vector<std::pair<scorep::chrono::ticks, double>>* getGatheredData()
	{
        return &gatheredData;
	}
};


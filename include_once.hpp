/*
 * include_once.hpp
 *
 *  Created on: 10.08.2018
 *      Author: jitschin
 */

#ifndef INCLUDE_ONCE_HPP_
#define INCLUDE_ONCE_HPP_


enum ACCUMULATION_STRATEGY {
	ACCUMULATION_MAX,
	ACCUMULATION_MIN,
	ACCUMULATION_SUM,
	ACCUMULATION_AVG
};

enum class EXAMON_METRIC_TYPE
{
	TEMPERATURE,
	ENERGY,
	FREQUENCY,
	UNKNOWN,
};

inline EXAMON_METRIC_TYPE parseMetricType(char* metricBasename)
{
	EXAMON_METRIC_TYPE metricType = EXAMON_METRIC_TYPE::UNKNOWN;
	if(0 == strncmp("temp", metricBasename, 4))
	{
	    metricType = EXAMON_METRIC_TYPE::TEMPERATURE;
	} else if(0 == strncmp("erg", metricBasename, 3))
	{
	    if(0 != strncmp("erg_units", metricBasename, 9))
		{
            metricType = EXAMON_METRIC_TYPE::ENERGY;
	    }
	 } else if(0 == strncmp("freq", metricBasename, 4))
	 {
	     metricType = EXAMON_METRIC_TYPE::FREQUENCY;
	 }

	return metricType;
}



#endif /* INCLUDE_ONCE_HPP_ */

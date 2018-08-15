/*
 * include_once.hpp
 *
 *  Created on: 10.08.2018
 *      Author: jitschin
 */

#ifndef INCLUDE_ONCE_HPP_
#define INCLUDE_ONCE_HPP_

/* TODO: make this an enum class */
/**
 * Tell which kind of accumulation approach should be used with a metric
 */
enum ACCUMULATION_STRATEGY
{
    ACCUMULATION_MAX,
    ACCUMULATION_MIN,
    ACCUMULATION_SUM,
    ACCUMULATION_AVG
};
/**
 * Parse a simple 3 letter abbreviation of an accumulation strategy
 */
inline ACCUMULATION_STRATEGY parse_accumulation_strategy(std::string str)
{
    if (0 == str.compare("avg") || 0 == str.compare("Avg") || 0 == str.compare("AVG") ||
        0 == str.compare("average") || 0 == str.compare("Average") ||
        0 == str.compare("AVERAGE"))
    {
        return ACCUMULATION_AVG;
    }
    else if (0 == str.compare("max") || 0 == str.compare("Max") || 0 == str.compare("MAX") ||
             0 == str.compare("maximum") || 0 == str.compare("Maximum") ||
             0 == str.compare("MAXIMUM"))
    {
        return ACCUMULATION_MAX;
    }
    else if (0 == str.compare("min") || 0 == str.compare("Min") || 0 == str.compare("MIN") ||
             0 == str.compare("minimum") || 0 == str.compare("Minimum") ||
             0 == str.compare("MINIMUM"))
    {
        return ACCUMULATION_MIN;
    }
    else if (0 == str.compare("sum") || 0 == str.compare("Sum") || 0 == str.compare("SUM") ||
             0 == str.compare("summation") || 0 == str.compare("Summation") ||
             0 == str.compare("SUMMATION"))
    {
        return ACCUMULATION_SUM;
    }
    return ACCUMULATION_AVG;
}



/**
 * Tell which kind of metric we might be dealing with
 */
enum class EXAMON_METRIC_TYPE
{
    TEMPERATURE,
    ENERGY,
    FREQUENCY,
    UNKNOWN,
};

/**
 * Assumes a metric's type based on it's name
 */
inline EXAMON_METRIC_TYPE parse_metric_type(char* metric_basename)
{
    EXAMON_METRIC_TYPE metric_type = EXAMON_METRIC_TYPE::UNKNOWN;
    if (0 == strncmp("temp", metric_basename, 4))
    {
        metric_type = EXAMON_METRIC_TYPE::TEMPERATURE;
    }
    else if (0 == strncmp("erg", metric_basename, 3))
    {
        if (0 != strncmp("erg_units", metric_basename, 9))
        {
            metric_type = EXAMON_METRIC_TYPE::ENERGY;
        }
    }
    else if (0 == strncmp("freq", metric_basename, 4))
    {
        metric_type = EXAMON_METRIC_TYPE::FREQUENCY;
    }

    return metric_type;
}

#endif /* INCLUDE_ONCE_HPP_ */

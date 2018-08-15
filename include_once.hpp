/*
 * include_once.hpp
 *
 *  Created on: 10.08.2018
 *      Author: jitschin
 */

#ifndef INCLUDE_ONCE_HPP_
#define INCLUDE_ONCE_HPP_

/* TODO: make this an enum class */
enum ACCUMULATION_STRATEGY
{
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

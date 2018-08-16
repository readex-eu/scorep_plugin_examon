/*
 * include_once.hpp
 *
 *  Created on: 10.08.2018
 *      Author: jitschin
 */

#ifndef INCLUDE_ONCE_HPP_
#define INCLUDE_ONCE_HPP_

extern "C" {
#include <string.h>
#include <stdarg.h>
}

#include <string>

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
enum class OUTPUT_DATATYPE
{
    DOUBLE,
    INT32_T,
    UINT32_T,
    INT64_T,
    UINT64_T
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

inline bool in_list(std::string comparison_str, ...)
{
    va_list ap;
    va_start(ap, comparison_str);
    char * cur = va_arg(ap, char*);
    while(NULL != cur)
    {
        if(0 == comparison_str.compare(cur))
        {
            return true;
        }
        cur = va_arg(ap, char*);
    }
    va_end(ap);
    return false;
}

inline int parse_metric_options(const char *metric_parameters, ACCUMULATION_STRATEGY &acc_strategy, OUTPUT_DATATYPE &out_datatype)
{
    if(NULL == metric_parameters)
    {
        return -1;
    }
    char *cur_token = NULL;
    char *saveptr = NULL;
    cur_token = strtok_r((char*) metric_parameters, ";", &saveptr);
    while(NULL != cur_token)
    {
        if(in_list(cur_token, "avg", "AVG", "average", "AVERAGE", NULL))
            acc_strategy = ACCUMULATION_AVG;
        else if(in_list(cur_token, "min", "MIN", "minimum", "MINIMUM", NULL))
            acc_strategy = ACCUMULATION_MIN;
        else if(in_list(cur_token, "max", "MAX", "maximum", "MAXIMUM", NULL))
            acc_strategy = ACCUMULATION_MAX;
        else if(in_list(cur_token, "sum", "SUM", "summation", "SUMMATION", NULL))
            acc_strategy = ACCUMULATION_SUM;
        else if(in_list(cur_token, "double", "DOUBLE", NULL))
            out_datatype = OUTPUT_DATATYPE::DOUBLE;
        else if(in_list(cur_token, "int32", "int32_t", "INT32", "INT32_T", NULL))
            out_datatype = OUTPUT_DATATYPE::INT32_T;
        else if(in_list(cur_token, "uint32", "uint32_t", "UINT32", "UINT32_T", NULL))
            out_datatype = OUTPUT_DATATYPE::UINT32_T;
        else if(in_list(cur_token, "int64", "int64_t", "INT64", "INT64_T", NULL))
            out_datatype = OUTPUT_DATATYPE::INT64_T;
        else if(in_list(cur_token, "uint64", "uint64_t", "UINT64", "UINT64_T", NULL))
            out_datatype = OUTPUT_DATATYPE::UINT64_T;
        cur_token = strtok_r(NULL, (char*) ";",  &saveptr);
    }

    return 0;
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

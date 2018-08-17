/*
 * include_once.hpp
 *
 *  Created on: 10.08.2018
 *      Author: jitschin
 *
 * Copyright (c) 2018, Technische Universität Dresden, Germany
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions
 *    and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other materials provided with
 * the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef INCLUDE_ONCE_HPP_
#define INCLUDE_ONCE_HPP_

extern "C" {
#include <string.h>
#include <stdarg.h>
}

#include <string>
#include <utility>
#include <scorep/plugin/plugin.hpp>

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

inline int parse_metric_options(const char *metric_parameters, ACCUMULATION_STRATEGY &acc_strategy, OUTPUT_DATATYPE &out_datatype, double &scale_mul)
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
        else if(0 == strncmp(cur_token, "s=", 2) || 0 == strncmp(cur_token, "S=", 2))
            sscanf(cur_token + 2, "%lf", &scale_mul);
        cur_token = strtok_r(NULL, (char*) ";",  &saveptr);
    }

    return 0;
}

/**
 * actually this is a replacement for std::pair
 *
 * since unfortunately std::pair was not capable of holding values
 * that don't have a default constructor with zero arguments
 */
struct metric_property_return
{
public:
    scorep::plugin::metric_property* props = NULL;
    EXAMON_METRIC_TYPE metric_type;
    metric_property_return()
    {
    }
};


inline struct metric_property_return *preprocess_metric_property(std::string metric_parse)
{
    std::string metric_basename = basename((char*)metric_parse.c_str());
    if (NULL != strchr(metric_basename.c_str(), '+') ||
        NULL != strchr(metric_basename.c_str(), '#'))
    {
        fprintf(stderr, "Can't allow metric \"%s\", don't know how to accumulate it. Only "
                        "homogenetic path endings allowed.\n",
                metric_basename.c_str());
        return NULL;
    }
    else
    {
        struct metric_property_return* return_pair = new struct metric_property_return();
        if(0 == metric_parse.compare("EXAMON/BLADE/E"))
        {
            if(NULL != getenv("SCOREP_METRIC_EXAMON_SYNC_PLUGIN_READEX_BLADE"))
            {
                metric_parse = getenv("SCOREP_METRIC_EXAMON_SYNC_PLUGIN_READEX_BLADE");
            } else if(NULL != getenv("SCOREP_METRIC_EXAMON_ASYNC_PLUGIN_READEX_BLADE"))
            {
                metric_parse = getenv("SCOREP_METRIC_EXAMON_ASYNC_PLUGIN_READEX_BLADE");
            } else
            {
                return NULL;
            }
        }

        char *buf = (char*)malloc(metric_parse.length() + 17);
        sprintf(buf, "Examon metric \"%s\"", metric_parse.c_str());
        buf[metric_parse.length() + 16] = '\0';
        EXAMON_METRIC_TYPE metric_type = parse_metric_type((char*)metric_basename.c_str());
        ACCUMULATION_STRATEGY acc_strategy = ACCUMULATION_AVG;
        OUTPUT_DATATYPE metric_datatype = OUTPUT_DATATYPE::DOUBLE;
        double scale_mul = 1.00;
        int semicolon_pos = metric_basename.find_first_of(';');
        if (std::string::npos != semicolon_pos)
        {
            parse_metric_options(metric_basename.substr(semicolon_pos + 1, std::string::npos).c_str(), acc_strategy, metric_datatype, scale_mul);
        }
        std::string unit = "";
        if(1000000000000 == scale_mul)
        {
            unit = "T";
        } else if(1000000000 == scale_mul)
        {
            unit = "G";
        } else if(1000000 == scale_mul)
        {
            unit = "M";
        } else if(1000 == scale_mul)
        {
            unit = "K";
        } else if(1 == scale_mul)
        {
            unit = "";
        } else if(0.001 == scale_mul)
        {
            unit = "m";
        } else if(0.000001 == scale_mul)
        {
            unit = "µ";
        } else if(0.000000001 == scale_mul)
        {
            unit = "n";
        } else if(0.000000000001 == scale_mul)
        {
            unit = "p";
        } else {
            unit = std::to_string(scale_mul) + " ";
        }

        switch (metric_type)
        {
        case EXAMON_METRIC_TYPE::TEMPERATURE:
            unit += "C";
            break;
        case EXAMON_METRIC_TYPE::ENERGY:
            unit += "J";
            break;
        case EXAMON_METRIC_TYPE::FREQUENCY:
            unit += "Hz";
            break;

        }

        scorep::plugin::metric_property* prop =
            new scorep::plugin::metric_property(metric_parse, buf, unit.c_str());


        switch(metric_datatype)
        {
        case OUTPUT_DATATYPE::DOUBLE:
            prop->value_double();
            break;
        case OUTPUT_DATATYPE::INT32_T:
        case OUTPUT_DATATYPE::INT64_T:  /* fall-through */
            prop->value_int();  // Score-P just knows INT64_T
            break;
        case OUTPUT_DATATYPE::UINT32_T:
        case OUTPUT_DATATYPE::UINT64_T:  /* fall-through */
            prop->value_uint();  // Score-P just knows UINT64_T
            break;
        }
        prop->decimal();

        return_pair->props = prop;
        return_pair->metric_type = metric_type;
        //should not be freed here
        //free(buf);

        return return_pair;
    }
}




#endif /* INCLUDE_ONCE_HPP_ */

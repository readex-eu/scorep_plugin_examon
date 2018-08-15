/*
 * examon_metric.c
 *
 *  Created on: 09.08.2018
 *      Author: jitschin
 */
extern "C" {
#include <libgen.h>
#include <string.h>
}

#include <mosquittopp.h>
#include <scorep/chrono/chrono.hpp>

#include <utility>
#include <vector>

#include "examon_mqtt_path.cpp"
#include <string>

#include "include_once.hpp"

class examon_metric
{
private:
    std::int32_t id;
    std::string name;
    examon_mqtt_path* channels;
    std::string full_topic;
    double metric_value;

    double metric_timestamp;
    double metric_elapsed;
    std::int64_t metric_iterations;
    std::int64_t metric_topic_count;
    double metric_accumulated;
    std::int64_t metric_sub_iterations;
    ACCUMULATION_STRATEGY acc_strategy;
    EXAMON_METRIC_TYPE metric_type;
    double erg_unit;
    bool do_gather_data;
    std::vector<std::pair<scorep::chrono::ticks, double>> gathered_data;


public:
    void set_erg_unit(double param_erg_unit)
    {
        erg_unit = param_erg_unit;
    }
    std::string get_full_topic()
    {
        return full_topic;
    }
    std::string get_name()
    {
        return name;
    }
    void set_gather_data(bool param_do_gather)
    {
        do_gather_data = param_do_gather;
    }
    std::int32_t get_id()
    {
        return id;
    }
    std::vector<std::pair<scorep::chrono::ticks, double>>* get_gathered_data()
    {
        return &gathered_data;
    }
    examon_metric(std::int32_t param_id, std::string param_name, examon_mqtt_path* param_channels,
                  bool param_gather)
    {
        id = param_id;
        /* configurable accumulation strategy, TODO: also have a heuristic to determine this setting
         * if not explicitly specified */
        acc_strategy = ACCUMULATION_AVG;
        int semicolon_pos = param_name.find_first_of(';');
        if (std::string::npos != semicolon_pos)
        {
            acc_strategy =
                parse_accumulation_strategy(param_name.substr(semicolon_pos + 1, std::string::npos));
            param_name = param_name.substr(0, semicolon_pos);
        }
        name = param_name;
        channels = param_channels;
        full_topic = channels->get_data_topic(name);
        char* metric_basename = basename((char*)name.c_str());
        metric_type = parse_metric_type(metric_basename);

        metric_value = -1.00;
        metric_timestamp = 0.00;
        metric_elapsed = 0.00;
        metric_iterations = 0;
        metric_topic_count = 1;
        metric_accumulated = -1.00;
        metric_sub_iterations = 0;

        do_gather_data = param_gather;
    }
    ACCUMULATION_STRATEGY parse_accumulation_strategy(std::string str)
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
    bool metric_matches(char* incoming_topic)
    {
        bool topic_matches = false;
        mosqpp::topic_matches_sub(full_topic.c_str(), incoming_topic, &topic_matches);
        return topic_matches;
    }
    void handle_message(char* incoming_topic, char* incoming_payload, int payloadlen)
    {
        double read_value = -1;
        double read_timestamp = -1;
        int values_read = sscanf(incoming_payload, "%lf;%lf", &read_value, &read_timestamp);
        if (2 == values_read)
        {
            if (read_timestamp != metric_timestamp)
            {
                metric_elapsed = read_timestamp - metric_timestamp;

                metric_value = read_value; // - put value in metricValues
                ++metric_iterations;
                metric_sub_iterations = 1;
                if (do_gather_data)
                {
                    if (1 < metric_iterations && 1 == metric_topic_count)
                    {
                        push_latest_value(false);
                    }
                }
            }
            else
            {
                ++metric_sub_iterations;
                if (1 == metric_iterations)
                {
                    ++metric_topic_count;
                }
                else
                {
                    // TODO: put all this logic in a separate class for the metric
                    //         optional: implement fancy pthread_mutex_locking for it

                    // accumulate strategy
                    // at this point metricTopicCount contains a sensible number of actually
                    //   subscribed to metrics

                    // here is the first duplicate, i.e. timestamps are equal
                    // so the current preceding value was already written to metricValues[i]
                    bool completed_cycle = metric_sub_iterations == metric_topic_count;
                    switch (acc_strategy)
                    {
                    case ACCUMULATION_AVG:
                        metric_value += read_value;
                        if (completed_cycle)
                        {
                            metric_accumulated = metric_value / metric_topic_count;
                        }
                        break;
                    case ACCUMULATION_SUM:
                        metric_value += read_value;
                        if (completed_cycle)
                            metric_accumulated = metric_value;
                        break;
                    case ACCUMULATION_MIN:
                        if (metric_value > read_value)
                            metric_value = read_value;
                        if (completed_cycle)
                            metric_accumulated = metric_value;
                        break;
                    case ACCUMULATION_MAX:
                        if (metric_value < read_value)
                            metric_value = read_value;
                        if (completed_cycle)
                            metric_accumulated = metric_value;
                        break;
                    }
                    if (completed_cycle && do_gather_data)
                    {
                        push_latest_value(true);
                    }
                }
            }
            metric_timestamp = read_timestamp;

        }
    }
    bool has_value()
    {
        if (1 < metric_iterations)
        {
            if (metric_type != EXAMON_METRIC_TYPE::ENERGY || 0 < erg_unit)
            {
                if (1 < metric_topic_count)
                {
                    return -1.00 != metric_accumulated;
                }
                else
                {
                    return true;
                }
            }
        }
        return false;
    }
    double get_latest_value()
    {
        double return_value = 0.00;
        if (1 < metric_topic_count)
        {
            return_value = metric_accumulated;
        }
        else
        {
            return_value = metric_value;
        }
        if (metric_type == EXAMON_METRIC_TYPE::ENERGY)
        {
            if (0 < erg_unit)
            {
                return_value = return_value * erg_unit;
            }
        }
        return return_value;
    }
    void push_latest_value(bool accumulated)
    {
        // std::chrono::system_clock::time_point
        scorep::chrono::ticks now_ticks = scorep::chrono::measurement_clock::now();
        double write_metric = 0.00;
        if (accumulated)
            write_metric = metric_accumulated;
        else
            write_metric = metric_value;
        if (metric_type == EXAMON_METRIC_TYPE::ENERGY)
        {
            if (0 < erg_unit)
            {

                gathered_data.push_back(
                    std::pair<scorep::chrono::ticks, double>(now_ticks, write_metric * erg_unit));
            }
        }
        else
        {
            gathered_data.push_back(
                std::pair<scorep::chrono::ticks, double>(now_ticks, write_metric));
        }
    }
};

/*
 * examon_sync_plugin.cpp
 *
 *  Created on: 26.07.2018
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

/* #include </usr/include/c++/5/string> */
#include <string>
#include <vector>
/* very basic plugin base */
#include <scorep/plugin/plugin.hpp>
/* use phtread */
#include <pthread.h>
/* for gethostname() */
#include <unistd.h>

#include <mosquittopp.h>

#include "examon_metric.cpp"

extern "C" {
#include <math.h>
}

namespace spp = scorep::plugin::policy;
/**
 * Score-P Plugin for Examon synchronous variant
 *
 * It expects to be provided with metric names by Score-P
 * which reads those values from environment variable
 * SCOREP_METRIC_EXAMON_SYNC_PLUGIN as a comma-separated list
 * Do note, that it may also take more environment variables like
 * SCOREP_METRIC_EXAMON_SYNC_PLUGIN_BROKER
 * SCOREP_METRIC_EXAMON_SYNC_PLUGIN_EXAMON_HOST
 * SCOREP_METRIC_EXAMON_SYNC_PLUGIN_CHANNEL
 * SCOREP_METRIC_EXAMON_SYNC_PLUGIN_INTERVAL
 * for connecting to MQTT and Examon
 *
 */
class examon_sync_plugin : public scorep::plugin::base<examon_sync_plugin, spp::per_host, spp::sync,
                                                       spp::scorep_clock, spp::synchronize>,
                           public mosqpp::mosquittopp
{
private:
    std::vector<examon_metric> metrics;  /**< the metrics that are being tracked */
    /**
     * invoked by add_metric() to register the desired metric
     */
    std::int32_t put_metric(std::string metric_name)
    {
        std::int32_t biggest_int = 0;
        if (0 < metrics.size())
            biggest_int = metrics[metrics.size() - 1].get_id();

        std::int32_t new_id = biggest_int + 1;
        metrics.push_back(examon_metric(new_id, metric_name, channels, false));

        return new_id;
    }
    char* hostname = NULL;  /**< the hostname of the system this program runs on */
    char* examon_host = NULL;  /**< the hostname of the system where examon is running at */
    char* connect_host = NULL;  /**< the host or address to the system on which MQTT is running */
    char* interval_duration = NULL;  /**< the delay that is told examon to heed between measurements */
    examon_mqtt_path* channels = NULL;  /**< a semantic abstraction of Examon topics */
    double erg_unit = -1;  /**< cache of erg_unit metric value */
    bool track_erg_unit = false;  /**< whether to subscribe to erg_unit to fetch it's value */
    bool is_connected = false;  /**< internally, stores if we are connected to MQTT */
    bool is_alive = true;  /**< internally, stores whether the thread is running */
    bool subscribed_to_metrics = false;  /**< stores whether it has already invoked subscribe() for the metrics with MQTT */
    pthread_t *thread_id = NULL;  /**< store the thread */

public:
    /**
     * initialization of this plugin
     *
     * Reads out environment variables and connects to
     * specified MQTT host, initializes the pthread
     */
    examon_sync_plugin()
    {
        int port = 1883; // default port
        int keepalive = 60;

        hostname = (char*)malloc(1024);
        int rc = gethostname(hostname, 1024);
        if (rc)
        {
            fprintf(stderr, "Could not read hostname of this host, abort imminent.\n");
            exit(0);
        }
        // Read out environment variables
        connect_host = getenv("SCOREP_METRIC_EXAMON_SYNC_PLUGIN_BROKER");
        examon_host = getenv("SCOREP_METRIC_EXAMON_SYNC_PLUGIN_EXAMON_HOST");
        char* config_channel = getenv("SCOREP_METRIC_EXAMON_SYNC_PLUGIN_CHANNEL");
        interval_duration = getenv("SCOREP_METRIC_EXAMON_SYNC_PLUGIN_INTERVAL");
        if (!connect_host)
            connect_host = (char*)"localhost";
        if (!examon_host)
            examon_host = hostname;
        if (!config_channel)
            config_channel = (char*)"org/antarex/cluster/testcluster";
        if (interval_duration)
        {
            float dummy_val;
            if (1 != sscanf(interval_duration, "%f", &dummy_val))
            {
                fprintf(stderr,
                        "Could not parse intervalDuration of \"%s\", not a floating point number\n",
                        interval_duration);
            }
        }
        channels = new examon_mqtt_path(examon_host, config_channel);

        mosqpp::lib_init();

        connect(connect_host, port, keepalive);

        is_alive = true;

        pthread_t thread_structure;
        pthread_create(&thread_structure, NULL, &pthread_loop, this);
        thread_id = &thread_structure;
    }
    ~examon_sync_plugin()
    {
        /*  close connection to mosquitto */
        is_alive = false;
        is_connected = false;
        disconnect();
        delete channels;
        free(hostname);
        mosqpp::lib_cleanup();
    }
    /**
     * returns if the thread is still running
     */
    bool alive()
    {
        return is_alive;
    }
    /**
     * returns whether there is an active connection to MQTT
     */
    bool connected()
    {
        return is_connected;
    }
    /**
     * loops so that the MQTT connection stays active
     *
     * Is invoked from the pthread started within the constructor
     * will loop until disconnect() is called and then it will
     * finish
     */
    static void* pthread_loop(void* arg)
    {
        examon_sync_plugin* myObj = (examon_sync_plugin*)arg;

        myObj->loop_forever();
    }
    /**
     * Called internally to subscribe to each metric in MQTT
     */
    void subscribe_the_metrics()
    {
        for (int i = 0; i < metrics.size(); ++i)
        {
            /* DONE: Make it multi-socket compatible (replace "0" below with "+",
             *         also implement logic for receiving multiple values at once */
            subscribe(NULL, metrics[i].get_full_topic().c_str());
        }
    }
    /**
     * Will update all metrics to the specified erg_unit
     */
    void update_erg_unit(double erg_unit)
    {
        for (int i = 0; i < metrics.size(); ++i)
        {
            metrics[i].set_erg_unit(erg_unit);
        }
    }
    /**
     * Called by MQTT once a connection could be established
     */
    void on_connect(int rc)
    {
        if (rc == 0)
        {

            is_connected = true;

            if (interval_duration)
            {
                char* interval_str = (char*)malloc(100);
                strcpy(interval_str, "-s ");
                strncpy(interval_str + 3, interval_duration, 95);
                publish(NULL, channels->topic_cmd().c_str(), strlen(interval_str), interval_str);
                free(interval_str);
            }

            if (track_erg_unit)
            {
                subscribe(NULL, channels->topic_erg_units().c_str());
            }
        }
    }
    /**
     * Called by MQTT when a message has been received
     */
    void on_message(const struct mosquitto_message* message)
    {
        if (channels->starts_with_topic_base(message->topic))
        {
            if (-1 == erg_unit && channels->is_erg_units(message->topic))
            {
                if (1 == sscanf((char*)message->payload, "%lf;%*f", &erg_unit))
                {
                    // energy unit
                    // taken from
                    // https://github.com/deater/uarch-configure/blob/master/rapl-read/rapl-read.c#L403
                    // also see http://web.eece.maine.edu/~vweaver/projects/rapl/
                    erg_unit = pow(0.5, (double)((((int)erg_unit) >> 8) & 0x1f));

                    unsubscribe(NULL, channels->topic_erg_units().c_str());
                    update_erg_unit(erg_unit);
                }
            }

            if (0 == strncmp("/data/", message->topic + channels->topic_base().length(), 6))
            {
                for (int i = 0; i < metrics.size(); ++i)
                {

                    if (metrics[i].metric_matches(message->topic))
                    {
                        metrics[i].handle_message(message->topic, (char*)message->payload,
                                                  message->payloadlen);
                        // Don't break here, allow for multiple handling
                    }
                }
            }
        }
    }
    /**
     * Called by MQTT, not required here
     */
    void on_subscribe(int mid, int qos_count, const int* granted_qos)
    {
    }
    /**
     * Called by MQTT, not required here
     */
    void on_unsubscribe(int mid)
    {
    }

    /**
     * Is supposed to return a list of metrics/matching Properties to Score-P
     *
     * Actually it just tries to check if the specified metric
     * does not end with a MQTT-wildcard (i.e. '+' or '#').
     * If it does not, it enriches the specified metric with it's unit
     * and a description of how it should be displayed (metric_property)
     */
    std::vector<scorep::plugin::metric_property>
    get_metric_properties(const std::string& metric_parse)
    {

        /* some cpu metrics:
         * tsc, temp_pkg, erg_dram, erg_cores, erg_pkg, erg_units, freq_ref, C2, C3, C6, uclk
         * some core metrics:
         * aperf, clk_curr, clk_ref, C3, C6, inst, mperf, temp, tsc
         * UOPS_RETIRED.RETIRE_SLOTS, CACHE.MISSES, LONGEST_LAT_CACHE.MISS,
         * MEM_LOAD_RETIRED.ALL_BRANCHES
         * UOPS_ISSUED.ANY, IDQ_UOPS_NOT_DELIVERED.CORE, INT_MISCRECOVERY_CYCLES
         */

        std::vector<scorep::plugin::metric_property> found_matches =
            std::vector<scorep::plugin::metric_property>();

        // revoke paths like cpu/0/+
        //                or cpu/+/+
        //                or +/+/+
        struct metric_property_return *parsed_pair = preprocess_metric_property(metric_parse);
        if(NULL != parsed_pair) {
            switch (parsed_pair->metric_type)
            {
            case EXAMON_METRIC_TYPE::TEMPERATURE:
                parsed_pair->props->absolute_point();
                break;
            case EXAMON_METRIC_TYPE::ENERGY:
                parsed_pair->props->accumulated_last();
                track_erg_unit = true;
                break;
            case EXAMON_METRIC_TYPE::FREQUENCY:
                parsed_pair->props->absolute_last();
                break;
            case EXAMON_METRIC_TYPE::UNKNOWN:
                parsed_pair->props->accumulated_last();
                break;
            }
            found_matches.push_back(*(parsed_pair->props));

            delete parsed_pair;
        }
        return found_matches;
    }
    /**
     * Receives the desired metrics from Score-P
     *
     * Receive metrics here, register them internally with a std::int32_t, which will be later used
     * by score-p to reference the metric here
     **/
    int32_t add_metric(const std::string& metric_name)
    {

        return put_metric(metric_name);
    }

    /** Will be called for every event in by the measurement environment.
      * You may or may not give it a value here.
      *
      * @param id contains the stored metric informations
      * @param p get and save the results
      *
      **/
    template <class Proxy>
    bool get_optional_value(std::int32_t id, Proxy& p)
    {
        if (is_connected)
        {
            int index = id - 1;
            if (0 <= index && index < metrics.size() && metrics[index].get_id() == id)
            {
                if (metrics[index].has_value())
                {
                    switch(metrics[index].get_output_datatype())
                    {
                    case OUTPUT_DATATYPE::DOUBLE:
                        p.store(metrics[index].get_latest_value());
                        break;
                    case OUTPUT_DATATYPE::INT32_T:
                    case OUTPUT_DATATYPE::INT64_T:  /* fall-through */
                        p.store((std::int64_t) metrics[index].get_latest_value());  // Score-P just knows INT64_T
                        break;
                    case OUTPUT_DATATYPE::UINT32_T:
                    case OUTPUT_DATATYPE::UINT64_T:  /* fall-through */
                    p.store((std::uint64_t) metrics[index].get_latest_value());  // Score-P just knows UINT64_T
                        break;
                    }
                    return true;
                }
            }
        }
        return false;
    }
    /**
     * Called by Score-P when the plugin is supposed to start and when it is supposed to end
      */
    void synchronize(bool is_responsible, SCOREP_MetricSynchronizationMode sync_mode)
    {
        if (!subscribed_to_metrics)
        {
            subscribe_the_metrics();
            subscribed_to_metrics = true;
        }
    }
};

SCOREP_METRIC_PLUGIN_CLASS(examon_sync_plugin, "examon_sync")

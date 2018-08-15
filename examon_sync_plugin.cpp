/*
 * examon_sync_plugin.cpp
 *
 *  Created on: 26.07.2018
 *      Author: jitschin
 *
 * Copyright (c) 2016, Technische Universität Dresden, Germany
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
/* matching for get_metric_properties */
#include <scorep/plugin/util/matcher.hpp>
/* only for debugging */
#include <iostream>
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

class examon_sync_plugin : public scorep::plugin::base<examon_sync_plugin, spp::per_host, spp::sync,
                                                       spp::scorep_clock, spp::synchronize>,
                           public mosqpp::mosquittopp
{
private:
    bool valid_connection = false;
    struct mosquitto* mosq = NULL;
    std::vector<examon_metric> metrics;
    std::int32_t put_metric(std::string metric_name)
    {
        std::int32_t biggest_int = 0;
        if (0 < metrics.size())
            biggest_int = metrics[metrics.size() - 1].get_id();

        std::int32_t new_id = biggest_int + 1;
        metrics.push_back(examon_metric(new_id, metric_name, channels, false));

        return new_id;
    }
    char* hostname = NULL;
    char* examon_host = NULL;
    char* connect_host = NULL;
    char* interval_duration = NULL;
    examon_mqtt_path* channels = NULL;
    double erg_unit = -1;
    bool track_erg_unit = false;
    bool is_connected = false;
    bool is_alive = true;
    bool subscribed_to_metrics = false;

public:
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

        printf("calling non-async connect\n");
        connect(connect_host, port, keepalive);

        is_alive = true;
        pthread_t thread_id; /* TODO: store the thread id */
        pthread_create(&thread_id, NULL, &pthread_loop, this);
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
    bool alive()
    {
        return is_alive;
    }
    bool connected()
    {
        return is_connected;
    }
    static void* pthread_loop(void* arg)
    {
        examon_sync_plugin* myObj = (examon_sync_plugin*)arg;

        myObj->loop_forever();
    }
    void subscribe_the_metrics()
    {
        for (int i = 0; i < metrics.size(); ++i)
        {
            /* DONE: Make it multi-socket compatible (replace "0" below with "+",
             *         also implement logic for receiving multiple values at once */
            printf("Trying to subscribe to %s.\n", metrics[i].get_full_topic().c_str());
            subscribe(NULL, metrics[i].get_full_topic().c_str());
        }
    }
    void update_erg_unit(double erg_unit)
    {
        for (int i = 0; i < metrics.size(); ++i)
        {
            metrics[i].set_erg_unit(erg_unit);
        }
    }
    void on_connect(int rc)
    {
        printf("Connected with code %d.\n", rc);
        if (rc == 0)
        {

            is_connected = true;

            if (interval_duration)
            {
                char* interval_str = (char*)malloc(100);
                strcpy(interval_str, "-s ");
                strncpy(interval_str + 3, interval_duration, 95);
                printf("Trying to publish to channel \"%s\", value \"%s\".\n",
                       channels->topic_cmd().c_str(), interval_str);
                publish(NULL, channels->topic_cmd().c_str(), strlen(interval_str), interval_str);
                free(interval_str);
            }

            if (track_erg_unit)
            {
                printf("Trying to subscribe to %s.\n", channels->topic_erg_units().c_str());
                subscribe(NULL, channels->topic_erg_units().c_str());
            }
        }
    }
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
    void on_subscribe(int mid, int qos_count, const int* granted_qos)
    {
        printf("Subscription succeeded.\n");
    }
    void on_unsubscribe(int mid)
    {
        printf("unsubscribed.\n");
    }

    /* return matching properties */
    std::vector<scorep::plugin::metric_property>
    get_metric_properties(const std::string& metric_parse)
    {
        // DEBUG
        std::cout << "get_metric_properties(\"" << metric_parse << "\")" << std::endl;
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
        std::string metric_basename = basename((char*)metric_parse.c_str());
        if (NULL != strchr(metric_basename.c_str(), '+') ||
            NULL != strchr(metric_basename.c_str(), '#'))
        {
            fprintf(stderr, "Can't allow metric \"%s\", don't know how to accumulate it. Only "
                            "homogenetic path endings allowed.\n",
                    metric_basename.c_str());
            return found_matches;
        }
        else
        {
            char* buf = (char*)malloc(metric_parse.length() + 17);
            sprintf(buf, "Examon metric \"%s\"", metric_parse.c_str());
            buf[metric_parse.length() + 16] = '\0';
            char* unit = (char*)"";
            EXAMON_METRIC_TYPE metric_type = parse_metric_type((char*)metric_basename.c_str());
            switch (metric_type)
            {
            case EXAMON_METRIC_TYPE::TEMPERATURE:
                unit = (char*)"C";
                break;
            case EXAMON_METRIC_TYPE::ENERGY:
                track_erg_unit = true;
                unit = (char*)"J";
                break;
            case EXAMON_METRIC_TYPE::FREQUENCY:
                unit = (char*)"Hz";
                break;
            }

            scorep::plugin::metric_property prop =
                scorep::plugin::metric_property(metric_parse, buf, unit);
            switch (metric_type)
            {
            case EXAMON_METRIC_TYPE::TEMPERATURE:
                prop.absolute_point();
                break;
            case EXAMON_METRIC_TYPE::ENERGY:
                prop.accumulated_last();
                break;
            case EXAMON_METRIC_TYPE::FREQUENCY:
                prop.absolute_last();
                break;
            case EXAMON_METRIC_TYPE::UNKNOWN:
                prop.accumulated_last();
                break;
            }
            prop.value_double();
            prop.decimal();
            found_matches.push_back(prop);

            free(buf);

            return found_matches;
        }
    }
    /* receive metrics here, register them internally with a std::int32_t, which will be later used
     * by score-p to reference the metric here */
    int32_t add_metric(const std::string& metric_name)
    {
        // DEBUG
        std::cout << "add_metric(\"" << metric_name << "\")" << std::endl;

        return put_metric(metric_name);
    }

    /** Will be called for every event in by the measurement environment.
      * You may or may not give it a value here.
      *
      * @param m contains the stored metric informations
      * @param proxy get and save the results
      *
      **/
    /* get_current_value is the strict variant, value is written to &proxy */
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
                    p.store(metrics[index].get_latest_value());
                    return true;
                }
            }
        }
        return false;
    }
    /** function to determine the responsible process for x86_energy
      *
      * If there is no MPI communication, the x86_energy communication is PER_PROCESS,
      * so Score-P cares about everything.
      * If there is MPI communication and the plugin is build with -DHAVE_MPI,
      * we are grouping all MPI_Processes according to their hostname hash.
      * Then we select rank 0 to be the responsible rank for MPI communication.
      *
      * @param is_responsible the Score-P responsibility
      * @param sync_mode sync mode, i.e. SCOREP_METRIC_SYNCHRONIZATION_MODE_BEGIN for non MPI
      *              programs and SCOREP_METRIC_SYNCHRONIZATION_MODE_BEGIN_MPP for MPI program.
      *              Does not deal with SCOREP_METRIC_SYNCHRONIZATION_MODE_END
      */
    void synchronize(bool is_responsible, SCOREP_MetricSynchronizationMode sync_mode)
    {
        printf("synchronize() called\n");
        if (!subscribed_to_metrics)
        {
            subscribe_the_metrics();
            subscribed_to_metrics = true;
        }
    }
};

SCOREP_METRIC_PLUGIN_CLASS(examon_sync_plugin, "examon_sync")

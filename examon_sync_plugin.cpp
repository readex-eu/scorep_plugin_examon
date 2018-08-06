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


#include <mosquittopp.h>


namespace spp = scorep::plugin::policy;




/* we coould inherit from lib/cpp/mosquitto.h and have a much easier implementation here... */
class examon_sync_plugin
: public scorep::plugin::base<examon_sync_plugin, spp::per_host, spp::sync,
                              spp::scorep_clock, spp::synchronize>, public mosqpp::mosquittopp
{
private:
	bool valid_connection = false;
	struct mosquitto *mosq = NULL;
	std::vector<std::string> metricNames;
	std::vector<std::int32_t> metricIds;
	std::int32_t put_metric(std::string metric)
	{
		std::int32_t biggestInt = -1;
		for(int i = 0; i < metricIds.size(); ++i)
		{
			if(metricIds[i] > biggestInt)
			{
				biggestInt = metricIds[i];
			}
		}
        std::int32_t newId = biggestInt + 1;
        metricIds.push_back(newId);
        metricNames.push_back(metric);

        return newId;
	}
public:
	examon_sync_plugin()
	{
		const char *id;//dunno what this is for
		const char *host = (char*) "localhost";
		int port = 1883; // default port

	    int keepalive = 60;

	    mosqpp::lib_init();

		connect(host, port, keepalive);

        pthread_t thread_id; /* TODO: store the thread id */
        pthread_create(&thread_id, NULL, &pthread_loop, this);
	}
	~examon_sync_plugin()
	{
		/*  close connection to mosquitto */
		mosqpp::lib_cleanup();

	}
	static void* pthread_loop(void* arg)
	{
		examon_sync_plugin* myObj = (examon_sync_plugin*) arg;
		myObj->loop_forever();
	}
	void on_connect(int rc)
	{
		printf("Connected with code %d.\n", rc);
		if(rc == 0){
			/* Only attempt to subscribe on a successful connect. */
			subscribe(NULL, "+/+/+/+/+/+");
		}
	}
	void on_message(const struct mosquitto_message *message)
	{
		char buf[101];

		//strcmp(message->topic, "channel/name")
		memset(buf, 0, 101*sizeof(char));
		/* Copy N-1 bytes to ensure always 0 terminated. */
		memcpy(buf, message->payload, 100*sizeof(char));
		printf("Received message, topic: %s, payload: %s\n", message->topic, buf);
	}
	void on_subscribe(int mid, int qos_count, const int *granted_qos)
	{
		printf("Subscription succeeded.\n");
	}


    /* return matching properties */
	std::vector<scorep::plugin::metric_property> get_metric_properties(const std::string& metric_parse)
	{
      std::cout << "get_metric_properties(\"" << metric_parse << "\")"<< std::endl;
      scorep::plugin::util::matcher myMatcher = scorep::plugin::util::matcher(metric_parse);
      std::vector<scorep::plugin::metric_property> foundMatches = std::vector<scorep::plugin::metric_property>();
      if(myMatcher("tsc") || myMatcher("Joule") || myMatcher("Watt"))
      {
    	  scorep::plugin::metric_property prop = scorep::plugin::metric_property("Joules", "Used energy in Joules", "J");
    	  foundMatches.push_back(prop);
      }

      return foundMatches;
	}
/* receive metrics here, register them internally with a std::int32_t, which will be later used by score-p to reference the metric here */
	int32_t add_metric(const std::string& metric_name)
	{
      //DEBUG
	  std::cout << "add_metric(\"" << metric_name << "\")"<< std::endl;
      if(metric_name == "Joule"
      || metric_name == "joule"
	  || metric_name == "tsc")
      {
    	  return put_metric("tsc");
      }

      return 0; // 0 means failure, I guess
	}


	 /** Will be called for every event in by the measurement environment.
	   * You may or may not give it a value here.
	   *
	   * @param m contains the stored metric informations
	   * @param proxy get and save the results
	   *
	   **/
	/* get_current_value is the strict variant, value is written to &proxy */
	template <class Proxy> bool get_optional_value(std::int32_t id, Proxy& p)
	{
	  /* TODO: do something */
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
      /* TODO: Do something */
	}
};



SCOREP_METRIC_PLUGIN_CLASS(examon_sync_plugin, "examon_sync")



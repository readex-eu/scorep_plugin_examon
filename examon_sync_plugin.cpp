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


namespace spp = scorep::plugin::policy;



class examon_sync_plugin
: public scorep::plugin::base<examon_sync_plugin, spp::per_host, spp::sync,
                              spp::scorep_clock, spp::synchronize>, public mosqpp::mosquittopp
{
private:
	bool valid_connection = false;
	struct mosquitto *mosq = NULL;
	std::vector<std::int32_t> metricIds;
    std::vector<examon_metric> metrics;
    std::int32_t put_metric(std::string metric)
	{
		std::int32_t biggestInt = 0;
		if(0 < metricIds.size()) biggestInt = metricIds[metricIds.size() - 1];

		std::int32_t newId = biggestInt + 1;
        metricIds.push_back(newId);
        metrics.push_back(examon_metric(newId, metric, channels));

        return newId;
	}
	char* hostname = NULL;
	char* examonHost = NULL;
	char* connectHost = NULL;
	char* intervalDuration = NULL;
	examon_mqtt_path* channels = NULL;
	double lastValue = 0;
	double lastTimestamp = 0;
	double ergUnit = -1;
	bool trackErgUnit = false;
	bool isConnected = false;
	bool isAlive = true;
	bool subscribedToMetrics = false;
public:
	examon_sync_plugin()
	{
		int port = 1883; // default port
	    int keepalive = 60;

	    hostname = (char *) malloc(1024);
	    int rc = gethostname(hostname, 1024);
	    if(rc) {
        	fprintf(stderr, "Could not read hostname of this host, abort imminent.\n");
        	exit(0);
	    }
	    connectHost =         getenv("SCOREP_METRIC_EXAMON_SYNC_PLUGIN_BROKER");
	    examonHost =          getenv("SCOREP_METRIC_EXAMON_SYNC_PLUGIN_EXAMON_HOST");
	    char* configChannel = getenv("SCOREP_METRIC_EXAMON_SYNC_PLUGIN_CHANNEL");
	    intervalDuration =    getenv("SCOREP_METRIC_EXAMON_SYNC_PLUGIN_INTERVAL");
        if(!connectHost)   connectHost = (char*) "localhost";
        if(!examonHost)    examonHost = hostname;
        if(!configChannel) configChannel = (char*) "org/antarex/cluster/testcluster";
        if(intervalDuration)
        {
            float dummyVal;
            if(1 != sscanf(intervalDuration, "%f", &dummyVal))
            {
            	fprintf(stderr, "Could not parse intervalDuration of \"%s\", not a floating point number\n", intervalDuration);
            }
       	}
        channels = new examon_mqtt_path(examonHost, configChannel);


	    mosqpp::lib_init();

	    printf("calling non-async connect\n");
		connect( connectHost, port, keepalive);

		isAlive = true;
        pthread_t thread_id; /* TODO: store the thread id */
        pthread_create(&thread_id, NULL, &pthread_loop, this);
	}
	~examon_sync_plugin()
	{
		/*  close connection to mosquitto */
		isAlive = false;
		isConnected = false;
		disconnect();
		mosqpp::lib_cleanup();

	}
	bool alive() { return isAlive; }
	bool connected() { return isConnected; }
	static void* pthread_loop(void* arg)
	{
		examon_sync_plugin* myObj = (examon_sync_plugin*) arg;

	    myObj->loop_forever();

	}
	void subscribe_the_metrics()
	{
		for(int i = 0; i < metricIds.size(); ++i)
		{
			/* DONE: Make it multi-socket compatible (replace "0" below with "+",
			 *         also implement logic for receiving multiple values at once */
            printf("Trying to subscribe to %s.\n", metrics[i].getFullTopic().c_str());
            subscribe(NULL, metrics[i].getFullTopic().c_str());
		}
	}
	void updateErgUnit(double ergUnit)
	{
		for(int i = 0; i < metrics.size(); ++i)
		{
			metrics[i].setErgUnit(ergUnit);
		}
	}
	void on_connect(int rc)
	{
		printf("Connected with code %d.\n", rc);
		if(rc == 0){

			isConnected = true;

			if(intervalDuration)
			{
		        char* intervalStr = (char *) malloc(100);
		        strcpy(intervalStr, "-s ");
		        strncpy(intervalStr + 3, intervalDuration, 95);
			    printf("Trying to publish to channel \"%s\", value \"%s\".\n", channels->topicCmd().c_str(), intervalStr);
			    publish(NULL, channels->topicCmd().c_str(), strlen(intervalStr), intervalStr);
			    free(intervalStr);
			}


			if(trackErgUnit)
			{
			    printf("Trying to subscribe to %s.\n", channels->topicErgUnits().c_str());
    			subscribe(NULL, channels->topicErgUnits().c_str());
			}

		}
	}
	void on_message(const struct mosquitto_message *message)
	{
		if(channels->startsWithTopicBase(message->topic))
		{
			if(-1 == ergUnit && channels->isErgUnits(message->topic))
			{
				if(1 == sscanf((char *) message->payload, "%lf;%*f", &ergUnit))
				{
					unsubscribe(NULL, channels->topicErgUnits().c_str());
					updateErgUnit(ergUnit);
				}
			}

			if(0 == strncmp("/data/", message->topic + channels->topicBase().length(), 6))
			{
				for(int i = 0; i < metrics.size(); ++i)
				{

					if(metrics[i].metricMatches(message->topic))
					{
						metrics[i].handleMessage(message->topic, (char*) message->payload, message->payloadlen);
						//Don't break here, allow for multiple handling
					}
				}
			}
		}
	}
	void on_subscribe(int mid, int qos_count, const int *granted_qos)
	{
		printf("Subscription succeeded.\n");
	}
	void on_unsubscribe(int mid)
	{
		printf("unsubscribed.\n");
	}

    /* return matching properties */
	std::vector<scorep::plugin::metric_property> get_metric_properties(const std::string& metric_parse)
	{
	  //DEBUG
      std::cout << "get_metric_properties(\"" << metric_parse << "\")"<< std::endl;
      /* valid metrics:
       * tsc
       * temp_pkg
       * erg_dram  -> also track erg_units
       * erg_cores -> also track erg_units
       * erg_pkg   -> also track erg_units
       * erg_units
       * freq_ref
       * C2
       * C3
       * C6
       * uclk
       */


	  std::vector<scorep::plugin::metric_property> foundMatches = std::vector<scorep::plugin::metric_property>();

	  // revoke paths like cpu/0/+
	  //                or cpu/+/+
	  //                or +/+/+
      std::string metricBasename = basename((char*) metric_parse.c_str());
      if(NULL != strchr(metricBasename.c_str(), '+')
      || NULL != strchr(metricBasename.c_str(), '#'))
      {
    	  fprintf(stderr, "Can't allow metric \"%s\", don't know how to accumulate it. Only homogenetic path endings allowed.\n", metricBasename.c_str());
    	  return foundMatches;
      } else
      {
		  char* buf = (char*) malloc(metric_parse.length() + 17);
		  sprintf(buf, "Examon metric \"%s\"", metric_parse.c_str());
		  buf[metric_parse.length() + 16] = '\0';
		  char* unit = (char*) "";
		  EXAMON_METRIC_TYPE metricType = parseMetricType((char*) metricBasename.c_str());
		  switch(metricType)
		  {
		  case EXAMON_METRIC_TYPE::TEMPERATURE:
			  unit = (char*) "C";
			  break;
		  case EXAMON_METRIC_TYPE::ENERGY:
			  trackErgUnit = true;
			  unit = (char*) "J";
			  break;
		  case EXAMON_METRIC_TYPE::FREQUENCY:
			  unit = (char*) "Hz";
			  break;
		  }

		  scorep::plugin::metric_property prop = scorep::plugin::metric_property(metric_parse, buf, unit);
		  switch(metricType)
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
			  prop.accumulated_start();
			  break;
		  }
		  prop.value_double();
		  prop.decimal();
		  foundMatches.push_back(prop);

		  free(buf);


		  return foundMatches;
      }
	}
/* receive metrics here, register them internally with a std::int32_t, which will be later used by score-p to reference the metric here */
	int32_t add_metric(const std::string& metric_name)
	{
      //DEBUG
	  std::cout << "add_metric(\"" << metric_name << "\")"<< std::endl;

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
	template <class Proxy> bool get_optional_value(std::int32_t id, Proxy& p)
	{
	    if(isConnected)
	    {
	    	int index = id - 1;
	    	if(0 <= index && index < metricIds.size() && metricIds[index] == id)
	    	{
	    		if(metrics[index].hasValue())
	    		{
	    			p.store(metrics[index].getLatestValue());
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
		if(!subscribedToMetrics)
		{
            subscribe_the_metrics();
            subscribedToMetrics = true;
		}
	}
};



SCOREP_METRIC_PLUGIN_CLASS(examon_sync_plugin, "examon_sync")



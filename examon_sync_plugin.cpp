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
	std::vector<double> metricValues;
	std::vector<double> metricDeltas;
	std::vector<bool> metricIsEnergy;

	std::vector<bool> metricReportDelta;
	std::vector<double> metricTimestamps;
	std::vector<double> metricElapsed;
	std::vector<std::int64_t> metricIterations;
	std::int32_t put_metric(std::string metric)
	{
		std::int32_t biggestInt = 0;
		if(0 < metricIds.size()) biggestInt = metricIds[metricIds.size() - 1];

		std::int32_t newId = biggestInt + 1;
        metricIds.push_back(newId);
        metricNames.push_back(metric);
        char* metricBasename = basename((char*) metric.c_str());
        bool reportDelta = true;
        if(0 == strncmp("temp", metricBasename, 4)
        || 0 == strncmp("freq", metricBasename, 4))
        {
        	reportDelta = false;
        }
        metricReportDelta.push_back(reportDelta);
        bool isEnergy = false;
        if(0 == strncmp("erg", metricBasename, 3)
        && 0 != strncmp("erg_units", metricBasename, 9))
        {
        	isEnergy = true;
        }
        metricIsEnergy.push_back(isEnergy);
        metricValues.push_back(-1);
        metricDeltas.push_back(0);
        metricTimestamps.push_back(0);
        metricElapsed.push_back(0);
        metricIterations.push_back(0);

        return newId;
	}
	std::string topicMaskPrefix = "/node/";
	std::string topicMaskPostfix = "/plugin/pmu_pub/chnl";
	std::string topicBase = "";
	std::string topicCmd = "";
	std::string topicErgUnits = "";
	char* hostname = NULL;
	char* examonHost = NULL;
	char* connectHost = NULL;
	char* configChannel = NULL;
	char* intervalDuration = NULL;
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
	    connectHost =      getenv("SCOREP_METRIC_EXAMON_SYNC_PLUGIN_BROKER");
	    examonHost =       getenv("SCOREP_METRIC_EXAMON_SYNC_PLUGIN_EXAMON_HOST");
	    configChannel =    getenv("SCOREP_METRIC_EXAMON_SYNC_PLUGIN_CHANNEL");
	    intervalDuration = getenv("SCOREP_METRIC_EXAMON_SYNC_PLUGIN_INTERVAL");
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

	    topicBase = configChannel + topicMaskPrefix + examonHost + topicMaskPostfix;
	    topicCmd = topicBase + "/cmd";
	    topicErgUnits = topicBase + "/data/cpu/0/erg_units";


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
        char* subTopic = (char*) malloc(1001);
		for(int i = 0; i < metricIds.size(); ++i)
		{
			/* TODO: Make it multi-socket compatible (replace "0" below with "+",
			 *         also implement logic for receiving multiple values at once */
            sprintf(subTopic, "%s/data/%s", topicBase.c_str(), metricNames[i].c_str());
            printf("Trying to subscribe to %s.\n", subTopic);
            subscribe(NULL, subTopic);
		}
		free(subTopic);
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
			    printf("Trying to publish to channel \"%s\", value \"%s\".\n", topicCmd.c_str(), intervalStr);
			    publish(NULL, topicCmd.c_str(), strlen(intervalStr), intervalStr);
			    free(intervalStr);
			}


			/* Disregard desired metrics in metricNames */
			/* TODO: Do heed the specified metrics */

			//printf("Trying to subscribe to %s.\n", topicErgPkg.c_str());
			//subscribe(NULL, topicErgPkg.c_str());
			if(trackErgUnit)
			{
			    printf("Trying to subscribe to %s.\n", topicErgUnits.c_str());
    			subscribe(NULL, topicErgUnits.c_str());
			}

		}
	}
	void on_message(const struct mosquitto_message *message)
	{
		if(0 == strncmp(message->topic, topicBase.c_str(), topicBase.length()))
		{
			if(-1 == ergUnit && 0 == strcmp(topicErgUnits.c_str(), message->topic))
			{
				if(1 == sscanf((char *) message->payload, "%lf;%*f", &ergUnit))
				{
					unsubscribe(NULL, topicErgUnits.c_str());
				}
			}

			if(0 == strncmp("/data/", message->topic + topicBase.length(), 6))
			{
				char* topicName = message->topic + topicBase.length() + 6;

				for(int i = 0; i < metricNames.size(); ++i)
				{
					//TODO: Write metric matching for paths like cpu/+/erg_pkg
					//                                        or +/+/tsc
					//        however revoke paths like cpu/0/+
					//                               or cpu/+/+
					//                               or +/+/+
					//use mosqpp::topic_matches_sub(string mask_of_topic, string actual_topic, bool* result) here
					bool topicMatches = false;
					mosqpp::topic_matches_sub(metricNames[i].c_str(), topicName, &topicMatches);
					if(topicMatches)
					{
						// - read out values from payload
						double readValue = -1;
						double readTimestamp = -1;
						int valuesRead = sscanf((char*) message->payload, "%lf;%lf", &readValue, &readTimestamp);
						if( 2 == valuesRead )
						{
							// - calculate delta
							bool reportDelta = -1 == metricValues[i];
							double delta = readValue - metricValues[i];
							metricDeltas[i] = delta;
							delta = readTimestamp - metricTimestamps[i];
							metricElapsed[i] = delta;

							// - put value in metricValues
							metricValues[i] = readValue;
							metricTimestamps[i] = readTimestamp;
							++metricIterations[i];
							printf("read metric %9s: %0.4lf, metricValues[%d]= %0.4lf\n", metricNames[i].c_str(), readValue, i, metricValues[i]);
						}
						break;
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
      std::string metricBasename = basename(metric_parse.c_str());

      std::vector<scorep::plugin::metric_property> foundMatches = std::vector<scorep::plugin::metric_property>();

	  char* buf = (char*) malloc(metric_parse.length() + 17);
	  sprintf(buf, "Examon metric \"%s\"", metric_parse.c_str());
	  buf[metric_parse.length() + 16] = '\0';
	  bool isAbsolute = false;
	  bool accumulatedStart = true;
	  char* unit = (char*) "";
	  if(0 == strncmp("temp", metricBasename.c_str(), 4))
	  {
		  unit = (char*) "C";
		  isAbsolute = true;
	  } else if(0 == strncmp("erg", metricBasename.c_str(), 3))
	  {
		  if(0 != strncmp("erg_units", metricBasename.c_str(), 9))
		  {
			  unit = (char*) "J";
			  trackErgUnit = true;
			  accumulatedStart = false;
		  }
	  } else if(0 == strncmp("freq", metricBasename.c_str(), 4))
	  {
		  unit = (char*) "Hz";
		  isAbsolute = true;
	  }

	  scorep::plugin::metric_property prop = scorep::plugin::metric_property(metric_parse, buf, unit);
	  if(isAbsolute)
	  {
		  prop.absolute_last();
	  } else
	  {
		  if(accumulatedStart)
		  {
			  prop.accumulated_start();
		  } else
		  {
			  prop.accumulated_last();
		  }
	  }
	  prop.value_double();
	  prop.decimal();
	  foundMatches.push_back(prop);

	  free(buf);


      return foundMatches;
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
	    		if(-1 < metricValues[index])
	    		{
    			    if(metricIsEnergy[index]) {
    			    	if(-1 < ergUnit && 1 < metricIterations[index])
    			    	{
    			    		double calculatedMetric = metricValues[index] / metricElapsed[index] / ergUnit;
    			    		printf("e %8s %0.4lf = %0.4lf / %0.4lf / %0.4lf\n", metricNames[index].c_str(), calculatedMetric, metricValues[index], metricElapsed[index], ergUnit);
    			    		p.store(calculatedMetric);
    			    		return true;
    			    	}
    			    } else
    			    {
    			    	printf("o %8s %0.4lf\n", metricNames[index].c_str(), metricValues[index]);
    				    p.store(metricValues[index]);
    				    return true;
    			    }

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
      /* TODO: Do something */
		printf("synchronize() called\n");
		if(!subscribedToMetrics)
		{
            subscribe_the_metrics();
            subscribedToMetrics = true;
		}
	}
};



SCOREP_METRIC_PLUGIN_CLASS(examon_sync_plugin, "examon_sync")



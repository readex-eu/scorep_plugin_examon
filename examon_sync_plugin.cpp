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
#include <scorep/plugin/plugin.hpp>
#include <scorep/plugin/util/matcher.hpp>
extern "C"
{
#include <mosquitto.h>
#include <client_shared.h>
}

namespace spp = scorep::plugin::policy;



class examon_sync_plugin
: public scorep::plugin::base<examon_sync_plugin, spp::per_host, spp::sync,
                              spp::scorep_clock, spp::synchronize>
{
private:
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
	/* TODO: write contstructor and destructor */
	examon_sync_plugin()
	{
		/* TODO: Do constructor stuff */
		/* TODO: initiate connection to mosquitto */

		struct mosq_config cfg;
		memset(&cfg, 0, sizeof(struct mosq_config));

/* code from mosquitto_sub
		rc = client_config_load(&cfg, CLIENT_SUB, argc, argv);
		if(rc){
			client_config_cleanup(&cfg);
		}
		mosquitto_lib_init();

		if(client_id_generate(&cfg, "mosqsub")){
			return 1;
		}

		mosq = mosquitto_new(cfg.id, cfg.clean_session, &cfg);
		if(!mosq){
			switch(errno){
				case ENOMEM:
					if(!cfg.quiet) fprintf(stderr, "Error: Out of memory.\n");
					break;
				case EINVAL:
					if(!cfg.quiet) fprintf(stderr, "Error: Invalid id and/or clean_session.\n");
					break;
			}
			mosquitto_lib_cleanup();
			return 1;
		}
		if(client_opts_set(mosq, &cfg)){
			return 1;
		}

		if(cfg.debug){
			mosquitto_log_callback_set(mosq, my_log_callback);
			mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
		}
		mosquitto_connect_with_flags_callback_set(mosq, my_connect_callback);
		mosquitto_message_callback_set(mosq, my_message_callback);

		rc = client_connect(mosq, &cfg);
		if(rc) return rc;

	#ifndef WIN32
		sigact.sa_handler = my_signal_handler;
		sigemptyset(&sigact.sa_mask);
		sigact.sa_flags = 0;

		if(sigaction(SIGALRM, &sigact, NULL) == -1){
			perror("sigaction");
			return 1;
		}

		if(cfg.timeout){
			alarm(cfg.timeout);
		}
	#endif

		rc = mosquitto_loop_forever(mosq, -1, 1);
*/


	}
	~examon_sync_plugin()
	{
		/* TODO: Do destructor stuff */
		/* TODO: close connection to mosquitto */
		/*
		mosquitto_destroy(mosq);
		mosquitto_lib_cleanup();
		*/
	}
    /* return matching properties */
	std::vector<scorep::plugin::metric_property> get_metric_properties(const std::string& metric_parse)
	{
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



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
#include <scorep/plugin/plugin.hpp>

namespace spp = scorep::plugin::policy;


class examon_sync_metric {
private:
	std::string mname;
	std::string mfull_name;
	std::string mquantity;
	int msensor;
	int mnode;
public:
    /* dummy constructor */
	examon_sync_metric(const std::string full_name, const std::string name, const int sensor, const int node, const std::string quantity):
		mfull_name(full_name), mname(name), msensor(sensor), mnode(node), mquantity(quantity)
    {
    }

    /* getter functions */
    std::string name() const
    {
        return mname;
    }

    std::string full_name() const
    {
        return mfull_name;
    }

    int sensor() const
    {
        return msensor;
    }

    int node() const
    {
        return mnode;
    }

    std::string quantity() const
    {
        return mquantity;
    }

};

template <typename T, typename Policies>
using examon_sync_object_id = spp::object_id<examon_sync_metric, T, Policies>;

class examon_sync_plugin
: public scorep::plugin::base<examon_sync_plugin, spp::per_thread, spp::sync_strict,
                              spp::scorep_clock, spp::synchronize, examon_sync_object_id>
{
	/* TODO: write contstructor and destructor */
	examon_sync_plugin()
	{
		/* TODO: Do constructor stuff */
	}
	~examon_sync_plugin()
	{
		/* TODO: Do destructor stuff */
	}
	void add_metric(x86_energy_sync_metric& m);
	 /** Will be called for every event in by the measurement environment.
	   * You may or may not give it a value here.
	   *
	   * @param m contains the sored metric informations
	   * @param proxy get and save the results
	   *
	   * NOTE: In this implemenation we use a few assumptions:
	   * * scorep calls at every event all metrics
	   * * this metrics are called everytime in the same order
	   **/
	template <typename P>
	void get_current_value(x86_energy_sync_metric& m, P& proxy);
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
	void synchronize(bool is_responsible, SCOREP_MetricSynchronizationMode sync_mode);
};

SCOREP_METRIC_PLUGIN_CLASS(examon_sync_plugin, "examon_sync")



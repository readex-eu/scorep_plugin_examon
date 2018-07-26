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

class x86_energy_sync_plugin
: public scorep::plugin::base<x86_energy_sync_plugin, spp::per_thread, spp::sync_strict,
                              spp::scorep_clock, spp::synchronize, x86_energy_sync_object_id>
{
};



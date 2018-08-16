scorep_plugin_examon
======================
Connects to EXAMON via MQTT and reads out tsc data to figure out how many Joules are being used by the processor

Dependencies
------------
* **[Score-P](https://www.vi-hps.org/projects/score-p/)**
* some server on which [EXAMON](https://github.com/EEESlab/examon) is running 
* some server on which [mosquitto](https://mosquitto.org/)(i.e. a MQTT-Broker) is running, see [this launchpad](https://launchpad.net/~mosquitto-dev/+archive/ubuntu/mosquitto-ppa) if you want a more recent version to run on an Ubuntu

other dependencies are included in this Git-Repository ([libmosquitto](https://github.com/eclipse/mosquitto), [scorep_plugin_cxx_wrapper](https://github.com/score-p/scorep_plugin_cxx_wrapper)), they should be fulfilled when running cmake and make.

License
-------
BSD-3

Building
--------
use CMake:
```
mkdir build
cd build;
cmake ../
make
```

Running
-------
1. configure examon to report metrics to the mosquitto server
2. compile your application with Score-P
3. set environment variable `LD_LIBRARY_PATH`
4. tell Score-P that you want a trace with `SCOREP_ENABLE_TRACING=true`
5. specify which plugin you want to run by setting environment variable SCOREP_METRIC_PLUGINS (either `examon_sync_plugin` or `examon_async_plugin`)
6. specify which metrics you want to be included into the trace with either `SCOREP_METRIC_EXAMON_SYNC_PLUGIN` or `SCOREP_METRIC_EXAMON_ASYNC_PLUGIN` environment variable.  They take a comma separated list
7. run your program

Configuration
-------------
In case you don't have scorep, mosquitto and examon running on the same system you might want to set the following *environment variables*:
<table>
    <tr>
        <th>environment variable</th><th>default</th><th>purpose</th>
    </tr>
    <tr>
        <td>SCOREP_METRIC_EXAMON_SYNC_PLUGIN</td><td><i>none</i></td><td>specify metrics for **sync plugin**</td>
    </tr>
    <tr>
        <td>SCOREP_METRIC_EXAMON_ASYNC_PLUGIN</td><td><i>none</i></td><td>specify metrics for **async plugin**</td>
    </tr>
    <tr>
        <td>SCOREP_METRIC_EXAMON_ASYNC_PLUGIN_BROKER / SCOREP_METRIC_EXAMON_SYNC_PLUGIN_BROKER</td><td>localhost</td><td>the address of the MQTT broker</td>
    </tr>
    <tr>
        <td>SCOREP_METRIC_EXAMON_ASYNC_PLUGIN_EXAMON_HOST / SCOREP_METRIC_EXAMON_SYNC_PLUGIN_EXAMON_HOST</td><td><i>gethostname()</i></td><td>the hostname which examon uses in it's topics</td>
    </tr>
    <tr>
        <td>SCOREP_METRIC_EXAMON_ASYNC_PLUGIN_CHANNEL / SCOREP_METRIC_EXAMON_SYNC_PLUGIN_CHANNEL</td><td>org/antarex/cluster/testcluster</td><td>the default channel configured in examon's `pmu_pub.conf` key `topic`</td>
    </tr>
    <tr>
        <td>SCOREP_METRIC_EXAMON_ASYNC_PLUGIN_INTERVAL / SCOREP_METRIC_EXAMON_SYNC_PLUGIN_INTERVAL</td><td><i>none</i></td><td>used to tell examon via command channel to adapt this delay between readouts, unit: seconds</td>
    </tr>
</table>

Example usage
-------------
See Score-P documentation on how to compile your software [with instrumentation](https://silc.zih.tu-dresden.de/scorep-current/quickstart.html#quick_instrumentation)

```
# required if plugin was not installed via make install
export LD_LIBRARY_PATH="/my/path/to/the/repository/scorep_plugin_examon/build"

export SCOREP_ENABLE_TRACING=true
export SCOREP_ENABLE_PROFILING=false
export SCOREP_TOTAL_MEMORY=1000M

export SCOREP_METRIC_PLUGINS="examon_async_plugin"
export SCOREP_METRIC_EXAMON_ASYNC_PLUGIN="cpu/0/tsc,core/+/temp;MAX,cpu/0/erg_pkg,cpu/0/erg_dram"

./my-program-which-was-compiled-using-scorep.bin

```

The above shell code should run your program and produce a trace, which you then may inspect with a tool like [VAMPIR](https://vampir.eu/), therein choose the button "Add Counter Data Timeline" (should be fourth from the left) to see the recorded stats.

Metric Syntax
-------------
Note the above example where metrics like `cpu/0/tsc` and `core/+/temp;MAX` are specified.
You may specify an arbitrary topic path that *does not end* with a MQTT wildcard (e.g. `+` or `#`). If you specify a wildcard within the topic path like above, MQTT will match several topics to it e.g. `core/0/temp`, `core/1/temp`, `core/2/temp` etc., in that case you might want to specify an **ACUMMULATION_STRATEGY**, which will tell this plugin how to combine the multiple metrics into one value.  Currently the following acummulation strategies are available:
* MINIMUM
* MAXIMUM
* SUMMATION
* AVERAGE (default)

(they can be abbreviated to `min`,`max`,`sum`,`avg`)



scorep_plugin_examon
======================
Connects to EXAMON via MQTT and reads out any kind of data, e.g., blade energy consumption

Dependencies
------------
* **[Score-P](https://www.vi-hps.org/projects/score-p/)**
* [CMake](https://cmake.org/) version 3.8 or higher
* make
* some server on which [EXAMON](https://github.com/EEESlab/examon) is running 
* some server on which [mosquitto](https://mosquitto.org/)(i.e. a MQTT-Broker) is running, see [this launchpad](https://launchpad.net/~mosquitto-dev/+archive/ubuntu/mosquitto-ppa) if you want a more recent version to run on an Ubuntu

other dependencies are included in this Git-Repository ([libmosquitto](https://github.com/eclipse/mosquitto), [scorep_plugin_cxx_wrapper](https://github.com/score-p/scorep_plugin_cxx_wrapper)), they should be fulfilled when running cmake and make.

License
-------
BSD-3

Building
--------
Use CMake:
```
mkdir build
cd build;
cmake ../
make
```
### CMake Options
* ENABLE_MPI (default: OFF)

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
        <td>SCOREP_METRIC_EXAMON_SYNC_PLUGIN</td><td><i>none</i></td><td>specify metrics for <b>sync plugin</b></td>
    </tr>
    <tr>
        <td>SCOREP_METRIC_EXAMON_ASYNC_PLUGIN</td><td><i>none</i></td><td>specify metrics for <b>async plugin</b></td>
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
        <td>SCOREP_METRIC_EXAMON_ASYNC_PLUGIN_INTERVAL / SCOREP_METRIC_EXAMON_SYNC_PLUGIN_INTERVAL</td><td><i>none</i></td><td>used to tell examon via command channel to adapt this delay between readouts, unit: seconds (you may specify a floating point number)</td>
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
Using either `SCOREP_METRIC_EXAMON_SYNC_PLUGIN` or `SCOREP_METRIC_EXAMON_ASYNC_PLUGIN` you can specify the metrics you want this plugin to report from examon.  For an example look ath the bash code above, which uses MQTT topics with cpu and core, like  `cpu/0/tsc` and `core/+/temp;MAX`.


### Structure
```
parameter = <metric-specification> [',' <metric-specification> ...]
metric-specification = <path> '/' <basename> [';' <option> ...]
path = any character except ','
basename = any character except ',' '/' or ';'
option = <accumulation-strategy> | <output-datatype> | <scaling_multiplicator>
accumulation-strategy = 'MIN' | 'MAX' | 'AVG' | 'SUM'
output-datatype = 'DOUBLE' | 'INT32' | 'UINT32' | 'INT64' | 'UINT64'
scaling_multiplicator = 's=' <floating point number>
```

**NOTE** when specifying multiple *accumulation-strategy* or *output-datatype* options on a single *metric-specification* the last one applies. E.g. `cpu/+/erg_dram;SUM;MAX;AVG;MIN;UINT64;INT32;DOUBLE` would be accumulated using the `MIN` strategy, and be reported as a `double` value to Score-P.

### Using Wildcards
MQTT understands wildcards in the *path*. `+` for any amount of characters excluding `/`. `#` for any amount of characters.

Do note that you may specify an arbitrary topic path that *does not end* with a MQTT wildcard.

### Accumulation-strategy, Matching multiple metrics at once
If you specify a wildcard within the topic path like in `core/+/temp`, MQTT will match several topics to it e.g. `core/0/temp`, `core/1/temp`, `core/2/temp` etc., in that case you might want to specify an **ACUMMULATION_STRATEGY**, which will tell this plugin how to combine the multiple metrics into one value.  Currently the following acummulation strategies are available:
* `MIN`, select the smallest received value for each time index
* `MAX`, select the largest received value for each time index
* `SUM`, for each time index summate all received values
* `AVG` (default), for each time index calculate the average value

### Output-datatype
You may also specify an output datatype. The metrics will be parsed to the specified datatype before being reported to Score-P.
Available Datatypes:
* `DOUBLE` (default)
* `INT32`
* `UINT32`
* `INT64`
* `UINT64`

### Path
The path you specify is only the very end of the Examon/MQTT path. If you look at the raw output of Examon using `mosquitto_sub -v -t "#"` you might receive an output like this:
```
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data CK
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/cpu/0/tsc 6514034410521054;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/cpu/0/temp_pkg 31;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/cpu/0/erg_dram 1347318602;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/cpu/0/erg_cores 493158;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/cpu/0/erg_pkg 4176111766;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/cpu/0/erg_units 658947;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/cpu/0/freq_ref 3400000000.000000;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/cpu/0/C2 6458454251168686;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/cpu/0/C3 0;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/cpu/0/C6 0;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/tsc 6514034410281892;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/temp 30;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/instr 1348379485276;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/clk_curr 3838437806472;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/clk_ref 3962611107782;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/C3 528063812196;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/C6 144192590938;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/aperf 5629924683127;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/mperf 5726500245551;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/UOPS_RETIRED.RETIRE_SLOTS 140737561474785;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/ICACHE.MISSES 140737488408801;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/LONGEST_LAT_CACHE.MISS 140737489632218;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/MEM_LOAD_UOPS_L3_HIT_RETIRED.XSNP_NONE 5726500245551;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/BR_MISP_RETIRED.ALL_BRANCHES 5726500245551;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/UOPS_ISSUED.ANY 65535;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/IDQ_UOPS_NOT_DELIVERED.CORE 5726500245551;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/0/INT_MISC.RECOVERY_CYCLES 5726500245551;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/tsc 6514034410435230;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/temp 29;1534429383.000
...
```
This plugin will assemble the data path using the following schema (use `SYNC` or `ASYNC` in the variable name depending on which you are using):
```
*SCOREP_METRIC_EXAMON_ASYNC_PLUGIN_CHANNEL* '/node/' *SCOREP_METRIC_EXAMON_ASYNC_PLUGIN_EXAMON_HOST* '/plugin/pmu_pub/chnl/data/'
```
To which your specified path will be appended. The resulting string will be used in an invocation of MQTT's subscribe() to receive metric values from Examon/MQTT.

Thus, if you would want to listen to `foo/node/supercomputer/plugin/pmu_pub/chnl/data/core/0/temp`, you might specify the following environment variables:
```
export SCOREP_METRIC_PLUGINS="examon_async_plugin"
export SCOREP_METRIC_EXAMON_ASYNC_PLUGIN_EXAMON_HOST="supercomputer"
export SCOREP_METRIC_EXAMON_ASYNC_PLUGIN_CHANNEL="foo"
export SCOREP_METRIC_EXAMON_ASYNC_PLUGIN="core/0/temp"
```

**Note** that Examon provides metrics per *cpu socket* and per *core*. E.g. metric `erg_pkg` is only available on a per cpu socket basis.

**Note** Metrics which's *basename* begins with `erg` (except erg_units) will trigger this plugin to multiply their output value with the factor [derived from](https://github.com/Quimoniz/scorep_plugin_examon/blob/182ab8a684ce20b19cf4a33ac404990148c726d2/examon_async_plugin.cpp#L251) cpu/0/erg_units to arrive at the correct Joule value.


### Examples
list a lot of metrics from cpu socket 0
`cpu/0/erg_pkg,cpu/0/erg_dram,cpu/0/tsc,cpu/0/temp_pkg,cpu/0/erg_cores,cpu/0/freq_ref,cpu/0/C2,cpu/0/C3,cpu/0/C6,cpu/0/uclk`

compare average temperature of all cores with maximum temperature:
`core/+/temp;AVG,core/+/temp;MAX`

look at temperature and energy consumption
`cpu/+/erg_pkg;AVG,core/+/temp;MAX`

Bugreport
---------
I would be thankfull for any bug report.


A note on configuring Examon
----------------------------
Example host_whitelist (replace the last name with the name from `/etc/hostname`)
```
[BROKER:] 127.0.0.1 1883
HOSTNAME_OF_THE_SYSTEM_EXAMON_IS_RUNNING_ON
```

Excerpt from pmu_pub.conf
```
[MQTT]
brokerHost = 127.0.0.1
brokerPort = 1883
topic = org/antarex/cluster/testcluster
qos = 0

[Daemon]
dT = 1
daemonize = False
...
```
Wherein `dT` may be a floating point denoting the delay between readouts of RAPL/MSR

**Note** I could not get Examon running with daemonization, therefore `daemonize` is disabled in my configuration.

**Note** On Kernels since Version 4 (April 2015) it is necessary to [enable RDPMC](https://github.com/EEESlab/examon#enable-rdpmc-instruction-in-kernels-4x), otherwise there is a segfault.


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
Look at the example above, which uses MQTT topics with cpu and core, like  `cpu/0/tsc` and `core/+/temp;MAX`.

### Structure
```
parameter = <metric-specification> [',' <metric-specification> ...]
metric-specification = <path> '/' <basename> [';' <option> ...]
path = any character except ','
basename = any character except ',' '/' or ';'
option = <accumulation-strategy> | <output-datatype>
accumulation-strategy = 'MIN' | 'MAX' | 'AVG' | 'SUM'
output-datatype = 'DOUBLE' | 'INT32' | 'UINT32' | 'INT64' | 'UINT64'
```

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
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/instr 1589895584919;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/clk_curr 3615841871577;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/clk_ref 3674617169822;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/C3 531797193722;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/C6 116814376844;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/aperf 4424887717554;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/mperf 4514209121473;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/UOPS_RETIRED.RETIRE_SLOTS 140737566371814;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/ICACHE.MISSES 140737488409804;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/LONGEST_LAT_CACHE.MISS 140737489096614;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/MEM_LOAD_UOPS_L3_HIT_RETIRED.XSNP_NONE 4514209121473;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/BR_MISP_RETIRED.ALL_BRANCHES 4514209121473;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/UOPS_ISSUED.ANY 4514209121473;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/IDQ_UOPS_NOT_DELIVERED.CORE 4514209121473;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/1/INT_MISC.RECOVERY_CYCLES 4514209121473;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/2/tsc 6514034410521054;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/2/temp 26;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/2/instr 948329172612;1534429383.000
org/antarex/cluster/testcluster/node/mabus/plugin/pmu_pub/chnl/data/core/2/clk_curr 3437216132129;1534429383.000
...
```
This plugin will assemble the data path using the following variables:
*SCOREP_METRIC_EXAMON_ASYNC_PLUGIN_CHANNEL* '/node/' *SCOREP_METRIC_EXAMON_ASYNC_PLUGIN_EXAMON_HOST* '/plugin/pmu_pub/chnl/data/'

To which your specified path will be appended. The resulting string will be used in an invocation of MQTT's subscribe() to receive metric values from Examon/MQTT.

TODO: difference between cpu/ path and core/ path

### Examples
list a lot of cpu metrics
cpu/0/erg_pkg,cpu/0/erg_dram,cpu/0/tsc,cpu/0/temp_pkg,cpu/0/erg_cores,cpu/0/freq_ref,cpu/0/C2,cpu/0/C3,cpu/0/C6,cpu/0/uclk

TODO: elaborate a few more examples

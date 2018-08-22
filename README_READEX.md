READEX specifics
=============

Compilation
-------------
The plugin *must* be compiled with `ENABLE_MPI`

Usage during Design Time Analysis with Periscope
-------------
For READEX purposes, we use the examon_sync_plugin, which enables us to attribute energy to specific regions. To use the plugin, it must be enabled during runs with Periscope. To do so, use the following environment variables:
```
export SCOREP_METRIC_PLUGINS=examon_sync_plugin
export SCOREP_METRIC_PLUGINS_SEP=";"
export SCOREP_METRIC_EXAMON_SYNC_PLUGIN_BROKER=<MQTT broker address>
#
# You can use this option, if your blade counter does not use the systems hostname but something else;
export SCOREP_METRIC_EXAMON_SYNC_PLUGIN_EXAMON_HOST=<hostname override&>
#
# the default is org/antarex/cluster/testcluster
export SCOREP_METRIC_EXAMON_SYNC_PLUGIN_CHANNEL=<the default channel configured in examon's pmu_pub.conf key "topic">
#
# lower means more overhead, but higher granularity.
# The granularity is also limited from the EXAMON publisher, which provides the data!
export SCOREP_METRIC_EXAMON_SYNC_PLUGIN_INTERVAL=<Readout delay in seconds (e.g., 0.001 for 1 ms)>
#
export SCOREP_METRIC_EXAMON_SYNC_PLUGIN="EXAMON/BLADE/E"
#
# the whole counter name used with MQTT is:
# $SCOREP_METRIC_EXAMON_SYNC_PLUGIN_EXAMON_HOST/$SCOREP_METRIC_EXAMON_SYNC_PLUGIN_CHANNEL/$SCOREP_METRIC_EXAMON_SYNC_PLUGIN_READEX_BLADE
export SCOREP_METRIC_EXAMON_SYNC_PLUGIN_READEX_BLADE="<The systems blade counter>INT64 s=0.001";
```
Furthermore, the metric source definition of your `readex_config.xml` should look like this:
```
<metricPlugin>
  <name>examon_sync_plugin</name>
</metricPlugin>
<metrics>
  <node_energy>EXAMON/BLADE/E</node_energy>
</metrics>
```

/*
 * examon_mqtt_path.c
 *
 *  Created on: 09.08.2018
 *      Author: jitschin
 */

extern "C" {
#include <string.h>
}

#include <string>

/**
 * A class to store and assemble the paths to MQTT/Examon topics
 */
class examon_mqtt_path
{
private:
    std::string mask_prefix = "/node/";  /**< the first part of the mask applied to assemble the base name */
    std::string mask_postfix = "/plugin/pmu_pub/chnl";  /**< the last part of the mask applied to assemble the base name */
    std::string base = "";  /**< the topic base for this MQTT/Examon connection */
    std::string cmd = "";  /**< the cmd topic which to use to address examon */
    std::string erg_units = "";  /**< the erg_units topic which to listen to to get the energy unit */

public:
    /**
     * Assembles the base path, the cmd path and the erg_units path
     * @param examon_host the host on which examon is running
     * @param config_channel the configured channel with which examon is supposed to be running
     */
    examon_mqtt_path(char* examon_host, char* config_channel)
    {
        base = config_channel + mask_prefix + examon_host + mask_postfix;
        cmd = base + "/cmd";
        erg_units = base + "/data/cpu/0/erg_units";
    }
    /**
     * Return the base topic for this connection
     */
    std::string topic_base()
    {
        return base;
    }
    /**
     * Return the erg_units topic for this connection
     */
    std::string topic_erg_units()
    {
        return erg_units;
    }
    /**
     * Return the cmd topic for this connection
     */
    std::string topic_cmd()
    {
        return cmd;
    }
    /**
     * Assemble the full topic for a given data topic (e.g. "cpu/+/tsc")
     * @param subtopic the short topic for the specific Examon-metric
     */
    std::string get_data_topic(std::string subtopic)
    {
        return base + "/data/" + subtopic;
    }
    /**
     * check whether a given topic matches with the erg_units topic
     * @param comparision_topic the raw, full topic without wildcards against which to match to
     * @see starts_with_topic_base()
     */
    bool is_erg_units(char* comparison_topic)
    {
        if (NULL != comparison_topic && 0 == erg_units.compare(comparison_topic))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    /**
     * check if the given topic starts with this connection's base topic
     * @param comparison_topic the long topic which should be checked if it starts with the base topic
     */
    bool starts_with_topic_base(char* comparison_topic)
    {
        if (NULL != comparison_topic && 0 == strncmp(comparison_topic, base.c_str(), base.length()))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
};

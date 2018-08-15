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

class examon_mqtt_path
{
private:
    std::string mask_prefix = "/node/";
    std::string mask_postfix = "/plugin/pmu_pub/chnl";
    std::string base = "";
    std::string cmd = "";
    std::string erg_units = "";

public:
    examon_mqtt_path(char* examon_host, char* config_channel)
    {
        base = config_channel + mask_prefix + examon_host + mask_postfix;
        cmd = base + "/cmd";
        erg_units = base + "/data/cpu/0/erg_units";
    }
    std::string topic_base()
    {
        return base;
    }
    std::string topic_erg_units()
    {
        return erg_units;
    }
    std::string topic_cmd()
    {
        return cmd;
    }
    std::string get_data_topic(std::string subtopic)
    {
        return base + "/data/" + subtopic;
    }
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

#ifndef INC_TASKWIFI_H_
#define INC_TASKWIFI_H_

#include "globals.h"
// #include "TaskMQTT.h"

#ifdef __cplusplus
extern "C"
{
#endif
    void parseWifiConfig(String json);
    void updateWifiConfig();
    void TaskWifi(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* INC_TASKWIFI_H_ */
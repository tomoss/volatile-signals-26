#ifndef MAPPING_HANDLER_H_
#define MAPPING_HANDLER_H_

#include "01_sensor/sensor_types.hpp"
#include "02_storage/storage.hpp"

class MappingHandler {
public:
    static Storage::StorageKey sensorModeToStorageKey(const SensorMode p_sensorMode);
};

#endif /* MAPPING_HANDLER_H_ */
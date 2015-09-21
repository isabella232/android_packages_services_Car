/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_VEHICLE_NETWORK_DATA_TYPES_H
#define ANDROID_VEHICLE_NETWORK_DATA_TYPES_H

#include <stdint.h>
#include <sys/types.h>
#include <assert.h>

#include <memory>

#include <hardware/vehicle.h>

#include <utils/List.h>
#include <utils/RefBase.h>
#include <utils/Errors.h>

/**
 * Define this macro to make the process crash when memory alloc fails.
 * Enabling this can be useful to track memory leak. When this macro is not define,
 * memory alloc failure will lead into returning from the current function
 * with behavior like returning NO_MEMORY error.
 */
#define ASSERT_ON_NO_MEMORY
#ifdef ASSERT_ON_NO_MEMORY
#define ASSERT_OR_HANDLE_NO_MEMORY(ptr, action) assert((ptr) != NULL)
#else
#define ASSERT_OR_HANDLE_NO_MEMORY(ptr, action) if ((ptr) == NULL) { action; }
#endif

#define ASSERT_ALWAYS_ON_NO_MEMORY(ptr) assert((ptr) != NULL)

namespace android {

// ----------------------------------------------------------------------------

/**
 * Collection of help utilities for vehicle_prop_config_t
 */
class VehiclePropertiesUtil {
public:
    /**
     * Helper utility to delete vehicle_prop_config_t manually. Client does not need to do this for
     * VehiclePropertiesHolder. This is for the case where client creates vehicle_prop_config_t
     * directly.
     */
    static void deleteMembers(vehicle_prop_config_t* config) {
        if (config->config_string.data != NULL && config->config_string.len > 0){
            delete[] config->config_string.data;
        }
    };
};
// ----------------------------------------------------------------------------

/**
 * Ref counted container for array of vehicle_prop_config_t.
 */
class VehiclePropertiesHolder : public virtual RefBase {
public:

    VehiclePropertiesHolder(bool deleteConfigsInDestructor = true)
        : mDeleteConfigsInDestructor(deleteConfigsInDestructor) {
    };

    virtual ~VehiclePropertiesHolder() {
        if (!mDeleteConfigsInDestructor) {
            return; // should not delete members
        }
        for (auto& e : mList) {
            vehicle_prop_config_t* eDelete = const_cast<vehicle_prop_config_t*>(e);
            VehiclePropertiesUtil::deleteMembers(eDelete);
            delete eDelete;
        }
        mList.clear();
    };

    List<vehicle_prop_config_t const *>& getList() {
        return mList;
    };

private:
    List<vehicle_prop_config_t const *> mList;
    bool mDeleteConfigsInDestructor;
};

// ----------------------------------------------------------------------------

/**
 * Collection of help utilities for vehicle_prop_value_t
 */
class VehiclePropValueUtil {
public:
    /**
     * This one only deletes pointer member, so that vehicle_prop_value_t can be stack variable.
     */
    static void deleteMembers(vehicle_prop_value_t* v) {
        if (v == NULL) {
            return;
        }
        switch (v->value_type) {
            case VEHICLE_VALUE_TYPE_BYTES:
            case VEHICLE_VALUE_TYPE_STRING: {
                delete[] v->value.str_value.data;
            } break;
        }
    };

    static status_t copyVehicleProp(vehicle_prop_value_t* dest, const vehicle_prop_value_t& src,
            bool deleteOldData = false) {
        if (deleteOldData && dest->value.str_value.data != NULL && dest->value.str_value.len > 0) {
            delete[] dest->value.str_value.data;
        }
        memcpy(dest, &src, sizeof(vehicle_prop_value_t));
        switch (src.value_type) {
        case VEHICLE_VALUE_TYPE_BYTES:
        case VEHICLE_VALUE_TYPE_STRING: {
            if (dest->value.str_value.len > 0) {
                dest->value.str_value.data = new uint8_t[dest->value.str_value.len];
                ASSERT_OR_HANDLE_NO_MEMORY(dest->value.str_value.data, return NO_MEMORY);
                memcpy(dest->value.str_value.data, src.value.str_value.data,
                        dest->value.str_value.len);
            } else {
                dest->value.str_value.data = NULL;
            }
        } break;
        }
        return NO_ERROR;
    }

    /**
     * Create a deep copy of vehicle_prop_value_t.
     */
    static vehicle_prop_value_t* allocVehicleProp(const vehicle_prop_value_t& v) {
        std::unique_ptr<vehicle_prop_value_t> copy(new vehicle_prop_value_t());
        ASSERT_OR_HANDLE_NO_MEMORY(copy.get(), return NO_MEMORY);
        status_t r = copyVehicleProp(copy.get(), v);
        if (r != NO_ERROR) {
            return NULL;
        }
        return copy.release();
    };
};

// ----------------------------------------------------------------------------

/**
 * This is utility class to have local vehicle_prop_value_t to hold data temporarily,
 * and to release all data without memory leak.
 * Usage is:
 *     ScopedVehiclePropValue value;
 *     // use value.value
 *     Then things allocated to value.value will be all cleaned up properly.
 */
class ScopedVehiclePropValue {
public:
    vehicle_prop_value_t value;

    ScopedVehiclePropValue() {
        memset(&value, 0, sizeof(value));
    };

    ~ScopedVehiclePropValue() {
        VehiclePropValueUtil::deleteMembers(&value);
    };
};

// ----------------------------------------------------------------------------
/**
 * Reference counted container of List holding vehicle_prop_value_t*.
 */
class VehiclePropValueListHolder : public virtual RefBase {
public:
    VehiclePropValueListHolder(List<vehicle_prop_value_t* > * list, bool deleteInDestructor = true)
      : mList(list),
        mDeleteInDestructor(deleteInDestructor) {};

    List<vehicle_prop_value_t*>& getList() {
        return *mList;
    };

    virtual ~VehiclePropValueListHolder() {
        if (mDeleteInDestructor && mList != NULL) {
            for (auto pv : *mList) {
                VehiclePropValueUtil::deleteMembers(pv);
                delete pv;
            }
            delete mList;
        }
    };

private:
    List<vehicle_prop_value_t*>* mList;
    bool mDeleteInDestructor;
};

}; //namespace android

#endif /* ANDROID_VEHICLE_NETWORK_DATA_TYPES_H*/

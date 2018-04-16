/*
 * Copyright (C) 2018 Matti Lehtimäki.
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
 *
 * Authored by: Matti Lehtimäki <matti.lehtimaki@gmail.com>
 */

#ifndef DROID_SENSORS_H
#define DROID_SENSORS_H

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <hardware/sensors.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DroidSensorManager DroidSensorManager;
typedef struct _DroidSensorEventQueue DroidSensorEventQueue;

typedef struct {
  char *  name;
  char *  vendor;
  int     version;
  int     handle;
  int     type;
  float   maxRange;
  float   resolution;
  float   power;
  int32_t minDelay;
} DroidSensor;

/* droidsensors.cpp */
void droid_sensors_init();
void droid_sensors_deinit();

/* private.h */
DroidSensorManager *droid_sensors_sensor_manager_create();
void droid_sensors_sensor_manager_destroy(DroidSensorManager *manager);
int droid_sensors_sensor_manager_get_sensor_list(DroidSensorManager *manager, DroidSensor **list);
DroidSensorEventQueue *droid_sensors_event_queue_create(DroidSensorManager *manager);
void droid_sensors_event_queue_destroy(DroidSensorEventQueue *queue);
int droid_sensors_event_queue_poll(DroidSensorEventQueue *queue, sensors_event_t *buffer, int numEvents);
int droid_sensors_event_queue_enable_sensor(DroidSensorManager *manager, DroidSensorEventQueue *queue, int handle);
int droid_sensors_event_queue_disable_sensor(DroidSensorManager *manager, DroidSensorEventQueue *queue, int handle);
int droid_sensors_event_queue_set_event_rate(DroidSensorManager *manager, DroidSensorEventQueue *queue, int handle, int64_t nsec);

#ifdef __cplusplus
};
#endif

#endif /* DROID_SENSORS_H */

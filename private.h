/*
 * Copyright (C) 2009 The Android Open Source Project
 * Copyright (C) 2018 Matti Lehtimäki
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

#ifndef DROID_SENSORS_PRIVATE_H
#define DROID_SENSORS_PRIVATE_H

#include "droidsensors.h"
#include <utils/Mutex.h>
#include <android/sensor.h>
#include <gui/ISensorServer.h>
#include <gui/Sensor.h>
#include <gui/SensorEventQueue.h>

struct _DroidSensorManager {
  _DroidSensorManager();
  ~_DroidSensorManager();

  ssize_t getSensorList(android::Sensor const* const** list);
  const android::Sensor *getSensor(int handle);
  android::sp<android::SensorEventQueue> createEventQueue(android::String8 packageName = android::String8(""), int mode = 0);
  bool isDataInjectionEnabled();

private:
  // DeathRecipient interface
  void sensorManagerDied();

  android::Mutex m_lock;
  android::sp<android::ISensorServer> m_sensorServer;

  android::Sensor const** m_sensorList;
  android::Vector<android::Sensor> m_sensors;
  android::sp<android::IBinder::DeathRecipient> m_deathObserver;

  const android::String16 m_opPackageName;

  android::status_t startStateLocked();
};

struct _DroidSensorEventQueue {
  _DroidSensorEventQueue(DroidSensorManager *manager);
  ~_DroidSensorEventQueue();

  int enableSensor(DroidSensorManager *manager, int handle);
  int disableSensor(DroidSensorManager *manager, int handle);

  size_t getEvents(sensors_event_t *buffer, int numEvents);
  android::status_t waitForEvent();
  int hasEvents();

  int setEventRate(DroidSensorManager *manager, int handle, int64_t nsec);

private:
  android::sp<android::SensorEventQueue> m_queue;
};

#endif /* DROID_SENSORS_PRIVATE_H */

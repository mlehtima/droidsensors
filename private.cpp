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

#include "private.h"
#include <poll.h>

#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <gui/ISensorEventConnection.h>
#include <utils/Timers.h>
#include <sensorservice/SensorService.h>

#define LOG_TAG "DroidSensorEventQueue"


_DroidSensorManager::_DroidSensorManager() :
  m_sensorList(0),
  m_opPackageName("DroidSensorManager") {
  android::SensorService::instantiate();
  startStateLocked();
}

_DroidSensorManager::~_DroidSensorManager() {
  free(m_sensorList);
}

void _DroidSensorManager::sensorManagerDied() {
  android::Mutex::Autolock _l(m_lock);
  m_sensorServer.clear();
  free(m_sensorList);
  m_sensorList = NULL;
  m_sensors.clear();
}

android::status_t _DroidSensorManager::startStateLocked() {
  bool initSensorManager = false;
  if (m_sensorServer == NULL) {
    initSensorManager = true;
  } else {
    // Ping binder to check if sensorservice is alive.
     android::status_t err = android::IInterface::asBinder(m_sensorServer)->pingBinder();
     if (err != android::NO_ERROR) {
       initSensorManager = true;
     }
  }
  if (initSensorManager) {
    // try for 300 seconds (60*5(getService() tries for 5 seconds)) before giving up ...
    const android::String16 name("sensorservice");
    for (int i = 0; i < 60; i++) {
      android::status_t err = getService(name, &m_sensorServer);
      if (err == android::NAME_NOT_FOUND) {
        sleep(1);
        continue;
      }
      if (err != android::NO_ERROR) {
        return err;
      }
      break;
    }

    class DeathObserver : public android::IBinder::DeathRecipient {
      _DroidSensorManager& m_sensorManager;
      virtual void binderDied(const android::wp<android::IBinder>& who) {
        ALOGW("sensorservice died [%p]", who.unsafe_get());
        m_sensorManager.sensorManagerDied();
      }
      public:
        DeathObserver(_DroidSensorManager& mgr) : m_sensorManager(mgr) { }
      };

      LOG_ALWAYS_FATAL_IF(m_sensorServer.get() == NULL, "getService(SensorService) NULL");

      m_deathObserver = new DeathObserver(*const_cast<_DroidSensorManager *>(this));
      android::IInterface::asBinder(m_sensorServer)->linkToDeath(m_deathObserver);

      m_sensors = m_sensorServer->getSensorList(m_opPackageName);
      size_t count = m_sensors.size();
      m_sensorList =
              static_cast<android::Sensor const**>(malloc(count * sizeof(android::Sensor *)));
      LOG_ALWAYS_FATAL_IF(m_sensorList == NULL, "m_sensorList NULL");

      for (size_t i=0 ; i<count ; i++) {
        m_sensorList[i] = m_sensors.array() + i;
      }
  }

  return android::NO_ERROR;
}

ssize_t _DroidSensorManager::getSensorList(android::Sensor const* const** list) {
  android::Mutex::Autolock _l(m_lock);
  android::status_t err = startStateLocked();
  if (err < 0) {
    return static_cast<ssize_t>(err);
  }
  *list = m_sensorList;
  return static_cast<ssize_t>(m_sensors.size());
}

const android::Sensor *_DroidSensorManager::getSensor(int handle) {
  android::Mutex::Autolock _l(m_lock);
  if (startStateLocked() == android::NO_ERROR) {
    size_t count = m_sensors.size();
    for (size_t i = 0 ; i < count ; i++) {
      if (m_sensorList[i]->getHandle() == handle) {
        return m_sensorList[i];
      }
    }
  }
  return NULL;
}

android::sp<android::SensorEventQueue> _DroidSensorManager::createEventQueue(android::String8 packageName, int mode) {
  android::sp<android::SensorEventQueue> queue;

  android::Mutex::Autolock _l(m_lock);
  while (startStateLocked() == android::NO_ERROR) {
    android::sp<android::ISensorEventConnection> connection =
        m_sensorServer->createSensorEventConnection(packageName, mode, m_opPackageName);
    if (connection == NULL) {
      // SensorService just died or the app doesn't have required permissions.
      ALOGE("createEventQueue: connection is NULL.");
      return NULL;
    }
    queue = new android::SensorEventQueue(connection);
    break;
  }
  return queue;
}

bool _DroidSensorManager::isDataInjectionEnabled() {
  android::Mutex::Autolock _l(m_lock);
  if (startStateLocked() == android::NO_ERROR) {
    return m_sensorServer->isDataInjectionEnabled();
  }
  return false;
}

_DroidSensorEventQueue::_DroidSensorEventQueue(DroidSensorManager *manager) {
  if (manager) {
    m_queue = manager->createEventQueue();
  }
}

_DroidSensorEventQueue::~_DroidSensorEventQueue()
{
}

int _DroidSensorEventQueue::enableSensor(DroidSensorManager *manager, int handle) {
  const android::Sensor *asensor = manager->getSensor(handle);
  return m_queue->enableSensor(asensor);
}

int _DroidSensorEventQueue::disableSensor(DroidSensorManager *manager, int handle) {
  const android::Sensor *asensor = manager->getSensor(handle);
  return m_queue->disableSensor(asensor);
}

size_t _DroidSensorEventQueue::getEvents(sensors_event_t *buffer, int numEvents) {
  ASensorEvent aevent[numEvents];
  ssize_t actual = m_queue->read(aevent, numEvents);
  if (actual > 0) {
    m_queue->sendAck(aevent, actual);
  }
  for (ssize_t i = 0 ; i < actual ; i++) {
    memcpy(&buffer[i], &aevent[i], sizeof(sensors_event_t));
  }
  return actual;
}

android::status_t _DroidSensorEventQueue::waitForEvent() {
  return m_queue->waitForEvent();
}

int _DroidSensorEventQueue::hasEvents() {
  struct pollfd pfd;
  pfd.fd = m_queue->getFd();
  pfd.events = POLLIN;
  pfd.revents = 0;

  int nfd = poll(&pfd, 1, 0);

  if (nfd < 0)
    return -errno;

  if (pfd.revents != POLLIN)
    return -1;

  return (nfd == 0) ? 0 : 1;
}

int _DroidSensorEventQueue::setEventRate(DroidSensorManager *manager, int handle, int64_t nsec) {
  const android::Sensor *asensor = manager->getSensor(handle);
  return m_queue->setEventRate(asensor, nsec);
}

extern "C" {

DroidSensorManager *droid_sensors_sensor_manager_create()
{
  return new DroidSensorManager();
}

void droid_sensors_sensor_manager_destroy(DroidSensorManager *manager)
{
  delete manager;
}

ssize_t droid_sensors_sensor_manager_get_sensor_list(DroidSensorManager *manager, DroidSensor **list)
{
  android::Sensor const* const* alist;
  ssize_t count = manager->getSensorList(&alist);
  *list = (DroidSensor *)malloc(count * sizeof(DroidSensor));
  for (ssize_t i = 0 ; i < count ; i++) {
//    (*list)[i].name = (char *)malloc(sizeof(char) * strlen(alist[i]->getName().string()));
//    (*list)[i].name = strcpy((*list)[i].name, alist[i]->getName().string());
//    (*list)[i].vendor = (char *)malloc(sizeof(char) * strlen(alist[i]->getVendor().string()));
//    (*list)[i].vendor = strcpy(list[i].vendor, alist[i]->getVendor().string());
    (*list)[i].name = NULL;
    (*list)[i].vendor = NULL;
    (*list)[i].handle = alist[i]->getHandle();
    (*list)[i].version = alist[i]->getVersion();
    (*list)[i].type = alist[i]->getType();
    (*list)[i].resolution = alist[i]->getResolution();
    (*list)[i].minDelay = alist[i]->getMinDelay();
    (*list)[i].maxRange = alist[i]->getMaxValue();
    (*list)[i].power = alist[i]->getPowerUsage();
  }
  return count;
}

DroidSensorEventQueue *droid_sensors_event_queue_create(DroidSensorManager *manager)
{
  if (manager) {
    return new DroidSensorEventQueue(manager);
  }
  return NULL;
}

void droid_sensors_event_queue_destroy(DroidSensorEventQueue *queue)
{
  delete queue;
}

int droid_sensors_event_queue_poll(DroidSensorEventQueue *queue, sensors_event_t *buffer, int numEvents)
{
  android::status_t ret = queue->waitForEvent();
  if (ret != android::NO_ERROR) {
    return ret;
  }
/*
  int ret = queue->hasEvents();
  if (ret <= 0) {
    return ret;
  }
*/
  return queue->getEvents(buffer, numEvents);
}

int droid_sensors_event_queue_enable_sensor(DroidSensorManager *manager, DroidSensorEventQueue *queue, int handle)
{
  return queue->enableSensor(manager, handle);
}

int droid_sensors_event_queue_disable_sensor(DroidSensorManager *manager, DroidSensorEventQueue *queue, int handle)
{
  return queue->disableSensor(manager, handle);
}

int droid_sensors_event_queue_set_event_rate(DroidSensorManager *manager, DroidSensorEventQueue *queue, int handle, int64_t nsec)
{
  return queue->setEventRate(manager, handle, nsec);
}

};

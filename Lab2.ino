#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

typedef struct {
  uint8_t sensorId;
  uint32_t timestamp;
  uint16_t value;
} SensorMessage;

typedef struct {
  uint8_t sensorId;
  uint8_t touchPin;
  QueueHandle_t queue;
  SemaphoreHandle_t touchMutex;
  TickType_t delayTicks;
} ReadTaskParams;

typedef struct {
  uint8_t sensorId;
  QueueHandle_t queue;
  SemaphoreHandle_t serialMutex;
  TickType_t delayTicks;
} SendTaskParams;

QueueHandle_t queueSensor1;
QueueHandle_t queueSensor2;

SemaphoreHandle_t serialMutex;
SemaphoreHandle_t touchMutex;

ReadTaskParams readParams1;
ReadTaskParams readParams2;
SendTaskParams sendParams1;
SendTaskParams sendParams2;

void taskReadSensor(void *pvParameters) {
  ReadTaskParams *params = (ReadTaskParams *)pvParameters;
  SensorMessage msg;

  for (;;) {
    msg.sensorId = params->sensorId;
    msg.timestamp = millis();

    if (xSemaphoreTake(params->touchMutex, portMAX_DELAY) == pdTRUE) {
      msg.value = touchRead(params->touchPin);
      xSemaphoreGive(params->touchMutex);
    } else {
      msg.value = 0;
    }

    BaseType_t ok = xQueueSend(params->queue, &msg, pdMS_TO_TICKS(100));

    if (ok != pdPASS) {
      if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        Serial.printf("{\"warning\":\"queue_full\",\"sensor\":%d}\n", params->sensorId);
        xSemaphoreGive(serialMutex);
      }
    }

    vTaskDelay(params->delayTicks);
  }
}

void taskSendJson(void *pvParameters) {
  SendTaskParams *params = (SendTaskParams *)pvParameters;
  SensorMessage msg;
  char jsonBuffer[128];

  for (;;) {
    if (xQueueReceive(params->queue, &msg, portMAX_DELAY) == pdTRUE) {
      snprintf(
        jsonBuffer,
        sizeof(jsonBuffer),
        "{\"sensor\":%d,\"timestamp\":%lu,\"value\":%u}",
        msg.sensorId,
        (unsigned long)msg.timestamp,
        msg.value
      );

      if (xSemaphoreTake(params->serialMutex, portMAX_DELAY) == pdTRUE) {
        Serial.println(jsonBuffer);
        xSemaphoreGive(params->serialMutex);
      }
    }

    vTaskDelay(params->delayTicks);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Inicializando sistema FreeRTOS...");

  queueSensor1 = xQueueCreate(10, sizeof(SensorMessage));
  queueSensor2 = xQueueCreate(10, sizeof(SensorMessage));

  serialMutex = xSemaphoreCreateMutex();
  touchMutex = xSemaphoreCreateMutex();

  if (queueSensor1 == NULL || queueSensor2 == NULL || serialMutex == NULL || touchMutex == NULL) {
    Serial.println("Error creando colas o mutex");
    while (true) { delay(1000); }
  }

  readParams1.sensorId = 1;
  readParams1.touchPin = T4;   // GPIO13
  readParams1.queue = queueSensor1;
  readParams1.touchMutex = touchMutex;
  readParams1.delayTicks = pdMS_TO_TICKS(400);

  readParams2.sensorId = 2;
  readParams2.touchPin = T6;   // GPIO14
  readParams2.queue = queueSensor2;
  readParams2.touchMutex = touchMutex;
  readParams2.delayTicks = pdMS_TO_TICKS(400);

  sendParams1.sensorId = 1;
  sendParams1.queue = queueSensor1;
  sendParams1.serialMutex = serialMutex;
  sendParams1.delayTicks = pdMS_TO_TICKS(50);

  sendParams2.sensorId = 2;
  sendParams2.queue = queueSensor2;
  sendParams2.serialMutex = serialMutex;
  sendParams2.delayTicks = pdMS_TO_TICKS(50);

  xTaskCreate(taskReadSensor, "ReadSensor1", 4096, &readParams1, 1, NULL);
  xTaskCreate(taskReadSensor, "ReadSensor2", 4096, &readParams2, 1, NULL);
  xTaskCreate(taskSendJson, "SendJson1", 4096, &sendParams1, 1, NULL);
  xTaskCreate(taskSendJson, "SendJson2", 4096, &sendParams2, 1, NULL);
}

void loop() {
}
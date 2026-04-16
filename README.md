# Laboratorio-2-FreeRTOS

<img width="681" height="425" alt="mermaid-diagram" src="https://github.com/user-attachments/assets/0f2fcda8-5797-4fa8-9a32-5fa38f3ad58a" />

#Respuestas a las preguntas
1. Se crea la misma tarea con xTaskCreate(), pero enviando un parámetro distinto en el argumento   pvParameters. Dentro de la función de tarea, ese parámetro se recibe como void * y se convierte al tipo necesario mediante casting.
  Ejemplo:
  xTaskCreate(taskReadSensor, "T1", 2048, &readParams1, 1, NULL);
  xTaskCreate(taskReadSensor, "T2", 2048, &readParams2, 1, NULL);

2. Recibe un puntero genérico void *pvParameters. Luego se convierte al tipo específico con casting.
   Ejemplo:
   void taskReadSensor(void *pvParameters) {
  ReadTaskParams *params = (ReadTaskParams *)pvParameters;
  }

3. Depende del tiempo de espera usado en xQueueSend():
   -si hay espacio antes de que expire el tiempo, el dato entra;  
   -si no hay espacio y expira el tiempo, la operación falla;
   -si el tiempo es 0, falla inmediatamente si la cola está llena.

4. Sí. FreeRTOS permite múltiples productores y múltiples consumidores sobre colas usando sus APIs de      cola. La cola administra el acceso concurrente internamente; lo importante es diseñar bien quién        produce, quién consume y qué semántica quieres en el sistema.

// #include "PerformanceMetrics.h"



// struct TaskMetric {
//     uint32_t responseTime;
//     uint32_t startTime;
//     uint32_t deadline;
//     uint32_t deadlineMisses;
//     uint32_t maxResponseTime;
// };

// static TaskMetric taskMetrics[configMAX_PRIORITIES]; // Assuming one task per priority level
// static uint32_t contextSwitchCount = 0;
// static uint32_t idleTicks = 0;
// static uint32_t totalTicks = 0;

// extern "C" {
//     void externTaskSwitchedIn() {
//         contextSwitchCount++;
//         Serial.print("Context Switch Count: ");
//         Serial.println(contextSwitchCount);
//     }
// }


// extern "C" void externTaskSwitchedOut() {
//     // If you need to handle something specific when tasks switch out, implement here.
// }


// void reportIdleTick() {
//     idleTicks++;
// }

// void taskStart(int taskId) {
//     taskMetrics[taskId].startTime = xTaskGetTickCount();
// }

// void taskComplete(int taskId) {
//     uint32_t now = xTaskGetTickCount();
//     uint32_t responseTime = now - taskMetrics[taskId].startTime;
//     taskMetrics[taskId].responseTime += responseTime;

//     Serial.print("Task "); Serial.print(taskId); Serial.println(" Completed.");
//     Serial.print("   Current Response Time: "); Serial.println(responseTime);
//     Serial.print("   Accumulated Response Time: "); Serial.println(taskMetrics[taskId].responseTime);

//     if (now > taskMetrics[taskId].deadline) {
//         taskMetrics[taskId].deadlineMisses++;
//         Serial.print("   Deadline Missed! Total Misses: "); Serial.println(taskMetrics[taskId].deadlineMisses);
//     }

//     if (responseTime > taskMetrics[taskId].maxResponseTime) {
//         taskMetrics[taskId].maxResponseTime = responseTime;
//         Serial.print("   New Worst Case Response Time: "); Serial.println(taskMetrics[taskId].maxResponseTime);
//     }
// }


// void initializePerformanceMetrics() {
//     memset(taskMetrics, 0, sizeof(taskMetrics));
//     Serial.println("Performance Metrics Initialized");
// }

// void printMetrics() {
//     Serial.println("Performance Metrics:");
//     Serial.print("Total Context Switches: "); Serial.println(contextSwitchCount);
//     Serial.print("CPU Load: "); Serial.print(100.0 * (totalTicks - idleTicks) / totalTicks); Serial.println("%");

//     for (int i = 0; i < configMAX_PRIORITIES; i++) {
//         Serial.print("Task "); Serial.print(i); Serial.println(" Metrics:");
//         Serial.print("   Total Response Time: "); Serial.println(taskMetrics[i].responseTime);
//         Serial.print("   Deadline Misses: "); Serial.println(taskMetrics[i].deadlineMisses);
//         Serial.print("   Worst Case Response Time: "); Serial.println(taskMetrics[i].maxResponseTime);
//     }
// }

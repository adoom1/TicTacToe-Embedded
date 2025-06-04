// Code for Phase 1 of the Network Simulation Project
// If the code fails with "PRIGROUP unimplemented", press the reset button on the kit pop-up window
// If the issue persists, clean the project and rebuild it
// This error seems to be inconsistent, so these steps should help resolve it
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// Constants used for the packet structure and simulation settings
#define PACKET_SIZE 1000            // Packet size set to 1000 bytes as requirements 
#define SENDER_PERIOD_MS 200        // Sender generates packets every 200 milliseconds
#define SWITCH_DELAY_MS 200         // Switch delays forwarding by 200 milliseconds
#define DROP_PROBABILITY 0.01       // 1% probability for packet drop
#define NUM_SENDERS 2               // Number of sender tasks is 2
#define NUM_RECEIVERS 2             // Number of receiver tasks is 2
#define PACKETS_PER_RECEIVER 200    // Default number of packets for each receiver to stop at; can be changed

// Packet structure definition, total size is 1000 bytes
typedef struct {
    uint32_t seq_num;               // Sequence number for tracking packets
    uint16_t length;                // Length of the packet (1000 bytes)
    uint8_t dest;                   // Destination ID (3 or 4, displayed as 1 or 2 in logs)
    uint8_t sender_id;              // Sender ID (0 or 1, displayed as 1 or 2 in logs)
    uint8_t data[PACKET_SIZE - sizeof(uint32_t) - sizeof(uint16_t) - 2 * sizeof(uint8_t)]; // Remaining bytes for data payload
} Packet_t;

// Global variables for managing queues, counters, and task handles
QueueHandle_t sender_queues[NUM_SENDERS];        // Queues for sender-to-switch communication
QueueHandle_t receiver_queues[NUM_RECEIVERS];    // Queues for switch-to-receiver communication
uint32_t receiver_counts[NUM_RECEIVERS];         // Tracks the number of packets received by each receiver
uint32_t receiver_lost[NUM_RECEIVERS];           // Tracks the number of lost packets for each receiver
uint32_t last_seq[NUM_RECEIVERS][NUM_SENDERS];   // Stores the last sequence number for each sender-receiver pair
SemaphoreHandle_t simulation_done_sem;           // Semaphore to signal when the simulation is complete
TaskHandle_t sender_handles[NUM_SENDERS];        // Handles for sender tasks
TaskHandle_t switch_handle;                      // Handle for the switch task
TaskHandle_t receiver_handles[NUM_RECEIVERS];    // Handles for receiver tasks
TaskHandle_t terminator_handle;                  // Handle for the terminator task
uint32_t packets_to_stop = PACKETS_PER_RECEIVER; // Number of packets to stop at; set to 200 for this phase but can be changed

// Sender task: generates packets every 200ms and sends them to the sender’s queue
void SenderTask(void *pvParameters) {
    int sender_id = (int)pvParameters;          // Sender ID (0 or 1)
    uint32_t seq_nums[NUM_RECEIVERS] = {0};     // Sequence numbers for each destination
    TickType_t last_wake = xTaskGetTickCount(); // Used to maintain precise 200ms timing

    while (1) {
        // Allocate memory for a new packet
        Packet_t *packet = (Packet_t *)pvPortMalloc(sizeof(Packet_t));
        if (!packet) {
            printf("Sender %d: Malloc failed\n", sender_id + 1);
            continue;
        }

        // Set packet details: random destination (3 or 4), sequence number, length, and sender ID
        packet->dest = (rand() % 2) + 3;
        packet->seq_num = seq_nums[packet->dest - 3]++;
        packet->length = PACKET_SIZE;
        packet->sender_id = sender_id;
        memset(packet->data, 0, sizeof(packet->data)); // Clear the data payload to avoid random values

        // Print packet generation details (Sender 1/2, Receiver 1/2)
        printf("Sender %d: Generated packet for dest %d, seq %u\n",
               sender_id + 1, packet->dest - 2, packet->seq_num);

        // Send the packet to the sender’s queue; free memory if the queue is full
        if (xQueueSend(sender_queues[sender_id], &packet, portMAX_DELAY) != pdTRUE) {
            printf("Sender %d: Queue send failed\n", sender_id + 1);
            vPortFree(packet);
        }

        // Wait 200ms to maintain the packet generation rate
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SENDER_PERIOD_MS));
    }
}

// Switch task: processes packets from sender queues, applies drop probability, and forwards to receivers
void SwitchTask(void *pvParameters) {
    Packet_t *packet; // Pointer to the received packet

    while (1) {
        // Check each sender queue for incoming packets
        for (int i = 0; i < NUM_SENDERS; i++) {
            if (xQueueReceive(sender_queues[i], &packet, 0) == pdTRUE) {
                // Apply a 1% probability to drop the packet
                if ((double)rand() / RAND_MAX < DROP_PROBABILITY) {
                    printf("Switch: Dropped packet from sender %d for dest %d, seq %u\n",
                           packet->sender_id + 1, packet->dest - 2, packet->seq_num);
                    vPortFree(packet); // Free the dropped packet
                } else {
                    // Simulate a 200ms forwarding delay
                    vTaskDelay(pdMS_TO_TICKS(SWITCH_DELAY_MS));
                    // Forward the packet to the correct receiver (dest 3 or 4)
                    if (packet->dest >= 3 && packet->dest <= 4) {
                        printf("Switch: Forwarding packet from sender %d to dest %d, seq %u\n",
                               packet->sender_id + 1, packet->dest - 2, packet->seq_num);
                        xQueueSend(receiver_queues[packet->dest - 3], &packet, portMAX_DELAY);
                    } else {
                        // Handle invalid destination by logging and freeing the packet
                        printf("Switch: Invalid dest %d from sender %d for seq %u\n",
                               packet->dest - 2, packet->sender_id + 1, packet->seq_num);
                        vPortFree(packet);
                    }
                }
            }
        }
        taskYIELD(); // Yield to allow other tasks to run
    }
}

// Receiver task: processes incoming packets, tracks losses, and stops at a configurable number of packets (200 for this phase)
void ReceiverTask(void *pvParameters) {
    int receiver_id = (int)pvParameters + 3;    // Receiver ID (3 or 4)
    int queue_idx = receiver_id - 3;            // Queue index (0 or 1)
    Packet_t *packet;                           // Pointer to the received packet
    static int receivers_done = 0;              // Counter for receivers that have finished

    while (1) {
        // Receive a packet from the queue (blocks until a packet arrives)
        if (xQueueReceive(receiver_queues[queue_idx], &packet, portMAX_DELAY) == pdTRUE) {
            // Check if the packet was sent to the wrong receiver
            if (packet->dest != receiver_id) {
                printf("Receiver %d: Error - received packet for dest %d from sender %d, seq %u\n",
                       receiver_id - 2, packet->dest - 2, packet->sender_id + 1, packet->seq_num);
            } else {
                // Process packets until the receiver reaches the specified number (200 for this phase)
                if (receiver_counts[queue_idx] < packets_to_stop) {
                    receiver_counts[queue_idx]++;
                    // Detect lost packets by checking for sequence number gaps
                    uint32_t expected_seq = last_seq[queue_idx][packet->sender_id] + 1;
                    if (packet->seq_num > expected_seq) {
                        receiver_lost[queue_idx] += packet->seq_num - expected_seq;
                        printf("Receiver %d: Detected %u lost packets from sender %d\n",
                               receiver_id - 2, packet->seq_num - expected_seq, packet->sender_id + 1);
                    }
                    last_seq[queue_idx][packet->sender_id] = packet->seq_num;
                    // Log the received packet (Receiver 1/2, Sender 1/2)
                    printf("Receiver %d: Received packet from sender %d, seq %u, total %u, lost %u\n",
                           receiver_id - 2, packet->sender_id + 1, packet->seq_num,
                           receiver_counts[queue_idx], receiver_lost[queue_idx]);

                    // If the specified number of packets is received, print final stats and stop
                    if (receiver_counts[queue_idx] >= packets_to_stop) {
                        printf("Receiver %d: Final stats - total %u, lost %u\n",
                               receiver_id - 2, receiver_counts[queue_idx], receiver_lost[queue_idx]);
                        receivers_done++;
                        // Signal completion when both receivers are done
                        if (receivers_done == NUM_RECEIVERS) {
                            xSemaphoreGive(simulation_done_sem);
                        }
                        vTaskSuspend(NULL); // Suspend this receiver task
                    }
                }
            }
            vPortFree(packet); // Free the processed packet
        }
    }
}

// Terminator task: waits for the simulation to complete and suspends all tasks
void TerminatorTask(void *pvParameters) {
    // Wait for both receivers to finish (semaphore is triggered when done)
    if (xSemaphoreTake(simulation_done_sem, portMAX_DELAY) == pdTRUE) {
        printf("Simulation complete. Suspending all tasks.\n");
        // Suspend all sender tasks
        for (int i = 0; i < NUM_SENDERS; i++) {
            vTaskSuspend(sender_handles[i]);
        }
        // Suspend the switch task
        vTaskSuspend(switch_handle);
        // Suspend the receiver tasks (already stopped, but ensure they remain suspended)
        for (int i = 0; i < NUM_RECEIVERS; i++) {
            vTaskSuspend(receiver_handles[i]);
        }
        vTaskSuspend(NULL); // Suspend the terminator task itself
    }
}

// FreeRTOS idle hook: required but left empty
void vApplicationIdleHook(void) {
}

// FreeRTOS malloc failure hook: logs an error and halts execution
void vApplicationMallocFailedHook(void) {
    printf("Malloc failed! Halting execution.\n");
    for (;;);
}

// FreeRTOS stack overflow hook: logs an error and halts execution
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName) {
    printf("Stack overflow in task %s! Halting execution.\n", pcTaskName);
    for (;;);
}

// FreeRTOS tick hook: required but left empty
void vApplicationTickHook(void) {
}

// Main function: initializes resources, creates tasks, and starts the scheduler
void main(void) {
    // Print to confirm that the main function is running
    printf("Starting simulation...\n");

    // Static counter to ensure a unique random seed for each run
    static uint32_t run_counter = 0;
    // Combine tick count with the counter to create a dynamic seed
    uint32_t seed = xTaskGetTickCount() + run_counter++;
    printf("Random seed: %u\n", seed); // Print the seed to verify it changes each run
    srand(seed);

    // Create a binary semaphore for simulation termination
    simulation_done_sem = xSemaphoreCreateBinary();
    if (!simulation_done_sem) {
        printf("Failed to create simulation done semaphore\n");
        return;
    }

    // Initialize sender queues (one per sender)
    for (int i = 0; i < NUM_SENDERS; i++) {
        sender_queues[i] = xQueueCreate(20, sizeof(Packet_t *));
        if (!sender_queues[i]) {
            printf("Failed to create sender queue %d\n", i);
            return;
        }
    }

    // Initialize receiver queues (one per receiver)
    for (int i = 0; i < NUM_RECEIVERS; i++) {
        receiver_queues[i] = xQueueCreate(20, sizeof(Packet_t *));
        if (!receiver_queues[i]) {
            printf("Failed to create receiver queue %d\n", i);
            return;
        }
    }

    // Create sender tasks and store their handles
    for (int i = 0; i < NUM_SENDERS; i++) {
        char name[16];
        snprintf(name, sizeof(name), "Sender%d", i + 1);
        xTaskCreate(SenderTask, name, 512, (void *)i,
                    tskIDLE_PRIORITY + 2, &sender_handles[i]);
    }

    // Create the switch task and store its handle
    xTaskCreate(SwitchTask, "Switch", 512, NULL,
                tskIDLE_PRIORITY + 1, &switch_handle);

    // Create receiver tasks and store their handles
    for (int i = 0; i < NUM_RECEIVERS; i++) {
        char name[16];
        snprintf(name, sizeof(name), "Receiver%d", i + 1);
        xTaskCreate(ReceiverTask, name, 512, (void *)i,
                    tskIDLE_PRIORITY + 1, &receiver_handles[i]);
    }

    // Create the terminator task to handle simulation shutdown
    xTaskCreate(TerminatorTask, "Terminator", 512, NULL,
                tskIDLE_PRIORITY + 3, &terminator_handle);

    // Start the FreeRTOS scheduler
    vTaskStartScheduler();

    // This loop should not be reached; included as a precaution
    for (;;) ;
}
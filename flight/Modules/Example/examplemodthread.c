/**
 ******************************************************************************
 *
 * @file       examplemodthread.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Example module to be used as a template for actual modules.
 *             Threaded version.
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/**
 * Input objects: ExampleObject1, ExampleSettings
 * Output object: ExampleObject2
 *
 * This module executes in response to ExampleObject1 updates. When the
 * module is triggered it will update the data of ExampleObject2.
 *
 * The module executes in its own thread in this example.
 *
 * UAVObjects are automatically generated by the UAVObjectGenerator from
 * the object definition XML file.
 *
 * Modules have no API, all communication to other modules is done through UAVObjects.
 * However modules may use the API exposed by shared libraries.
 * See the OpenPilot wiki for more details.
 * http://www.openpilot.org/OpenPilot_Application_Architecture
 */

#include "openpilot.h"
#include "examplemodthread.h"
#include "exampleobject1.h"	// object the module will listen for updates (input)
#include "exampleobject2.h"	// object the module will update (output)
#include "examplesettings.h"	// object holding module settings (input)

// Private constants
#define MAX_QUEUE_SIZE 20
#define STACK_SIZE configMINIMAL_STACK_SIZE
#define TASK_PRIORITY (tskIDLE_PRIORITY+1)

// Private types

// Private variables
static xQueueHandle queue;
static xTaskHandle taskHandle;

// Private functions
static void exampleTask(void *parameters);

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t ExampleModThreadInitialize()
{
	// Create object queue
	queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(UAVObjEvent));
	vQueueAddToRegistry(queue, (char*)"Example_queue");

	// Listen for ExampleObject1 updates
	ExampleObject1ConnectQueue(queue);

	// Start main task
	xTaskCreate(exampleTask, (signed char *)"ExampleThread", STACK_SIZE, NULL, TASK_PRIORITY, &taskHandle);

	return 0;
}

/**
 * Module thread, should not return.
 */
static void exampleTask(void *parameters)
{
	UAVObjEvent ev;
	ExampleSettingsData settings;
	ExampleObject1Data data1;
	ExampleObject1Data data2;
	int32_t step;

	// Main task loop
	while (1) {
		// Check the event queue for any object updates. In this case
		// we are only listening for the ExampleSettings object. However
		// the module could listen to multiple objects.
		// Since this object only executes on object updates, the task
		// will block until an object event is pushed in the queue.
		while (xQueueReceive(queue, &ev, portMAX_DELAY) != pdTRUE) ;

		// Make sure that the object update is for ExampleObject1
		// This is redundant in this example since we have only
		// registered a single object in the queue, however
		// in most practical cases there will be more than one
		// object connected in the queue.
		if (ev.obj == ExampleObject1Handle()) {
			// Update settings with latest value
			ExampleSettingsGet(&settings);

			// Get the input object
			ExampleObject1Get(&data1);

			// Determine how to update the output object
			if (settings.StepDirection == EXAMPLESETTINGS_STEPDIRECTION_UP) {
				step = settings.StepSize;
			} else {
				step = -settings.StepSize;
			}

			// Update data
			data2.Field1 = data1.Field1 + step;
			data2.Field2 = data1.Field2 + step;
			data2.Field3 = data1.Field3 + step;
			data2.Field4[0] = data1.Field4[0] + step;
			data2.Field4[1] = data1.Field4[1] + step;

			// Update the ExampleObject2, after this function is called
			// notifications to any other modules listening to that object
			// will be sent and the GCS object will be updated through the
			// telemetry link. All operations will take place asynchronously
			// and the following call will return immediately.
			ExampleObject2Set(&data2);
		}
	}
}

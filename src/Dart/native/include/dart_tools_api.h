// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef RUNTIME_INCLUDE_DART_TOOLS_API_H_
#define RUNTIME_INCLUDE_DART_TOOLS_API_H_

#include "dart_api.h" /* NOLINT */

/** \mainpage Dart Tools Embedding API Reference
 *
 * This reference describes the Dart embedding API for tools. Tools include
 * a debugger, service protocol, and timeline.
 *
 * NOTE: The APIs described in this file are unstable and subject to change.
 *
 * This reference is generated from the header include/dart_tools_api.h.
 */

/*
 * ========
 * Debugger
 * ========
 */

/**
 * ILLEGAL_ISOLATE_ID is a number guaranteed never to be associated with a
 * valid isolate.
 */
#define ILLEGAL_ISOLATE_ID ILLEGAL_PORT


/*
 * =======
 * Service
 * =======
 */

/**
 * A service request callback function.
 *
 * These callbacks, registered by the embedder, are called when the VM receives
 * a service request it can't handle and the service request command name
 * matches one of the embedder registered handlers.
 *
 * The return value of the callback indicates whether the response
 * should be used as a regular result or an error result.
 * Specifically, if the callback returns true, a regular JSON-RPC
 * response is built in the following way:
 *
 * {
 *   "jsonrpc": "2.0",
 *   "result": <json_object>,
 *   "id": <some sequence id>,
 * }
 *
 * If the callback returns false, a JSON-RPC error is built like this:
 *
 * {
 *   "jsonrpc": "2.0",
 *   "error": <json_object>,
 *   "id": <some sequence id>,
 * }
 *
 * \param method The rpc method name.
 * \param param_keys Service requests can have key-value pair parameters. The
 *   keys and values are flattened and stored in arrays.
 * \param param_values The values associated with the keys.
 * \param num_params The length of the param_keys and param_values arrays.
 * \param user_data The user_data pointer registered with this handler.
 * \param result A C string containing a valid JSON object. The returned
 *   pointer will be freed by the VM by calling free.
 *
 * \return True if the result is a regular JSON-RPC response, false if the
 *   result is a JSON-RPC error.
 */
typedef bool (*Dart_ServiceRequestCallback)(const char* method,
                                            const char** param_keys,
                                            const char** param_values,
                                            intptr_t num_params,
                                            void* user_data,
                                            const char** json_object);

/**
 * Register a Dart_ServiceRequestCallback to be called to handle
 * requests for the named rpc on a specific isolate. The callback will
 * be invoked with the current isolate set to the request target.
 *
 * \param method The name of the method that this callback is responsible for.
 * \param callback The callback to invoke.
 * \param user_data The user data passed to the callback.
 *
 * NOTE: If multiple callbacks with the same name are registered, only
 * the last callback registered will be remembered.
 */
DART_EXPORT void Dart_RegisterIsolateServiceRequestCallback(
    const char* method,
    Dart_ServiceRequestCallback callback,
    void* user_data);

/**
 * Register a Dart_ServiceRequestCallback to be called to handle
 * requests for the named rpc. The callback will be invoked without a
 * current isolate.
 *
 * \param method The name of the command that this callback is responsible for.
 * \param callback The callback to invoke.
 * \param user_data The user data passed to the callback.
 *
 * NOTE: If multiple callbacks with the same name are registered, only
 * the last callback registered will be remembered.
 */
DART_EXPORT void Dart_RegisterRootServiceRequestCallback(
    const char* method,
    Dart_ServiceRequestCallback callback,
    void* user_data);

/**
 * Embedder information which can be requested by the VM for internal or
 * reporting purposes.
 *
 * The pointers in this structure are not going to be cached or freed by the VM.
 */

 #define DART_EMBEDDER_INFORMATION_CURRENT_VERSION (0x00000001)

typedef struct {
  int32_t version;
  const char* name;  // [optional] The name of the embedder
  int64_t current_rss;  // [optional] the current RSS of the embedder
  int64_t max_rss;  // [optional] the maximum RSS of the embedder
} Dart_EmbedderInformation;

/**
 * Callback provided by the embedder that is used by the vm to request
 * information.
 *
 * \return Returns a pointer to a Dart_EmbedderInformation structure.
 * The embedder keeps the ownership of the structure and any field in it.
 * The embedder must ensure that the structure will remain valid until the
 * next invokation of the callback.
 */
typedef void (*Dart_EmbedderInformationCallback)(
    Dart_EmbedderInformation* info);

/**
 * Register a Dart_ServiceRequestCallback to be called to handle
 * requests for the named rpc. The callback will be invoked without a
 * current isolate.
 *
 * \param method The name of the command that this callback is responsible for.
 * \param callback The callback to invoke.
 * \param user_data The user data passed to the callback.
 *
 * NOTE: If multiple callbacks with the same name are registered, only
 * the last callback registered will be remembered.
 */
DART_EXPORT void Dart_SetEmbedderInformationCallback(
    Dart_EmbedderInformationCallback callback);

/**
 * Invoke a vm-service method and wait for its result.
 *
 * \param request_json The utf8-encoded json-rpc request.
 * \param request_json_length The length of the json-rpc request.
 *
 * \param response_json The returned utf8-encoded json response, must be
 *   free()ed by caller.
 * \param response_json_length The length of the returned json response.
 * \param error An optional error, must be free()ed by caller.
 *
 * \return Whether the call was sucessfully performed.
 *
 * NOTE: This method does not need a current isolate and must not have the
 * vm-isolate being the current isolate. It must be called after
 * Dart_Initialize() and before Dart_Cleanup().
 */
DART_EXPORT bool Dart_InvokeVMServiceMethod(uint8_t* request_json,
                                            intptr_t request_json_length,
                                            uint8_t** response_json,
                                            intptr_t* response_json_length,
                                            char** error);

/*
 * ========
 * Event Streams
 * ========
 */

/**
 * A callback invoked when the VM service gets a request to listen to
 * some stream.
 *
 * \return Returns true iff the embedder supports the named stream id.
 */
typedef bool (*Dart_ServiceStreamListenCallback)(const char* stream_id);

/**
 * A callback invoked when the VM service gets a request to cancel
 * some stream.
 */
typedef void (*Dart_ServiceStreamCancelCallback)(const char* stream_id);

/**
 * Adds VM service stream callbacks.
 *
 * \param listen_callback A function pointer to a listen callback function.
 *   A listen callback function should not be already set when this function
 *   is called. A NULL value removes the existing listen callback function
 *   if any.
 *
 * \param cancel_callback A function pointer to a cancel callback function.
 *   A cancel callback function should not be already set when this function
 *   is called. A NULL value removes the existing cancel callback function
 *   if any.
 *
 * \return Success if the callbacks were added.  Otherwise, returns an
 *   error handle.
 */
DART_EXPORT char* Dart_SetServiceStreamCallbacks(
    Dart_ServiceStreamListenCallback listen_callback,
    Dart_ServiceStreamCancelCallback cancel_callback);

/**
 * Sends a data event to clients of the VM Service.
 *
 * A data event is used to pass an array of bytes to subscribed VM
 * Service clients.  For example, in the standalone embedder, this is
 * function used to provide WriteEvents on the Stdout and Stderr
 * streams.
 *
 * If the embedder passes in a stream id for which no client is
 * subscribed, then the event is ignored.
 *
 * \param stream_id The id of the stream on which to post the event.
 *
 * \param event_kind A string identifying what kind of event this is.
 *   For example, 'WriteEvent'.
 *
 * \param bytes A pointer to an array of bytes.
 *
 * \param bytes_length The length of the byte array.
 *
 * \return NULL if the arguments are well formed.  Otherwise, returns an
 *   error string. The caller is responsible for freeing the error message.
 */
DART_EXPORT char* Dart_ServiceSendDataEvent(const char* stream_id,
                                            const char* event_kind,
                                            const uint8_t* bytes,
                                            intptr_t bytes_length);

/**
 * Usage statistics for a space/generation at a particular moment in time.
 *
 * \param used Amount of memory used, in bytes.
 *
 * \param capacity Memory capacity, in bytes.
 *
 * \param external External memory, in bytes.
 *
 * \param collections How many times the garbage collector has run in this
 *   space.
 *
 * \param time Cumulative time spent collecting garbage in this space, in
 *   seconds.
 *
 * \param avg_collection_period Average time between garbage collector running
 *   in this space, in milliseconds.
 */
typedef struct {
  intptr_t used;
  intptr_t capacity;
  intptr_t external;
  intptr_t collections;
  double time;
  double avg_collection_period;
} Dart_GCStats;

/**
 * A Garbage Collection event with memory usage statistics.
 *
 * \param type The event type. Static lifetime.
 *
 * \param reason The reason for the GC event. Static lifetime.
 *
 * \param new_space Data for New Space.
 *
 * \param old_space Data for Old Space.
 */
typedef struct {
  const char* type;
  const char* reason;
  const char* isolate_id;

  Dart_GCStats new_space;
  Dart_GCStats old_space;
} Dart_GCEvent;

/**
 * A callback invoked when the VM emits a GC event.
 *
 * \param event The GC event data. Pointer only valid for the duration of the
 *   callback.
 */
typedef void (*Dart_GCEventCallback)(Dart_GCEvent* event);

/**
 * Sets the native GC event callback.
 *
 * \param callback A function pointer to an event handler callback function.
 *   A NULL value removes the existing listen callback function if any.
 */
DART_EXPORT void Dart_SetGCEventCallback(Dart_GCEventCallback callback);

/*
 * ========
 * Reload support
 * ========
 *
 * These functions are used to implement reloading in the Dart VM.
 * This is an experimental feature, so embedders should be prepared
 * for these functions to change.
 */

/**
 * A callback which determines whether the file at some url has been
 * modified since some time.  If the file cannot be found, true should
 * be returned.
 */
typedef bool (*Dart_FileModifiedCallback)(const char* url, int64_t since);

DART_EXPORT char* Dart_SetFileModifiedCallback(
    Dart_FileModifiedCallback file_modified_callback);

/**
 * Returns true if isolate is currently reloading.
 */
DART_EXPORT bool Dart_IsReloading();

/*
 * ========
 * Timeline
 * ========
 */

/**
 * Enable tracking of specified timeline category. This is operational
 * only when systrace timeline functionality is turned on.
 *
 * \param categories A comma seperated list of categories that need to
 *   be enabled, the categories are
 *   "all" : All categories
 *   "API" - Execution of Dart C API functions
 *   "Compiler" - Execution of Dart JIT compiler
 *   "CompilerVerbose" - More detailed Execution of Dart JIT compiler
 *   "Dart" - Execution of Dart code
 *   "Debugger" - Execution of Dart debugger
 *   "Embedder" - Execution of Dart embedder code
 *   "GC" - Execution of Dart Garbage Collector
 *   "Isolate" - Dart Isolate lifecycle execution
 *   "VM" - Excution in Dart VM runtime code
 *   "" - None
 *
 *  When "all" is specified all the categories are enabled.
 *  When a comma seperated list of categories is specified, the categories
 *   that are specified will be enabled and the rest will be disabled. 
 *  When "" is specified all the categories are disabled.
 *  The category names are case sensitive.
 *  eg:  Dart_EnableTimelineCategory("all");
 *       Dart_EnableTimelineCategory("GC,API,Isolate");
 *       Dart_EnableTimelineCategory("GC,Debugger,Dart");
 *
 * \return True if the categories were successfully enabled, False otherwise.
 */
DART_EXPORT bool Dart_SetEnabledTimelineCategory(const char* categories);

/**
 * Returns a timestamp in microseconds. This timestamp is suitable for
 * passing into the timeline system, and uses the same monotonic clock
 * as dart:developer's Timeline.now.
 *
 * \return A timestamp that can be passed to the timeline system.
 */
DART_EXPORT int64_t Dart_TimelineGetMicros();

/**
 * Returns a raw timestamp in from the monotonic clock.
 *
 * \return A raw timestamp from the monotonic clock.
 */
DART_EXPORT int64_t Dart_TimelineGetTicks();

/**
 * Returns the frequency of the monotonic clock.
 *
 * \return The frequency of the monotonic clock.
 */
DART_EXPORT int64_t Dart_TimelineGetTicksFrequency();

typedef enum {
  Dart_Timeline_Event_Begin,          // Phase = 'B'.
  Dart_Timeline_Event_End,            // Phase = 'E'.
  Dart_Timeline_Event_Instant,        // Phase = 'i'.
  Dart_Timeline_Event_Duration,       // Phase = 'X'.
  Dart_Timeline_Event_Async_Begin,    // Phase = 'b'.
  Dart_Timeline_Event_Async_End,      // Phase = 'e'.
  Dart_Timeline_Event_Async_Instant,  // Phase = 'n'.
  Dart_Timeline_Event_Counter,        // Phase = 'C'.
  Dart_Timeline_Event_Flow_Begin,     // Phase = 's'.
  Dart_Timeline_Event_Flow_Step,      // Phase = 't'.
  Dart_Timeline_Event_Flow_End,       // Phase = 'f'.
} Dart_Timeline_Event_Type;

/**
 * Add a timeline event to the embedder stream.
 *
 * \param label The name of the event. Its lifetime must extend at least until
 *     Dart_Cleanup.
 * \param timestamp0 The first timestamp of the event.
 * \param timestamp1_or_async_id The second timestamp of the event or
 *     the async id.
 * \param argument_count The number of argument names and values.
 * \param argument_names An array of names of the arguments. The lifetime of the
 *     names must extend at least until Dart_Cleanup. The array may be reclaimed
 *     when this call returns.
 * \param argument_values An array of values of the arguments. The values and
 *     the array may be reclaimed when this call returns.
 */
DART_EXPORT void Dart_TimelineEvent(const char* label,
                                    int64_t timestamp0,
                                    int64_t timestamp1_or_async_id,
                                    Dart_Timeline_Event_Type type,
                                    intptr_t argument_count,
                                    const char** argument_names,
                                    const char** argument_values);

/**
 * Associates a name with the current thread. This name will be used to name
 * threads in the timeline. Can only be called after a call to Dart_Initialize.
 *
 * \param name The name of the thread.
 */
DART_EXPORT void Dart_SetThreadName(const char* name);

/*
 * =======
 * Metrics
 * =======
 */

/**
 * Return metrics gathered for the VM and individual isolates.
 *
 * NOTE: Non-heap metrics are not available in PRODUCT builds of Dart.
 * Calling the non-heap metric functions on a PRODUCT build might return invalid metrics.
 */
DART_EXPORT int64_t Dart_VMIsolateCountMetric();  // Counter
DART_EXPORT int64_t Dart_VMCurrentRSSMetric();    // Byte
DART_EXPORT int64_t Dart_VMPeakRSSMetric();       // Byte
DART_EXPORT int64_t
Dart_IsolateHeapOldUsedMetric(Dart_Isolate isolate);  // Byte
DART_EXPORT int64_t
Dart_IsolateHeapOldUsedMaxMetric(Dart_Isolate isolate);  // Byte
DART_EXPORT int64_t
Dart_IsolateHeapOldCapacityMetric(Dart_Isolate isolate);  // Byte
DART_EXPORT int64_t
Dart_IsolateHeapOldCapacityMaxMetric(Dart_Isolate isolate);  // Byte
DART_EXPORT int64_t
Dart_IsolateHeapOldExternalMetric(Dart_Isolate isolate);  // Byte
DART_EXPORT int64_t
Dart_IsolateHeapNewUsedMetric(Dart_Isolate isolate);  // Byte
DART_EXPORT int64_t
Dart_IsolateHeapNewUsedMaxMetric(Dart_Isolate isolate);  // Byte
DART_EXPORT int64_t
Dart_IsolateHeapNewCapacityMetric(Dart_Isolate isolate);  // Byte
DART_EXPORT int64_t
Dart_IsolateHeapNewCapacityMaxMetric(Dart_Isolate isolate);  // Byte
DART_EXPORT int64_t
Dart_IsolateHeapNewExternalMetric(Dart_Isolate isolate);  // Byte
DART_EXPORT int64_t
Dart_IsolateHeapGlobalUsedMetric(Dart_Isolate isolate);  // Byte
DART_EXPORT int64_t
Dart_IsolateHeapGlobalUsedMaxMetric(Dart_Isolate isolate);  // Byte
DART_EXPORT int64_t
Dart_IsolateRunnableLatencyMetric(Dart_Isolate isolate);  // Microsecond
DART_EXPORT int64_t
Dart_IsolateRunnableHeapSizeMetric(Dart_Isolate isolate);  // Byte

/*
 * ========
 * UserTags
 * ========
 */

/*
 * Gets the current isolate's currently set UserTag instance.
 *
 * \return The currently set UserTag instance.
 */
DART_EXPORT Dart_Handle Dart_GetCurrentUserTag();

/*
 * Gets the current isolate's default UserTag instance.
 *
 * \return The default UserTag with label 'Default'
 */
DART_EXPORT Dart_Handle Dart_GetDefaultUserTag();

/*
 * Creates a new UserTag instance.
 *
 * \param label The name of the new UserTag.
 *
 * \return The newly created UserTag instance or an error handle.
 */
DART_EXPORT Dart_Handle Dart_NewUserTag(const char* label);

/*
 * Updates the current isolate's UserTag to a new value.
 *
 * \param user_tag The UserTag to be set as the current UserTag.
 *
 * \return The previously set UserTag instance or an error handle.
 */
DART_EXPORT Dart_Handle Dart_SetCurrentUserTag(Dart_Handle user_tag);

/*
 * Returns the label of a given UserTag instance.
 *
 * \param user_tag The UserTag from which the label will be retrieved.
 *
 * \return The UserTag's label. NULL if the user_tag is invalid. The caller is
 *   responsible for freeing the returned label.
 */
DART_EXPORT DART_WARN_UNUSED_RESULT char* Dart_GetUserTagLabel(
    Dart_Handle user_tag);

#endif  // RUNTIME_INCLUDE_DART_TOOLS_API_H_

/*
** Copyright 2014-2016 The Earlham Institute
** 
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
** 
**     http://www.apache.org/licenses/LICENSE-2.0
** 
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/**
 * @file
 * @brief
 */
/*
 * blast_service_job.h
 *
 *  Created on: 21 Jan 2016
 *      Author: tyrrells
 */

#ifndef BLAST_SERVICE_JOB_H_
#define BLAST_SERVICE_JOB_H_


#include "blast_service_api.h"
#include "service.h"
#include "service_job.h"
#include "blast_service.h"

class BlastTool;

/**
 * This datatype stores the ServiceJob and its associated BlastTool.
 *
 * @extends ServiceJob
 * @ingroup blast_service
 */
typedef struct BLAST_SERVICE_LOCAL BlastServiceJob
{
	/** The ServiceJob. */
	ServiceJob bsj_job;

	/** The BlastTool. */
	BlastTool *bsj_tool_p;

} BlastServiceJob;


/** The name of the exposed database output parameter for LinkedServices. */
BLAST_SERVICE_PREFIX const char *BSJ_LINKED_SERVICE_DATABASE_S BLAST_SERVICE_VAL ("database");


/** The name of the exposed scaffold output parameter for LinkedServices. */
BLAST_SERVICE_PREFIX const char *BSJ_LINKED_SERVICE_SCAFFOLD_S BLAST_SERVICE_VAL ("scaffold");



#ifdef __cplusplus
extern "C"
{
#endif


/**
 * Create a BlastServiceJob.
 *
 * @param service_p The Blast service.
 * @param db_p The DatabaseInfo details that the BlastServiceJob will run against.
 * @param data_p The BlastServiceData.
 * @return The newly-allocated BlastServiceJob or <code>NULL</code> upon error.
 * @memberof BlastServiceJob
 */
BLAST_SERVICE_LOCAL BlastServiceJob *AllocateBlastServiceJobForDatabase (Service *service_p, const DatabaseInfo *db_p, struct BlastServiceData *data_p);


/**
 * Create a BlastServiceJob.
 *
 * @param service_p The Blast service.
 * @param job_name_s The name of the BlastServiceJob.
 * @param job_description_s The description of the BlastServiceJob. This can be <code>NULL</code>.
 * @param tool_name_s The name to give to the underlying BlastTool for this BlastServiceJob.
 * This value will be deep-copied.
 * @param data_p The BlastServiceData.
 * @return The newly-allocated BlastServiceJob or <code>NULL</code> upon error.
 * @memberof BlastServiceJob
 */
BLAST_SERVICE_LOCAL BlastServiceJob *AllocateBlastServiceJob (Service *service_p, const char *job_name_s, const char *job_description_s, const char *tool_name_s, BlastServiceData *data_p);


/**
 * Free the BlastServiceJob.
 *
 * @param job_p The BlastServiceJob to free.
 * @memberof BlastServiceJob
 */
BLAST_SERVICE_LOCAL void FreeBlastServiceJob (ServiceJob *job_p);


/**
 * Get the filename with the data for a previously-ran BlastServiceJob.
 *
 * @param data_p The configuration data for the BlastService that ran the BlastServiceJob.
 * @param job_id_s A string containing the UUID for the BlastServiceJob.
 * @param suffix_s The file suffix to append.
 * @return The appropriate filename or <code>NULL</code> upon error.
 * @see BS_INPUT_SUFFIX_S
 * @see BS_OUTPUT_SUFFIX_S
 * @see BS_LOG_SUFFIX_S
 * @memberof BlastServiceJob
 */
BLAST_SERVICE_LOCAL char *GetPreviousJobFilename (const BlastServiceData *data_p, const char *job_id_s, const char *suffix_s);


/**
 * Save the BlastServiceJob to a persistent format that allows the BlastServiceJob
 * to be recreated in a potentially different thread and/or process. This is used to
 * save a BlastServiceJob in the JobsManager.
 *
 * @param base_job_p The BlastServiceJob.
 * @param value_length_p If the serialisation was successful, the value that this pointer
 * refers to will have the length of the serialised data in bytes.
 * @return The persistent representation of this BlastServiceJob or <code>NULL</code>
 * upon error.
 * @see JobsManager
 * @see SerialiseServiceJobToJSON
 * @see DeserialiseBlastServiceJobFromJSON
 * @memberof BlastServiceJob
 */
BLAST_SERVICE_LOCAL unsigned char *SerialiseBlastServiceJob (ServiceJob *base_job_p, unsigned int *value_length_p);


/**
 * Recreate a BlastServiceJob from a persistent format. This is used to
 * access a BlastServiceJob stored in the JobsManager.
 *
 * @param data_p The data representing the BlastServiceJob.
 * @param config_p A BlastServiceData object containing the Blast Service configuration details.
 * @return The BlastServiceJob or <code>NULL</code>
 * upon error.
 * @see JobsManager
 * @see SerialiseBlastServiceJob
 * @see DeserialiseServiceJobFromJSON
 * @memberof BlastServiceJob
 */
BLAST_SERVICE_LOCAL ServiceJob *DeserialiseBlastServiceJob (unsigned char *data_p, void *config_p);


/**
 * Deserialise a BlastServiceJob from a JSON fragment.
 *
 * @param blast_job_json_p The JSON fragment to generate the BlastServiceJob from .
 * @param config_p The configuration data for the Blast Service.
 * @return The newly-created BlastServiceJob or <code>NULL</code> upon error.
 * @memberof BlastServiceJob
 */
BLAST_SERVICE_LOCAL BlastServiceJob *GetBlastServiceJobFromJSON (const json_t *blast_job_json_p, BlastServiceData *config_p);


/**
 * Serialise a BlastServiceJob into a JSON fragment.
 *
 * @param job_p The BlastServiceJob to serialise
 * @param omit_results_flag <code>true</code> if the BlastServiceJob is to exclude
 * any results from the created JSON fragment, <code>false</code> to include them.
 * @return The newly-created JSON fragment or <code>NULL</code> upon error.
 * @memberof BlastServiceJob
 */
BLAST_SERVICE_LOCAL json_t *ConvertBlastServiceJobToJSON (BlastServiceJob *job_p, bool omit_results_flag);


/**
 * Add the logged information from a failed BlastServiceJob to its
 * list of stored errors.
 *
 * @param job_p The BlastServiceJob to update
 * @return <code>true</code> if the BlastServiceJob had its errors
 * updated successfully, <code>false</code> otherwise.
 * @memberof BlastServiceJob
 */
BLAST_SERVICE_LOCAL bool AddErrorToBlastServiceJob (BlastServiceJob *job_p);


/**
 * Update the running status of BlastServiceJob if needed.
 *
 * @param job_p The BlastServiceJob to check.
 * @return <code>true</code> if the BlastServiceJob was updated successfully,
 * <code>false</code> otherwise.
 * @memberof BlastServiceJob
 */
BLAST_SERVICE_LOCAL bool UpdateBlastServiceJob (ServiceJob *job_p);


/**
 * Process a LinkedService for a given BlastServiceJob.
 *
 * @param service_p The BlastService that generated the BlastServiceJob.
 * @param job_p The BlastServiceJob to process for LinkedSrevice results.
 * @param linked_service_p The LinkedService definition to produce the
 * values for.
 * @return <code>true</code> if the BlastServiceJob was processed
 * successfully, <code>false</code> otherwise.
 * @memberof BlastServiceJob
 */
BLAST_SERVICE_LOCAL bool ProcessLinkedServicesForBlastServiceJobOutput (Service *service_p, ServiceJob *job_p, LinkedService *linked_service_p);


/**
 * This is the callback function used by an AsyncTask that is called when a BlastServiceJob has
 * completed running. This is used when by AsyncSystemBlastTool objects.
 *
 * @param job_p The BlastServiceJob that has completed running.
 * @memberof BlastServiceJob
 */
BLAST_SERVICE_LOCAL void BlastServiceJobCompleted (ServiceJob *job_p);



#ifdef __cplusplus
}
#endif


#endif /* BLAST_SERVICE_JOB_H_ */

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
#include <string.h>

#define ALLOCATE_BLAST_SERVICE_CONSTANTS (1)
#include "blast_service.h"

#include "blastn_service.h"
#include "blastp_service.h"
#include "blastx_service.h"
#include "magic_blast_service.h"

#include "memory_allocations.h"

#include "service_job_set_iterator.h"
#include "string_utils.h"
#include "grassroots_server.h"
#include "temp_file.hpp"
#include "json_tools.h"
#include "blast_formatter.h"
#include "blast_service_job.h"
#include "paired_blast_service.h"
#include "blast_service_params.h"
#include "blast_tool_factory.hpp"
#include "jobs_manager.h"
#include "blast_service_job.h"
#include "blast_service_params.h"
#include "blast_service_job_markup.h"

#include "servers_manager.h"
#include "remote_parameter_details.h"
#include "audit.h"

#include "string_parameter.h"
#include "boolean_parameter.h"
#include "unsigned_int_parameter.h"


#ifdef _DEBUG
#define BLAST_SERVICE_DEBUG (STM_LEVEL_FINEST)
#else
#define BLAST_SERVICE_DEBUG (STM_LEVEL_NONE)
#endif


/***************************************/

static void InitBlastService (Service *blast_service_p);

static void RunJobs (Service *service_p, ParameterSet *param_set_p, const char *input_filename_s, BlastAppParameters *app_params_p, ServiceJobSetIterator *iterator_p);

static bool PreRunJobs (BlastServiceData *blast_data_p);

static bool CleanupAsyncBlastService (void *data_p);

static bool AddDatabaseForIndexing (const DatabaseInfo *db_p, json_t *json_p);

static char *ConfigureWorkingDirectoryPath (const json_t *blast_config_p);

static json_t *GetBlastIndexingDataPayload (GrassrootsServer *server_p, const char *service_s, const DatabaseInfo *db_p);

static json_t *GetIndexingDataForDatabase (const Service *service_p, const DatabaseInfo *db_p);




/*
 * API FUNCTIONS
 */
ServicesArray *GetServices (UserDetails * UNUSED_PARAM (user_p), GrassrootsServer *grassroots_p)
{
	ServicesArray *services_array_p = NULL;
	const uint32 NUM_SERVICES = 4;
	Service *services_pp [NUM_SERVICES];
	uint32 num_added_services = 0;
	uint32 i;

	memset (services_pp, 0, NUM_SERVICES * (sizeof (Service *)));

	*services_pp = GetBlastNService (grassroots_p);
	* (services_pp + 1) = GetBlastPService (grassroots_p);
	* (services_pp + 2) = GetBlastXService (grassroots_p);
	* (services_pp + 3) = GetMagicBlastService (grassroots_p);


	/*
	 * Loop through all of the Blast services
	 * and try and initialise them
	 */
	for (i = 0; i < NUM_SERVICES; ++ i)
		{
			Service *service_p = * (services_pp + i);

			if (service_p)
				{
					InitBlastService (service_p);
					++ num_added_services;
				}
		}

	/*
	 * If we have successfully got any Blast services,
	 * create and populate the ServicesArray that will
	 * contain them
	 */
	if (num_added_services > 0)
		{
			services_array_p = AllocateServicesArray (num_added_services);

			if (services_array_p)
				{
					Service **added_service_pp = services_array_p -> sa_services_pp;

					for (i = 0; i < NUM_SERVICES; ++ i)
						{
							if (* (services_pp + i))
								{
									*added_service_pp = * (services_pp + i);
									++ added_service_pp;
								}
						}

				}		/* if (services_array_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create ServicesArray for Blast services");

					for (i = 0; i < NUM_SERVICES; ++ i)
						{
							if (* (services_pp + i))
								{
									FreeService (* (services_pp + i));
								}
						}

				}

		}		/* if (num_added_services > 0) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create any Blast services");
		}

	return services_array_p;
}


void ReleaseServices (ServicesArray *services_p)
{
	FreeServicesArray (services_p);
}



void ReleaseBlastServiceParameters (Service * UNUSED_PARAM (service_p), ParameterSet *params_p)
{
	FreeParameterSet (params_p);
}




ServiceJobSet *RunBlastService (Service *service_p, ParameterSet *param_set_p, UserDetails *  UNUSED_PARAM (user_p), ProvidersStateTable *providers_p, BlastAppParameters *app_params_p)
{
	BlastServiceData *blast_data_p = (BlastServiceData *) (service_p -> se_data_p);
	const char *input_value_s = NULL;

#if BLAST_SERVICE_DEBUG >= STM_LEVEL_FINEST
	PrintErrors (STM_LEVEL_FINEST, __FILE__, __LINE__,  "Running the blast service with data at %.16X", blast_data_p);
#endif


	/*
	 * We will check for all of our parameters, such as previous job ids, etc. first, until
	 * we are left with the blast-specific parameters
	 */

	/* Are we retrieving previously run jobs? */
	if (GetCurrentStringParameterValueFromParameterSet (param_set_p, BS_JOB_ID.npt_name_s, &input_value_s))
		{
			if (!IsStringEmpty (input_value_s))
				{
					if (service_p -> se_synchronous != SY_SYNCHRONOUS)
						{
							service_p -> se_synchronous = SY_SYNCHRONOUS;
						}

					service_p -> se_jobs_p  = CreateJobsForPreviousResults (param_set_p, input_value_s, blast_data_p);
				}

			/*
			 * Since we are just checking the status of already running jobs,
			 * we are not running any jobs with this Service instance, so
			 * we can label it as running synchronously if it is not already
			 */


		}		/* if (GetParameterValueFromParameterSet (param_set_p, BS_JOB_ID.npt_name_s, &param_value, true)) */

	if (! (service_p -> se_jobs_p))
		{
			service_p -> se_jobs_p = AllocateServiceJobSet (service_p);

			if (service_p -> se_jobs_p)
				{
					RunPairedServices (service_p, param_set_p, providers_p, SaveRemoteBlastJobDetails);

					/* Get all of the selected databases and create a BlastServiceJob for each one */
					PrepareBlastServiceJobs (blast_data_p -> bsd_databases_p, param_set_p, service_p, blast_data_p);

					if (GetServiceJobSetSize (service_p -> se_jobs_p) > 0)
						{
							ServiceJobSetIterator iterator;
							BlastServiceJob *job_p = NULL;

							InitServiceJobSetIterator (&iterator, service_p -> se_jobs_p);

							job_p = (BlastServiceJob *) GetNextServiceJobFromServiceJobSetIterator (&iterator);

							if (job_p)
								{
									TempFile *input_p = GetInputTempFile (param_set_p, blast_data_p -> bsd_working_dir_s, job_p -> bsj_job.sj_id);

									if (input_p)
										{
											const char *input_filename_s = input_p -> GetFilename();

											if (input_filename_s)
												{
													if (PreRunJobs (blast_data_p))
														{
															/* Rewind the ServiceJobIterator */
															InitServiceJobSetIterator (&iterator, service_p -> se_jobs_p);

															RunJobs (service_p, param_set_p, input_filename_s, app_params_p, &iterator);
														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "PreRunJobs for Blast Service failed");
														}

												}		/* if (input_filename_s) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get input filename for blast tool \"%s\"", job_p -> bsj_job.sj_name_s);
												}

											delete input_p;
										}		/* if (input_p) */
									else
										{
											const char * const error_s = "Failed to create input temp file for blast tool";
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create input temp file for blast tool \"%s\" in \"%s\"", job_p -> bsj_job.sj_name_s, blast_data_p -> bsd_working_dir_s);

											/* Since we couldn't save the input sequence, all jobs need to be set to have an error status */
											if (!AddGeneralErrorMessageToServiceJob (& (job_p -> bsj_job), error_s))
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add error to job", job_p -> bsj_job.sj_name_s);
												}

											SetServiceJobStatus (& (job_p -> bsj_job), OS_FAILED_TO_START);

											while ( (job_p = (BlastServiceJob *) GetNextServiceJobFromServiceJobSetIterator (&iterator)) != NULL)
												{
													if (!AddGeneralErrorMessageToServiceJob (& (job_p -> bsj_job), error_s))
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add error to job", job_p -> bsj_job.sj_name_s);
														}

													SetServiceJobStatus (& (job_p -> bsj_job), OS_FAILED_TO_START);
												}

										}

								}


						}		/* if (GetServiceJobSetSize (service_p -> se_jobs_p) > 0) */

					if (GetServiceJobSetSize (service_p -> se_jobs_p) == 0)
						{
							PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "No jobs specified");
						}

				}		/* if (service_p -> se_jobs_p) */

		}		/* if ((GetParameterValueFromParameterSet (param_set_p, TAG_BLAST_JOB_ID, &param_value, t rue)) && (!IsStringEmpty (param_value.st_string_value_s))) else */

	return service_p -> se_jobs_p;
}



ServiceJobSet *CreateJobsForPreviousResults (ParameterSet *params_p, const char *ids_s, BlastServiceData *blast_data_p)
{
	LinkedList *ids_p = GetUUIDSList (ids_s);
	ServiceJobSet *jobs_p = NULL;

	if (ids_p)
		{
			const uint32 *fmt_code_p = NULL;
			uint32 output_format_code = BS_DEFAULT_OUTPUT_FORMAT;
			const char *output_format_param_s = NULL;

			GetCurrentStringParameterValueFromParameterSet (params_p, BS_CUSTOM_OUTPUT_FORMAT.npt_name_s, &output_format_param_s);

			if (GetCurrentUnsignedIntParameterValueFromParameterSet (params_p, BS_OUTPUT_FORMAT.npt_name_s, &fmt_code_p))
				{
					if (fmt_code_p)
						{
							output_format_code = *fmt_code_p;
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Couldn't get requested output format code, using " UINT32_FMT " instead", output_format_code);
				}


			jobs_p = GetPreviousJobResults (ids_p, blast_data_p, output_format_code, output_format_param_s);

			if (jobs_p)
				{
					if (output_format_code == BOF_GRASSROOTS)
						{
							ServiceJobNode *job_node_p = (ServiceJobNode *) (jobs_p -> sjs_jobs_p -> ll_head_p);

							while (job_node_p)
								{
									ServiceJob *job_p = job_node_p -> sjn_job_p;
									json_t *results_p = MarkUpBlastResult ( (BlastServiceJob *) job_p);

									if (!ReplaceServiceJobResults (job_p, results_p))
										{
											char uuid_s [UUID_STRING_BUFFER_SIZE];

											ConvertUUIDToString (job_p -> sj_id, uuid_s);
											PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, results_p, "Failed to replace job results for \"%s\" with id %s", job_p -> sj_name_s, uuid_s);
										}

									job_node_p = (ServiceJobNode *) (job_node_p -> sjn_node.ln_next_p);
								}
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get ServiceJobSet for previously run blast job \"%s\"", ids_s);
				}

			FreeLinkedList (ids_p);
		}		/* if (ids_p) */
	else
		{
			Service *service_p = blast_data_p -> bsd_base_data.sd_service_p;
			jobs_p = AllocateServiceJobSet (service_p);

			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to parse \"%s\" to get uuids", ids_s);

			if (jobs_p)
				{
					ServiceJob *job_p = CreateAndAddServiceJobToService (service_p, ids_s, "Failed UUID lookup", NULL, NULL, NULL);

					if (job_p)
						{
							bool added_error_flag = false;
							char *errors_s = ConcatenateVarargsStrings ("Failed to parse \"", ids_s, "\" to get uuids", NULL);

							if (errors_s)
								{
									added_error_flag = AddParameterErrorMessageToServiceJob (job_p, BS_JOB_ID.npt_name_s, BS_JOB_ID.npt_type, errors_s);
									FreeCopiedString (errors_s);
								}
							else
								{
									added_error_flag = AddParameterErrorMessageToServiceJob (job_p, BS_JOB_ID.npt_name_s, BS_JOB_ID.npt_type, "Failed to parse uuids");
								}

							if (!added_error_flag)
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add errors to job for \"%s\" uuids", ids_s);
								}

						}		/* if (job_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add failed job for \"%s\" uuids", ids_s);
							FreeServiceJobSet (jobs_p);
							jobs_p = NULL;
						}

				}		/* if (service_p -> se_jobs_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate job set");
				}
		}

	return jobs_p;
}


bool CloseBlastService (Service *service_p)
{
	BlastServiceData *blast_data_p = (BlastServiceData *) (service_p -> se_data_p);

	FreeBlastServiceData (blast_data_p);

	return true;
}


bool AddBaseBlastServiceParameters (Service *blast_service_p, ParameterSet *param_set_p, Resource *resource_p, const DatabaseType db_type, AddAdditionalParamsFn add_additional_params_fn, void *callback_data_p)
{
	bool success_flag = false;
	BlastServiceData *blast_data_p = (BlastServiceData *) (blast_service_p -> se_data_p);

	if (AddQuerySequenceParams (blast_data_p, param_set_p, add_additional_params_fn, callback_data_p))
		{
			uint16 num_dbs = AddDatabaseParams (blast_data_p, param_set_p, resource_p, db_type);

			success_flag = AddPairedServiceParameters (blast_service_p, param_set_p);
		}

	return success_flag;
}



bool GetBaseBlastServiceParameterTypeForNamedParameter (const Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (!GetQuerySequenceParameterTypeForNamedParameter (param_name_s, pt_p))
		{
			BlastServiceData *data_p = (BlastServiceData *) (service_p -> se_data_p);

			if (!GetDatabaseParameterTypeForNamedParameter (data_p, param_name_s, pt_p))
				{
					if (!GetPairedServiceParameterTypeForNamedParameter (service_p, param_name_s, pt_p))
						{
							success_flag = false;
						}		/* if (!GetPairedServiceParameterTypeForNamedParameter (param_name_s, pt_p)) */

				}		/* if (!GetDatabaseParameterTypeForNamedParameter (param_name_s, pt_p)) */

		}		/* if (!GetQuerySequenceParameterTypeForNamedParameter (param_name_s, pt_p)) */

	return success_flag;
}



TempFile *GetInputTempFile (const ParameterSet *params_p, const char *working_directory_s, const uuid_t id)
{
	TempFile *input_file_p = NULL;
	const char *sequence_s = NULL;


	/* Input query */
	if (GetCurrentStringParameterValueFromParameterSet (params_p, BS_INPUT_QUERY.npt_name_s, &sequence_s))
		{
			if (!IsStringEmpty (sequence_s))
				{
					input_file_p = TempFile :: GetTempFile (working_directory_s, id, BS_INPUT_SUFFIX_S);

					if (input_file_p)
						{
							bool success_flag = true;

							if (!input_file_p -> IsOpen())
								{
									success_flag = input_file_p -> Open ("w");
								}

							if (success_flag)
								{
									success_flag = input_file_p -> Print (sequence_s);
									input_file_p -> Close();
								}

							if (!success_flag)
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Blast service failed to write to temp file \"%s\" for query \"%s\"", input_file_p -> GetFilename(), sequence_s);
									delete input_file_p;
									input_file_p = NULL;
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Blast service failed to open temp file for query \"%s\"", sequence_s);
						}

				}		/* if (!IsStringEmpty (sequence_s)) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get uuid as string");
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Blast input query is empty");
		}

	return input_file_p;
}


ServiceJobSet *GetPreviousJobResults (LinkedList *ids_p, BlastServiceData *blast_data_p, const uint32 output_format_code, const char *output_format_params_s)
{
	char *error_s = NULL;
	Service *service_p = blast_data_p -> bsd_base_data.sd_service_p;
	ServiceJobSet *jobs_p = AllocateServiceJobSet (service_p);

	if (jobs_p)
		{
			BlastServiceJob *blast_job_p = AllocateBlastServiceJob (jobs_p -> sjs_service_p, "Previous Blast Results", NULL, "Previous Blast Results", blast_data_p);

			if (blast_job_p)
				{
					ServiceJob *job_p = (ServiceJob *) blast_job_p;

					if (AddServiceJobToService (service_p, (ServiceJob *) job_p))
						{
							uuid_t job_id;
							StringListNode *node_p = (StringListNode *) (ids_p -> ll_head_p);
							uint32 num_successful_jobs = 0;
							OperationStatus status;

							while (node_p)
								{
									const char * const job_id_s = node_p -> sln_string_s;

									if (uuid_parse (job_id_s, job_id) == 0)
										{
											char *result_s = GetBlastResultByUUIDString (blast_data_p, job_id_s, (output_format_code != BOF_GRASSROOTS) ? output_format_code : (uint32) BOF_SINGLE_FILE_JSON_BLAST, output_format_params_s);
											SetServiceJobStatus (job_p, OS_FAILED);

											if (result_s)
												{
													json_t *result_json_p = NULL;

													if (output_format_code == BOF_GRASSROOTS)
														{
															json_error_t err;
															json_t *temp_p  = json_loads (result_s, 0, &err);

															/*
															 * Convert the blast json to our markup and then
															 * get the result
															 */
															if (temp_p)
																{
																	result_json_p = ConvertBlastResultToGrassrootsMarkUp (temp_p, blast_data_p);

																	json_decref (temp_p);
																}
														}
													else
														{
															result_json_p = json_string (result_s);
														}


													if (result_json_p)
														{
															json_t *blast_result_json_p = GetResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, job_id_s, result_json_p);

															if (blast_result_json_p)
																{
																	if (AddResultToServiceJob (job_p, blast_result_json_p))
																		{
																			++ num_successful_jobs;
																		}
																	else
																		{
																			error_s = ConcatenateVarargsStrings ("Failed to add blast result \"", job_id_s, "\" to json results array", NULL);
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add blast result \"%s\" to json results array", job_id_s);
																		}
																}
															else
																{
																	error_s = ConcatenateVarargsStrings ("Failed to get full blast result as json \"", job_id_s, "\"", NULL);
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get full blast result as json \"%s\"", job_id_s);
																}

															json_decref (result_json_p);
														}
													else
														{
															error_s = ConcatenateVarargsStrings ("Failed to get blast result as json \"", job_id_s, "\"", NULL);
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get blast result as json \"%s\"", job_id_s);
														}

													FreeCopiedString (result_s);
												}		/* if (result_s) */
											else
												{
													error_s = ConcatenateVarargsStrings ("Failed to get blast result for \"", job_id_s, "\"", NULL);
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get blast result for \"%s\"", job_id_s);
												}
										}		/* if (uuid_parse (param_value.st_string_value_s, job_id) == 0) */
									else
										{
											error_s = ConcatenateVarargsStrings ("Failed to convert \"", job_id_s, "\" to a valid uuid", NULL);
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to convert \"%s\" to a valid uuid", job_id_s);
										}

									if (error_s)
										{
											if (!AddParameterErrorMessageToServiceJob (job_p, BS_JOB_ID.npt_name_s, BS_JOB_ID.npt_type, error_s))
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create json error string for \"%s\"", job_id_s);
												}

											FreeCopiedString (error_s);
										}		/* if (error_s) */


									node_p = (StringListNode *) (node_p -> sln_node.ln_next_p);
								}		/* while (node_p) */

#if BLAST_SERVICE_DEBUG >= STM_LEVEL_FINE
							PrintLog (STM_LEVEL_FINE, __FILE__, __LINE__, "Num input jobs " UINT32_FMT " num successful json results " UINT32_FMT, ids_p -> ll_size, num_successful_jobs);
#endif

							if (num_successful_jobs == ids_p -> ll_size)
								{
									status = OS_SUCCEEDED;
								}
							else if (num_successful_jobs == 0)
								{
									status = OS_FAILED;
								}
							else
								{
									status = OS_PARTIALLY_SUCCEEDED;
								}

							SetServiceJobStatus (job_p, status);

						}		/* if (AddServiceJobToService (jservice_p, (ServiceJob *) job_p)) */

				}		/* if (job_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate service job");
				}

		}		/* if (jobs_p) */

	return jobs_p;
}



void PrepareBlastServiceJobs (const DatabaseInfo *db_p, const ParameterSet * const param_set_p, Service *service_p, BlastServiceData *data_p)
{
	GrassrootsServer *grassroots_p = GetGrassrootsServerFromService (service_p);
	char *group_s = GetLocalDatabaseGroupName (grassroots_p);

	if (db_p)
		{
			while (db_p -> di_name_s)
				{
					char *full_db_name_s = GetFullyQualifiedDatabaseName (group_s, db_p -> di_name_s);

					if (full_db_name_s)
						{
							const bool *db_flag_p = NULL;

							/* Do we have a matching parameter? */
							if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, full_db_name_s, &db_flag_p))
								{
									/* Is the database selected to search against? */
									if ( (db_flag_p != NULL) && (*db_flag_p == true))
										{
											BlastServiceJob *job_p = AllocateBlastServiceJobForDatabase (service_p, db_p, data_p);

											if (!job_p)
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create ServiceJob for \"%s\"", db_p -> di_name_s);
												}

										}
								}		/* if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, full_db_name_s, &db_flag_p)) */

							FreeCopiedString (full_db_name_s);
						}		/* if (full_db_name_s) */

					++ db_p;
				}		/* while (db_p) */

		}		/* if (db_p) */


	if (group_s)
		{
			FreeCopiedString (group_s);
		}
}


ParameterSet *IsResourceForBlastService (Service *service_p, Resource *resource_p, Handler * UNUSED_PARAM (handler_p))
{
	ParameterSet *params_p = NULL;

	if (strcmp (resource_p -> re_protocol_s, PROTOCOL_FILE_S) == 0)
		{
			/*
			 * @TODO
			 * We could check if the file is on a remote filesystem and if so
			 * make a full or partial local copy for analysis.
			 */

			/*
			 * We can check on file extension and also the content of the file
			 * to determine if we want to blast this file.
			 */
			if (resource_p -> re_value_s)
				{
					const char *extension_s = Stristr (resource_p -> re_value_s, ".");

					if (extension_s)
						{
							/* move past the . */
							++ extension_s;

							/* check that the file doesn't end with the . */
							if (*extension_s != '\0')
								{
									if (strcmp (extension_s, "fa") == 0)
										{
											/* Get the parameters */
										}

								}		/* if (*extension_s != '\0') */

						}		/* if (extension_s) */

				}		/* if (filename_s) */

		}		/* if (strcmp (resource_p -> re_protocol_s, PROTOCOL_FILE_S)) */
	else if (strcmp (resource_p -> re_protocol_s, PROTOCOL_TEXT_S) == 0)
		{
			if (resource_p -> re_value_s)
				{
					BlastServiceData *blast_data_p = (BlastServiceData *) service_p -> se_data_p;
					DatabaseInfo *db_p = blast_data_p -> bsd_databases_p;

					/*
					 * Scroll through the databases and see if the phrase is in either the
					 * database title or description
					 */
					if (db_p)
						{
							params_p = GetServiceParameters (service_p, resource_p, NULL);

							if (params_p)
								{
									bool matched_db_flag = false;

									db_p = blast_data_p -> bsd_databases_p;

									while (db_p -> di_name_s)
										{
											Parameter *param_p = GetParameterFromParameterSetByName (params_p, db_p -> di_name_s);

											if (param_p)
												{
													if (IsBooleanParameter (param_p))
														{
															/* Set the matching databases to active */
															bool active_flag = false;

															if ( (Stristr (db_p -> di_name_s, resource_p -> re_value_s)) ||
															     (Stristr (db_p -> di_description_s, resource_p -> re_value_s)))
																{
																	active_flag = true;
																	matched_db_flag = true;
																}

															if (!SetBooleanParameterCurrentValue ( (BooleanParameter *) param_p, &active_flag))
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set Parameter \"%s\" to true", param_p -> pa_name_s);
																}
														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Parameter \"%s\" is of type \"%s\" not boolean", param_p -> pa_name_s, GetGrassrootsTypeAsString (param_p -> pa_type));
														}
												}		/* if (param_p) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to find parameter \"%s\"", db_p -> di_name_s);
												}

											++ db_p;
										}		/* while (db_p -> di_name_s) */

									if (!matched_db_flag)
										{
											ReleaseServiceParameters (service_p, params_p);
											params_p = NULL;
										}

								}		/* if (params_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetBlastServiceParameters failed");
								}

						}		/* if (db_p) */

				}		/* if (resource_p -> re_value_s) */

		}		/* else if (strcmp (resource_p -> re_protocol_s, PROTOCOL_INLINE_S)) */


	return params_p;
}


OperationStatus GetBlastServiceStatus (Service *service_p, const uuid_t job_id)
{
	OperationStatus status = OS_ERROR;
	BlastServiceJob *job_p = NULL;
	char uuid_s [UUID_STRING_BUFFER_SIZE];

	ConvertUUIDToString (job_id, uuid_s);

	if (service_p -> se_jobs_p)
		{
			job_p = (BlastServiceJob *) GetServiceJobFromServiceJobSetById (service_p -> se_jobs_p, job_id);
		}

	if (!job_p)
		{
			GrassrootsServer *grassroots_p = GetGrassrootsServerFromService (service_p);
			JobsManager *jobs_manager_p = GetJobsManager (grassroots_p);

			if (jobs_manager_p)
				{
					ServiceJob *previous_job_p = GetServiceJobFromJobsManager (jobs_manager_p, job_id);

					if (previous_job_p)
						{
							job_p = (BlastServiceJob *) previous_job_p;
						}
				}		/* if (jobs_manager_p) */
			else
				{

				}

		}		/* if (!job_p) */


	if (job_p)
		{
			status = GetCachedServiceJobStatus (& (job_p -> bsj_job));
		}		/* if (job_p) */
	else
		{

		}

	return status;
}




bool DetermineBlastResult (BlastServiceJob *job_p)
{
	bool success_flag = false;
	Service *service_p = job_p -> bsj_job.sj_service_p;
	BlastTool *tool_p = job_p -> bsj_tool_p;
	BlastServiceData *blast_data_p = (BlastServiceData *) (service_p -> se_data_p);
	char uuid_s [UUID_STRING_BUFFER_SIZE];
	json_t *result_json_p = NULL;
	OperationStatus status = GetServiceJobStatus (& (job_p -> bsj_job));


	ConvertUUIDToString (job_p -> bsj_job.sj_id, uuid_s);

	if (status == OS_SUCCEEDED)
		{
			uint32 out_fmt = tool_p -> GetOutputFormat();

			if (out_fmt == BOF_GRASSROOTS)
				{
					result_json_p = MarkUpBlastResult (job_p);
				}		/* if (out_fmt == BOF_GRASSROOTS) */
			else
				{
					char *result_s = tool_p -> GetResults (blast_data_p -> bsd_formatter_p);

					if (result_s)
						{
							result_json_p = json_string (result_s);
							FreeCopiedString (result_s);
						}
				}

			if (result_json_p)
				{
					json_t *blast_result_json_p = GetResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, uuid_s, result_json_p);

					if (blast_result_json_p)
						{
							if (AddResultToServiceJob (& (job_p -> bsj_job), blast_result_json_p))
								{
									success_flag = true;
								}
							else
								{
									json_decref (blast_result_json_p);
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to append blast result for \"%s\"", uuid_s);
								}

						}		/* if (blast_result_json_p) */

					json_decref (result_json_p);
				}		/* if (result_json_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get blast result for \"%s\"", uuid_s);
				}

		}		/* if (status == OS_SUCCEEDED) */
	else
		{
			success_flag = true;
		}

	return success_flag;
}


char *GetBlastResultByUUID (const BlastServiceData *data_p, const uuid_t job_id, const uint32 output_format_code, const char *output_format_params_s)
{
	char job_id_s [UUID_STRING_BUFFER_SIZE];
	char *result_s = NULL;

	ConvertUUIDToString (job_id, job_id_s);

	result_s = GetBlastResultByUUIDString (data_p, job_id_s, output_format_code, output_format_params_s);

	return result_s;
}



char *GetBlastResultByUUIDString (const BlastServiceData *data_p, const char *job_id_s, const uint32 output_format_code, const char *output_format_params_s)
{
	char *result_s = NULL;
	char *job_output_filename_s = GetPreviousJobFilename (data_p, job_id_s, BS_OUTPUT_SUFFIX_S);

	if (job_output_filename_s)
		{
			/* Does the file already exist? */
			char *converted_filename_s = BlastFormatter :: GetConvertedOutputFilename (job_output_filename_s, output_format_code);

			if (converted_filename_s)
				{
					FILE *job_f = fopen (converted_filename_s, "r");

					if (job_f)
						{
							result_s = GetFileContentsAsString (job_f);

							if (!result_s)
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Couldn't get content of job file \"%s\"", job_output_filename_s);
								}

							if (fclose (job_f) != 0)
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Couldn't close job file \"%s\"", job_output_filename_s);
								}
						}		/* if (job_f) */

					FreeCopiedString (converted_filename_s);
				}		/* if (converted_filename_s) */

			if (!result_s)
				{
					/*
					 * We haven't got the output in the desired output format so we need to run the formatter.
					 */

					/* Is it a local job? */
					if (IsPathValid (job_output_filename_s))
						{
							if (data_p -> bsd_formatter_p)
								{
									result_s = data_p -> bsd_formatter_p -> GetConvertedOutput (job_id_s, output_format_code, output_format_params_s, data_p);
								}		/* if (data_p -> bsd_formatter_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No formatter specified");
								}

						}		/* if (IsPathValid (job_output_filename_s)) */

				}		/* if (!result_s) */

			FreeCopiedString (job_output_filename_s);
		}		/* if (job_output_filename_s) */


	if (!result_s)
		{
			/* Is it a remote job? */
			result_s = GetPreviousRemoteBlastServiceJob (job_id_s, output_format_code, data_p);
		}

	return result_s;
}


json_t *BuildBlastServiceJobJSON (Service * UNUSED_PARAM (service_p), ServiceJob *service_job_p, bool omit_results_flag)
{
	json_t *res_p = NULL;

	if (strcmp (service_job_p -> sj_type_s, BSJ_TYPE_S) == 0)
		{
			res_p = ConvertBlastServiceJobToJSON ( (BlastServiceJob *) service_job_p, omit_results_flag);
		}
	else if (strcmp (service_job_p -> sj_type_s, RSJ_TYPE_S) == 0)
		{
			res_p = GetRemoteServiceJobAsJSON ( (RemoteServiceJob *) service_job_p, omit_results_flag);
		}
	else
		{
			char uuid_s [UUID_STRING_BUFFER_SIZE];

			ConvertUUIDToString (service_job_p -> sj_id, uuid_s);
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "BuildBlastServiceJobJSON failed, unknown type \"%s\" for \"%s\"", service_job_p -> sj_type_s, uuid_s);
		}

	return res_p;
}



BlastServiceData *AllocateBlastServiceData (Service * UNUSED_PARAM (blast_service_p), DatabaseType database_type)
{
	BlastServiceData *data_p = (BlastServiceData *) AllocMemory (sizeof (BlastServiceData));

	if (data_p)
		{
			memset (& (data_p -> bsd_base_data), 0, sizeof (ServiceData));

			data_p -> bsd_working_dir_s = NULL;
			data_p -> bsd_databases_p = NULL;
			data_p -> bsd_formatter_p = NULL;
			data_p -> bsd_tool_factory_p = NULL;
			data_p -> bsd_type = database_type;
			data_p -> bsd_task_manager_p = NULL;
		}


#if BLAST_SERVICE_DEBUG >= STM_LEVEL_FINEST
	PrintErrors (STM_LEVEL_FINEST, __FILE__, __LINE__,  "Allocating the blast service data at %.16X", data_p);
#endif

	return data_p;
}


ServiceJob *BuildBlastServiceJob (struct Service *service_p, const json_t *service_job_json_p)
{
	ServiceJob *job_p = NULL;
	BlastServiceData *config_p = (BlastServiceData*) (service_p -> se_data_p);

	if (IsRemoteServiceJobJSON (service_job_json_p))
		{
			GrassrootsServer *grassroots_p = GetGrassrootsServerFromService (service_p);
			RemoteServiceJob *remote_job_p = GetRemoteServiceJobFromJSON (service_job_json_p, service_p, grassroots_p);

			if (remote_job_p)
				{

					job_p = & (remote_job_p -> rsj_job);
				}		/* if (remote_job_p) */

		}		/* if (IsRemoteServiceJobJSON (service_job_json_p)) */
	else
		{
			BlastServiceJob *blast_job_p = GetBlastServiceJobFromJSON (service_job_json_p, config_p);

			if (blast_job_p)
				{
					BlastTool *tool_p = blast_job_p -> bsj_tool_p;

					OperationStatus old_status = GetCachedServiceJobStatus (& (blast_job_p -> bsj_job));
					OperationStatus current_status = tool_p -> GetStatus();

					if (old_status != current_status)
						{
							switch (current_status)
								{
									case OS_SUCCEEDED:
									case OS_PARTIALLY_SUCCEEDED:
									{
										if (! (blast_job_p -> bsj_job.sj_result_p))
											{
												if (!DetermineBlastResult (blast_job_p))
													{
														char job_id_s [UUID_STRING_BUFFER_SIZE];

														ConvertUUIDToString (blast_job_p -> bsj_job.sj_id, job_id_s);
														PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get results for %s", job_id_s);
													}
											}
									}
									break;

									case OS_FAILED:
									case OS_FAILED_TO_START:
									{
										AddErrorToBlastServiceJob (blast_job_p);
									}
									break;

									default:
										break;
								}
						}

					job_p = & (blast_job_p -> bsj_job);
				}		/* if (blast_job_p) */

		}


	return job_p;
}


void CustomiseBlastServiceJob (Service * UNUSED_PARAM (service_p), ServiceJob *job_p)
{
	job_p -> sj_update_fn = UpdateBlastServiceJob;
	job_p -> sj_free_fn = FreeBlastServiceJob;
}




bool GetBlastServiceConfig (BlastServiceData *data_p)
{
	bool success_flag = false;
	const json_t *blast_config_p = data_p -> bsd_base_data.sd_config_p;

	if (blast_config_p)
		{
			if ( (data_p -> bsd_working_dir_s = ConfigureWorkingDirectoryPath (blast_config_p)) != NULL)
				{
					json_t *value_p = json_object_get (blast_config_p, BS_DATABASES_S);

					if (value_p)
						{
							if (json_is_array (value_p))
								{
									size_t i = json_array_size (value_p);
									DatabaseInfo *databases_p = (DatabaseInfo *) AllocMemoryArray (i + 1, sizeof (DatabaseInfo));

									if (databases_p)
										{
											json_t *db_json_p;
											DatabaseInfo *db_p = databases_p;

											json_array_foreach (value_p, i, db_json_p)
											{
												const char *filename_s = GetJSONString (db_json_p, "filename");

												if (filename_s)
													{
														const char *name_s = GetJSONString (db_json_p, "name");

														if (name_s)
															{
																const char *description_s = GetJSONString (db_json_p, "description");

																if (description_s)
																	{
																		const char *type_s = GetJSONString (db_json_p, "type");
																		const char *download_uri_s = GetJSONString (db_json_p, "download_uri");
																		const char *info_uri_s = GetJSONString (db_json_p, "info_uri");
																		const char *scaffold_regex_s = GetJSONString (db_json_p, "scaffold_regex");
																		const char *scaffold_key_s = GetJSONString (db_json_p, "scaffold_key");
																		const char *search_description_s = GetJSONString (db_json_p, "search_description");

																		db_p -> di_name_s = name_s;
																		db_p -> di_filename_s = filename_s;
																		db_p -> di_description_s = description_s;
																		db_p -> di_search_description_s = search_description_s;
																		db_p -> di_download_uri_s = download_uri_s;
																		db_p -> di_info_uri_s = info_uri_s;
																		db_p -> di_active_flag = true;
																		db_p -> di_type = DT_NUCLEOTIDE;
																		db_p -> di_scaffold_key_s = scaffold_key_s ? scaffold_key_s : "id";
																		db_p -> di_scaffold_regex_s = scaffold_regex_s;

																		GetJSONBoolean (db_json_p, "active", & (db_p -> di_active_flag));

																		if (type_s)
																			{
																				if (strcmp (type_s, "protein") == 0)
																					{
																						db_p -> di_type = DT_PROTEIN;
																					}
																			}

																		success_flag = true;
																		++ db_p;
																	}		/* if (description_s) */
																else
																	{
																		PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, db_json_p, "Failed to add database, no description key");
																	}

															}		/* if (name_s) */
														else
															{
																PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, db_json_p, "Failed to add database, no name key");
															}

													}		/* if (filename_s) */
												else
													{
														PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, db_json_p, "Failed to add database, no filename key");
													}


											}		/* json_array_foreach (value_p, i, db_json_p) */

											if (success_flag)
												{
													data_p -> bsd_databases_p = databases_p;
												}
											else
												{
													FreeMemory (databases_p);
												}

										}		/* if (databases_p) */

									if (success_flag)
										{
											data_p -> bsd_tool_factory_p = BlastToolFactory :: GetBlastToolFactory (blast_config_p);

											if (data_p -> bsd_tool_factory_p)
												{
													Service *service_p = data_p -> bsd_base_data.sd_service_p;

													service_p -> se_synchronous = GetBlastToolFactorySynchronicity (data_p -> bsd_tool_factory_p);

													if (service_p -> se_synchronous == SY_ASYNCHRONOUS_ATTACHED)
														{
															data_p -> bsd_task_manager_p = AllocateAsyncTasksManager (GetServiceName (service_p), CleanupAsyncBlastService, service_p);

															if (data_p -> bsd_task_manager_p)
																{
																	SetServiceReleaseFunction (service_p, ReleaseBlastService);
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AllocateAsyncTasksManager failed");
																	success_flag = false;
																}
														}

												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetBlastToolFactory failed");
													success_flag = false;
												}
										}

								}		/* if (json_is_array (value_p)) */

						}		/* if (value_p) */

				}		/* 				if ((data_p -> bsd_working_dir_s = ConfigureWorkingDirectoryPath (blast_config_p)) != NULL) */

			if (success_flag)
				{
					const char *value_s = GetJSONString (blast_config_p, "blast_formatter");

					if (value_s)
						{
							if (strcmp (value_s, "system") == 0)
								{
									const json_t *formatter_config_p = json_object_get (blast_config_p, "system_formatter_config");

									data_p -> bsd_formatter_p = SystemBlastFormatter :: Create (formatter_config_p);
								}
							else
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Unknown BlastFormatter type \"%s\"", value_s);
								}
						}
				}

		}		/* if (blast_config_p) */

	return success_flag;
}



void FreeBlastServiceData (BlastServiceData *data_p)
{
#if BLAST_SERVICE_DEBUG >= STM_LEVEL_FINEST
	PrintErrors (STM_LEVEL_FINEST, __FILE__, __LINE__,  "Freeing the blast service data at %.16X", data_p);
#endif

	if (data_p -> bsd_databases_p)
		{
			FreeMemory (data_p -> bsd_databases_p);
		}

	if (data_p -> bsd_formatter_p)
		{
			delete (data_p -> bsd_formatter_p);
		}

	if (data_p -> bsd_tool_factory_p)
		{
			delete (data_p -> bsd_tool_factory_p);
		}

	if (data_p -> bsd_working_dir_s)
		{
			FreeCopiedString (data_p -> bsd_working_dir_s);
		}


	if (data_p -> bsd_task_manager_p)
		{
			if (!IsAsyncTaskManagerRunning (data_p -> bsd_task_manager_p))
				{
					FreeAsyncTasksManager (data_p -> bsd_task_manager_p);
				}
		}

	FreeMemory (data_p);
}


ServiceMetadata *GetGeneralBlastServiceMetadata (Service *service_p)
{
	const char *term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "operation_0491";
	SchemaTerm *category_p = AllocateSchemaTerm (term_url_s, "Pairwise sequence alignment", "Methods might perform one-to-one, one-to-many or many-to-many comparisons. Align exactly two molecular sequences.");

	if (category_p)
		{
			ServiceMetadata *metadata_p = AllocateServiceMetadata (category_p, NULL);

			if (metadata_p)
				{
					SchemaTerm *output_p;

					term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "format_1333";
					output_p = AllocateSchemaTerm (term_url_s, "BLAST results", "Format of results of a sequence database search using some variant of BLAST. This includes score data, alignment data and summary table.");

					if (output_p)
						{
							if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
								{
									return metadata_p;
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
									FreeSchemaTerm (output_p);
								}

						}		/* if (output_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
						}

				}		/* if (metadata_p) */

		}		/* if (category_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate category term %s for service metadata", term_url_s);
		}

	return NULL;
}



void ReleaseBlastService (Service *service_p)
{
	BlastServiceData *data_p = (BlastServiceData *) service_p -> se_data_p;

	if (data_p -> bsd_task_manager_p)
		{
			IncrementAsyncTaskManagerCount (data_p -> bsd_task_manager_p);
		}		/* if (data_p -> bsd_task_manager_p) */
}



/*
 * STATIC FUNCTIONS
 */


static void InitBlastService (Service *blast_service_p)
{
	blast_service_p -> se_synchronous = GetBlastToolFactorySynchronicity ( ( (BlastServiceData *) (blast_service_p -> se_data_p)) -> bsd_tool_factory_p);

	blast_service_p -> se_deserialise_job_json_fn = BuildBlastServiceJob;
	blast_service_p -> se_serialise_job_json_fn = BuildBlastServiceJobJSON;

	blast_service_p -> se_process_linked_services_fn = ProcessLinkedServiceForBlastServiceJobOutput;
}


const char *GetMatchingDatabaseFilename (const BlastServiceData *data_p, const char *name_s)
{
	const DatabaseInfo *db_p = data_p -> bsd_databases_p;

	if (db_p)
		{
			while (db_p -> di_name_s)
				{
					if (strcmp (name_s, db_p -> di_name_s) == 0)
						{
							return db_p -> di_filename_s;
						}

					++ db_p;

				}		/* while (db_p -> di_name_s) */

		}		/* if (db_p) */

	return NULL;
}


const char *GetMatchingDatabaseName (const BlastServiceData *data_p, const char *filename_s)
{
	const char *db_name_s = NULL;
	const DatabaseInfo *db_p = GetMatchingDatabaseByFilename (data_p, filename_s);

	if (db_p)
		{
			db_name_s = db_p -> di_name_s;
		}		/* if (db_p) */

	return db_name_s;
}


const DatabaseInfo *GetMatchingDatabaseByFilename (const BlastServiceData *data_p, const char *filename_s)
{
	const DatabaseInfo *db_p = data_p -> bsd_databases_p;

	if (db_p)
		{
			while (db_p -> di_filename_s)
				{
					if (strcmp (filename_s, db_p -> di_filename_s) == 0)
						{
							return db_p;
						}

					++ db_p;

				}		/* while (db_p -> di_name_s) */

		}		/* if (db_p) */

	return NULL;
}


/*
{
  "@type": "Grassroots:Service",
  "service": "BlastN",
  "id": "http://localhost:8080/grassroots/public/service/blast/blastn/paragon",
  "so:image": "http://localhost:8080/grassroots/images/BlastN%20service",
  "internal_url": "http://localhost:8080/grassroots/public",
  "payload": {
    "services": [{
      "so:name": "BlastN",
      "refresh_service": true,
      "parameter_set": {
        "level": "simple",
        "parameters": [{
          "param": "Available Databases provided by billy public -> Paragon",
          "current_value": true
        }]
      }
    }]
  },
  "so:name": "Paragon",
  "so:description": "Version 1.1 assembly of Triticum aestivum generated from filtering v1.0 assemblies to remove non-wheat sequences and scaffolds below 1 kb"
}
*/


static json_t *GetBlastIndexingDataPayload (GrassrootsServer *grassroots_p, const char *service_s, const DatabaseInfo *db_p)
{
	json_t *payload_p = NULL;
	char *group_s = GetLocalDatabaseGroupName (grassroots_p);
	char *full_db_name_s = GetFullyQualifiedDatabaseName (group_s, db_p -> di_name_s);

	if (full_db_name_s)
		{
			json_t *params_p = json_array ();

			if (params_p)
				{
					json_t *param_p = json_pack ("{s:s, s:b}", PARAM_NAME_S, full_db_name_s, PARAM_CURRENT_VALUE_S, true);

					if (param_p)
						{
							if (json_array_append_new (params_p, param_p) == 0)
								{
									payload_p = GetIndexingDataPayload (grassroots_p, service_s, params_p);
								}		/* if (json_array_append_new (params_p, param_p) == 0) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, payload_p, "Failed to add params to  \"%s\"", full_db_name_s);
									json_decref (param_p);
								}

						}		/* if (param_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate params array for \"%s\"", full_db_name_s);
						}

					if (!payload_p)
						{
							json_decref (params_p);
						}

				}		/* if (params_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate params array for \"%s\"", full_db_name_s);
				}

			FreeCopiedString (full_db_name_s);
		}		/* if (full_db_name_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetFullyQualifiedDatabaseName failed for \"%s\" and \"%s\"", group_s, db_p -> di_name_s);
		}


	return NULL;
}


json_t *GetBlastIndexingData (Service *service_p)
{
	json_t *indexing_docs_p = json_array ();

	if (indexing_docs_p)
		{
			BlastServiceData *data_p = (BlastServiceData *) (service_p -> se_data_p);
			const DatabaseInfo *db_p = data_p -> bsd_databases_p;

			if (db_p)
				{
					while (db_p -> di_name_s)
						{
							json_t *doc_p = GetIndexingDataForDatabase (service_p, db_p);

							if (doc_p)
								{
									if (json_array_append_new (indexing_docs_p, doc_p) != 0)
										{
											json_decref (doc_p);
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, doc_p, "Failed to add db data to array for \"%s\" in service \"%s\"", db_p-> di_name_s, GetServiceName (service_p));
										}
								}		/* if (doc_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get indexing data for \"%s\" in service \"%s\"", db_p-> di_name_s, GetServiceName (service_p));
								}

							++ db_p;
						}		/* while (db_p) */

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No databases for \"%s\"", GetServiceName (service_p));
				}

			return indexing_docs_p;
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate indexing_docs_p for \"%s\"", GetServiceName (service_p));
		}

	return NULL;
}


static json_t *GetIndexingDataForDatabase (const Service *service_p, const DatabaseInfo *db_p)
{
	const char *name_s = GetServiceName (service_p);
	json_t *index_data_p = json_object();

	if (index_data_p)
		{
			if (SetJSONString (index_data_p, INDEXING_SERVICE_NAME_S, name_s))
				{
					const char *alias_s = GetServiceAlias (service_p);

					if (SetJSONString (index_data_p, INDEXING_SERVICE_ALIAS_S, alias_s))
						{
							if (SetJSONString (index_data_p, INDEXING_NAME_S, db_p -> di_name_s))
								{
									GrassrootsServer *grassroots_p = GetGrassrootsServerFromService (service_p);
									const json_t *hostname_p = GetGlobalConfigValue (grassroots_p, "so:url");
									const char *icon_s = GetJSONString (service_p -> se_data_p -> sd_config_p, OPERATION_ICON_URI_S);

									if (icon_s)
										{
											if (!SetJSONString (index_data_p, INDEXING_ICON_URI_S, icon_s))
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, index_data_p, "Failed to add \"%s\": \"%s\" for \"%s\" in \"%s\"", INDEXING_ICON_URI_S, icon_s, name_s);
												}		/* if (SetJSONString (index_data_p, INDEXING_ICON_URI_S, icon_s)) */

										}		/* if (icon_s) */

									if (hostname_p)
										{
											if (json_is_string (hostname_p))
												{
													const char *url_s = json_string_value (hostname_p);

													if (SetJSONString (index_data_p, INDEXING_PAYLOAD_URL_S, url_s))
														{
															size_t l = strlen (url_s);
															const char *path_s = (* (url_s + l - 1) == '/') ? "service/" : "/service/";
															char *id_s = ConcatenateVarargsStrings (url_s, path_s, alias_s, "/", db_p -> di_name_s, NULL);

															if (id_s)
																{
																	if (SetJSONString (index_data_p, INDEXING_ID_S, id_s))
																		{
																			if (SetJSONString (index_data_p, INDEXING_TYPE_S, INDEXING_TYPE_SERVICE_GRASSROOTS_S))
																				{
																					if (SetJSONString (index_data_p, INDEXING_TYPE_DESCRIPTION_S, INDEXING_TYPE_DESCRIPTION_SERVICE_GRASSROOTS_S))
																						{
																							const char *description_s = db_p -> di_search_description_s ? db_p -> di_search_description_s : db_p -> di_description_s;

																							if (description_s)
																								{
																									if (SetJSONString (index_data_p, INDEXING_DESCRIPTION_S, description_s))
																										{
																											json_t *payload_p = GetIndexingDataPayload (grassroots_p, name_s, db_p);

																											if (payload_p)
																												{
																													if (json_object_set_new (index_data_p, INDEXING_PAYLOAD_DATA_S, payload_p) == 0)
																														{
																															return index_data_p;
																														}		/* if (json_object_set_new (index_data_p, "payload", payload_p) == 0) */
																													else
																														{
																															json_decref (payload_p);
																															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, index_data_p, "Failed to append data to array for \"%s\" in \"%s\"", db_p -> di_name_s, name_s);
																														}
																												}		/* if (payload_p) */
																											else
																												{
																													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetIndexingDataPayload failed for \"%s\" in \"%s\"", db_p -> di_name_s, name_s);
																												}

																										}		/* if (SetJSONString (index_data_p, INDEXING_DESCRIPTION_S, description_s)) */
																									else
																										{
																											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, index_data_p, "Failed to add \"%s\": \"%s\" for \"%s\" in \"%s\"", INDEXING_DESCRIPTION_S, description_s, db_p -> di_name_s, name_s);
																										}

																								}		/* if (description_s) */
																							else
																								{
																									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No description for \"%s\" in \"%s\"", db_p -> di_name_s, name_s);
																								}

																						}
																					else
																						{

																							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, index_data_p, "Failed to add \"%s\": \"%s\" for \"%s\" in \"%s\"", INDEXING_TYPE_DESCRIPTION_S, INDEXING_TYPE_DESCRIPTION_SERVICE_GRASSROOTS_S, db_p -> di_name_s, name_s);
																						}


																				}		/* if (SetJSONString (index_data_p, INDEXING_TYPE_S, INDEXING_TYPE_SERVICE_GRASSROOTS_S)) */
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, index_data_p, "Failed to add \"%s\": \"%s\" for \"%s\" in \"%s\"", INDEXING_TYPE_S, INDEXING_TYPE_SERVICE_GRASSROOTS_S, db_p -> di_name_s, name_s);
																				}

																		}		/* if (SetJSONString (index_data_p, "id", id_s)) */
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, index_data_p, "Failed to add \"id\": \"%s\" for \"%s\" in \"%s\"", id_s, db_p -> di_name_s, name_s);
																		}

																}		/* if (id_s) */
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get id from \"%s\", \"%s\", \"%s\"", url_s, path_s, alias_s);
																}

														}		/* if (SetJSONString (index_data_p, "internal_url", url_s)) */
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, index_data_p, "Failed to add \"internal_url\": \"%s\" for \"%s\" in \"%s\"", url_s, db_p -> di_name_s, name_s);
														}

												}		/* if (json_is_string (hostname_p)) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, hostname_p, "Hostname is not a string");
												}

										}		/* if (hostname_p) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get hostname from global config");
										}

								}		/* if (SetJSONString (index_data_p, INDEXING_NAME_S, database_p -> di_name_s)) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, index_data_p, "Failed to add \"%s\": \"%s\" for \"%s\" in \"%s\"", INDEXING_NAME_S, db_p -> di_name_s, db_p -> di_name_s, name_s);
								}

						}		/* if (SetJSONString (index_data_p, INDEXING_SERVICE_ALIAS_S, name_s)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, index_data_p, "Failed to add \"%s\": \"%s\" for \"%s\" in \"%s\"", INDEXING_SERVICE_ALIAS_S, name_s, db_p -> di_name_s, name_s);
						}


				}		/* if (SetJSONString (index_data_p, SERVICE_NAME_S, name_s)) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, index_data_p, "Failed to add \"%s\": \"%s\" for \"%s\" in \"%s\"", SERVICE_NAME_S, name_s, db_p -> di_name_s, name_s);
				}

			json_decref (index_data_p);
		}		/* if (index_data_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate index data for \"%s\" in \"%s\"", db_p -> di_name_s, name_s);
		}

	return NULL;
}


static void RunJobs (Service *service_p, ParameterSet *param_set_p, const char *input_filename_s, BlastAppParameters *app_params_p, ServiceJobSetIterator *iterator_p)
{
	/*
	 * Get the absolute path and filename stem e.g.
	 *
	 *  file_stem_s = /usr/foo/bar/job_id
	 *
	 *  which will be used to build the input, output and other associated filenames e.g.
	 *
	 *  input file = file_stem_s + ".input"
	 *  output_file = file_stem_s + ".output"
	 *
	 *  As each job will have the same input file name it using the first job's id
	 *
	 */
	ServiceJob *base_job_p = GetNextServiceJobFromServiceJobSetIterator (iterator_p);

	if (base_job_p)
		{
			bool loop_flag = true;
			bool job_ran_flag;

			/*
			 * Iterate over all the jobs and run them if need be
			 */
			while (loop_flag)
				{
					job_ran_flag = false;

					/*
					 * Check that it is a BlastServiceJob as we may also have
					 * RemoteServiceJobs on this list
					 */
					if (strcmp (base_job_p -> sj_type_s, BSJ_TYPE_S) == 0)
						{
							BlastServiceJob *job_p = (BlastServiceJob *) base_job_p;
							BlastTool *tool_p = job_p -> bsj_tool_p;

							/*
							 * Assume the job has failed unless proved otherwise
							 */
							SetServiceJobStatus (base_job_p, OS_FAILED_TO_START);

							LogServiceJob (base_job_p);

							if (tool_p)
								{
									if (tool_p -> SetInputFilename (input_filename_s))
										{
											if (tool_p -> SetUpOutputFile())
												{
													if (tool_p -> ParseParameters (param_set_p, app_params_p))
														{
															if (RunBlast (tool_p))
																{
																	/* If the status needs updating, refresh it */
																	if ( (job_p -> bsj_job.sj_status == OS_PENDING) || (job_p -> bsj_job.sj_status == OS_STARTED))
																		{
																			SetServiceJobStatus (base_job_p, tool_p -> GetStatus());
																		}

																	LogServiceJob (base_job_p);
																	job_ran_flag = true;

																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to run blast tool \"%s\"", job_p -> bsj_job.sj_name_s);
																}

														}		/* if (tool_p -> ParseParameters (param_set_p, input_filename_s)) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to parse parameters for blast tool \"%s\"", job_p -> bsj_job.sj_name_s);
														}

												}		/* if (tool_p -> SetOutputFilename ()) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set output filename for blast tool \"%s\"", job_p -> bsj_job.sj_name_s);
												}

										}		/* if (tool_p -> SetInputFilename (input_filename_s)) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set input filename for blast tool \"%s\" to \"%s\"", job_p -> bsj_job.sj_name_s, input_filename_s);
										}

								}		/* if (tool_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get blast tool for \"%s\"", job_p -> bsj_job.sj_name_s);
								}

						}		/* if (strcmp (base_job_p -> sj_type_s, BSJ_TYPE_S) == 0) */
					else if (strcmp (base_job_p -> sj_type_s, RSJ_TYPE_S) == 0)
						{
							job_ran_flag = true;
						}		/* else if (strcmp (base_job_p -> sj_type_s, RSJ_TYPE_S) == 0) */


					if (job_ran_flag)
						{
							switch (base_job_p -> sj_status)
								{
									case OS_SUCCEEDED:
									case OS_PARTIALLY_SUCCEEDED:
										if (base_job_p -> sj_result_p)
											{
												char job_id_s [UUID_STRING_BUFFER_SIZE];

												ConvertUUIDToString (base_job_p -> sj_id, job_id_s);
												PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get results for %s", job_id_s);
											}

										break;

									case OS_PENDING:
									case OS_STARTED:
									{
										GrassrootsServer *grassroots_p = GetGrassrootsServerFromService (service_p);
										JobsManager *jobs_manager_p = GetJobsManager (grassroots_p);

										if (jobs_manager_p)
											{
												if (!AddServiceJobToJobsManager (jobs_manager_p, base_job_p -> sj_id, base_job_p))
													{
														char job_id_s [UUID_STRING_BUFFER_SIZE];

														ConvertUUIDToString (base_job_p -> sj_id, job_id_s);
														PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add job \"%s\" to JobsManager", job_id_s);
													}
											}
									}
									break;

									default:
										break;
								}		/* switch (job_p -> bsj_job.sj_status) */

						}		/* if (job_ran_flag) */

					base_job_p = GetNextServiceJobFromServiceJobSetIterator (iterator_p);
					loop_flag = (base_job_p != NULL);

				}		/* while (loop_flag) */


		}		/* if (job_p) */

}


static bool PreRunJobs (BlastServiceData *blast_data_p)
{
	bool success_flag = true;

	if (blast_data_p -> bsd_task_manager_p)
		{
			Service *blast_service_p = blast_data_p -> bsd_base_data.sd_service_p;

			/*
			 * Set the initial counter value to 1 which will take the system's call
			 * to ReleaseService () into account once it has finished with this
			 * Service. This is to make sure that the Service doesn't delete itself
			 * after it has finished running all of its jobs asynchronously before
			 * the system has finished with it and then accessing freed memory.
			 */
			PrepareAsyncTasksManager (blast_data_p -> bsd_task_manager_p, 1);

			/* If we have asynchronous jobs running then set the "is running" flag for this service */
			SetServiceRunning (blast_service_p, true);
		}

	return success_flag;
}


static bool CleanupAsyncBlastService (void *data_p)
{
	bool success_flag = false;
	Service *blast_service_p = (Service *) data_p;

	if (LockService (blast_service_p))
		{
			/*
			 * Since we could lock the Service, it means that the main program
			 * thread has finished using it, so we're safe to delete it.
			 */
			if (UnlockService (blast_service_p))
				{
					FreeService (blast_service_p);
					success_flag = true;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to unlock service \"%s\"", GetServiceName (blast_service_p));
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to lock service \"%s\"", GetServiceName (blast_service_p));
		}

	return success_flag;
}




static char *ConfigureWorkingDirectoryPath (const json_t *blast_config_p)
{
	char *dir_s = NULL;
	const char *working_dir_config_s = GetJSONString (blast_config_p, "working_directory");

	if (working_dir_config_s)
		{
			char sep_s [2];

			*sep_s = GetFileSeparatorChar();
			* (sep_s + 1) = '\0';

			if (DoesStringEndWith (working_dir_config_s, sep_s))
				{
					dir_s = EasyCopyToNewString (working_dir_config_s);

					if (!dir_s)
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "EasyCopyToNewString failed for working directory \"%s\"", working_dir_config_s);
						}
				}
			else
				{
					dir_s = ConcatenateStrings (working_dir_config_s, sep_s);

					if (!dir_s)
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "ConcatenateStrings failed for working directory \"%s\" and \"%s\"", working_dir_config_s, sep_s);
						}
				}
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, blast_config_p, "No working directory configured");
		}

	return dir_s;
}

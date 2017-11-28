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

/*
 * paired_blast_service.c
 *
 *  Created on: 9 Feb 2016
 *      Author: billy
 */
#include <string.h>

#include "typedefs.h"
#include "paired_blast_service.h"
#include "blast_service.h"
#include "servers_pool.h"
#include "parameter.h"
#include "parameter_group.h"
#include "json_tools.h"
#include "service_job.h"
#include "blast_service_job.h"
#include "blast_service_params.h"
#include "remote_service_job.h"

#ifdef _DEBUG
	#define PAIRED_BLAST_SERVICE_DEBUG	(STM_LEVEL_FINER)
#else
	#define PAIRED_BLAST_SERVICE_DEBUG	(STM_LEVEL_NONE)
#endif


static const char *s_remote_suffix_s = ".remote";


/**
 * Add a database if it is a non-database parameter or if
 * refers to a database at the paired service sent in as data_p.
 *
 * @param param_p
 * @param data_p
 * @return
 */
static bool AddRemoteServiceParametersToJSON (const Parameter *param_p, void *data_p);


static char *GetLocalJobFilename (const char *uuid_s, const BlastServiceData *blast_data_p);




/***********************************/
/******** PUBLIC FUNCTIONS *********/
/***********************************/


bool AddPairedServiceParameters (Service *service_p, ParameterSet *internal_params_p)
{
	bool success_flag = true;

	if (service_p -> se_paired_services.ll_size > 0)
		{
			PairedServiceNode *node_p = (PairedServiceNode *) (service_p -> se_paired_services.ll_head_p);

			while (node_p)
				{
					/*
					 * Try and add the external server's databases
					 */
					PairedService *paired_service_p = node_p -> psn_paired_service_p;
					char *databases_group_s = CreateGroupName (paired_service_p -> ps_server_name_s);

					if (databases_group_s)
						{
							ParameterGroup *db_group_p = GetParameterGroupFromParameterSetByGroupName (paired_service_p -> ps_params_p, databases_group_s);

							if (db_group_p)
								{
									const uint32 num_params = db_group_p -> pg_params_p -> ll_size;

									if (num_params > 0)
										{
											ParameterNode *src_node_p = (ParameterNode *) (db_group_p -> pg_params_p -> ll_head_p);
											ParameterGroup *dest_group_p = CreateAndAddParameterGroupToParameterSet (databases_group_s, paired_service_p -> ps_server_uri_s, false, service_p -> se_data_p, internal_params_p);
											SharedType def;
											uint32 num_added_dbs = 0;

											InitSharedType (&def);

											while (src_node_p)
												{
													/* Add the database to our list */
													Parameter *external_param_p = src_node_p -> pn_parameter_p;
													Parameter *param_p = NULL;

													def.st_boolean_value = external_param_p -> pa_current_value.st_boolean_value;

													param_p = CreateAndAddParameterToParameterSet (service_p -> se_data_p, internal_params_p, dest_group_p, PT_BOOLEAN, false, external_param_p -> pa_name_s, external_param_p -> pa_display_name_s, external_param_p -> pa_description_s, NULL, def, NULL, NULL, PL_INTERMEDIATE | PL_ALL, NULL);

													if (param_p)
														{
															if (!CopyRemoteParameterDetails (external_param_p, param_p))
																{
																	PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to copy details from external parameter \"%s\" to param \"%s\"", external_param_p -> pa_name_s, param_p -> pa_name_s);
																}		/* if (!CopyRemoteParameterDetails (external_param_p, param_p)) */

															++ num_added_dbs;
														}		/* if (param_p) */

													src_node_p = (ParameterNode *) (src_node_p -> pn_node.ln_next_p);
												}		/* while (i > 0) */

										}		/* if (num_params > 0) */

								}		/* if (db_group_p) */

							FreeCopiedString (databases_group_s);
						}		/* if (databases_group_s) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate memory for database group name \"%s\"", paired_service_p -> ps_server_name_s);
						}


					node_p = (PairedServiceNode *) (node_p -> psn_node.ln_next_p);
				}		/* while (node_p) */

		}		/* if (service_p -> se_paired_services.ll_size > 0) */

	return success_flag;
}



//int32 RunRemoteBlastJobs (Service *service_p, ServiceJobSet *jobs_p, ParameterSet *params_p, PairedService *paired_service_p, ProvidersStateTable *providers_p)
//{
//	int32 num_successful_runs = -1;
//	json_t *res_p = MakeRemotePairedServiceCall (GetServiceName (service_p), params_p, paired_service_p -> ps_server_uri_s, providers_p);
//
//	if (res_p)
//		{
//			num_successful_runs = AddRemoteResultsToServiceJobs (res_p, jobs_p, paired_service_p -> ps_name_s, paired_service_p -> ps_server_uri_s, service_p -> se_data_p, SaveRemoteBlastJobDetails);
//
//			json_decref (res_p);
//		}		/* if (res_p) */
//
//	return num_successful_runs;
//}



char *GetPreviousRemoteBlastServiceJob (const char *local_job_id_s, const uint32 output_format_code, const BlastServiceData *blast_data_p)
{
	char *result_s = NULL;
	char *job_filename_s = GetLocalJobFilename (local_job_id_s, blast_data_p);

	if (job_filename_s)
		{
			json_error_t e;
			json_t *remote_p = json_load_file (job_filename_s, 0, &e);

			if (remote_p)
				{
					/*
					 * This json_t should consist of the remote server's uri and the uuid
					 * of the job on that server
					 */
					const char *uri_s = GetJSONString (remote_p, JOB_REMOTE_URI_S);

					if (uri_s)
						{
							const char *remote_job_id_s = GetJSONString (remote_p, JOB_REMOTE_UUID_S);

							if (remote_job_id_s)
								{

									/*
									 * Make the call to the remote server for the previous results
									 * using this id
									 */
									ParameterSet *param_set_p = AllocateParameterSet ("Blast service parameters", "The parameters used for the Blast service");
									ParameterGroup *group_p = NULL;

									if (param_set_p)
										{
											Parameter *param_p = SetUpPreviousJobUUIDParamater (blast_data_p, param_set_p, group_p);

											if (param_p)
												{
													if (SetParameterValue (param_p, remote_job_id_s, true))
														{
															param_p = SetUpOutputFormatParamater (blast_data_p, param_set_p, group_p);

															if (param_p)
																{
																	if (SetParameterValue (param_p, &output_format_code, true))
																		{
																			Service *service_p = blast_data_p -> bsd_base_data.sd_service_p;

																			if (service_p)
																				{
																					const char *service_name_s = GetServiceName (blast_data_p -> bsd_base_data.sd_service_p);

																					if (service_name_s)
																						{
																							json_t *res_p = MakeRemotePairedServiceCall (service_name_s, param_set_p, uri_s, NULL);

																							if (res_p)
																								{
																									int32 num_added = AddRemoteResultsToServiceJobs (res_p, service_p, service_name_s, uri_s, service_p -> se_data_p, SaveRemoteBlastJobDetails);

																									#if PAIRED_BLAST_SERVICE_DEBUG >= STM_LEVEL_FINE
																									PrintLog (STM_LEVEL_FINE, __FILE__, __LINE__, "Added " INT32_FMT " jobs from remote results");
																									#endif

																									json_decref (res_p);
																								}		/* if (res_p) */
																							else
																								{
																									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "MakeRemotePairedServiceCall to \"%s\" at \"%s\" with param set to \"%s\" returned NULL", service_name_s, uri_s, param_p -> pa_current_value.st_string_value_s);
																								}

																						}		/* if (service_name_s) */
																					else
																						{
																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get Blast service name");
																						}

																				}		/* if (service_p) */
																			else
																				{
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set get Blast service");
																				}

																		}		/* if (SetParameterValue (param_p, &output_format_code, true)) */
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set Parameter value for out put format to " UINT32_FMT, output_format_code);
																		}
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create Parameter for output format");
																}

														}		/* if (SetParameterValue (param_p, remote_job_id_s, true)) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set Parameter value for previous job ids to \"%s\"", remote_job_id_s);
														}

												}		/* if (param_p) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create Parameter for previous job ids");
												}

											FreeParameterSet (param_set_p);
										}		/* if (param_set_p) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate ParamaterSet");
										}

								}		/* if (remote_job_id_s) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get \"%s\" from \"%s\"", JOB_REMOTE_UUID_S, job_filename_s);
								}

						}		/* if (uri_s) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get \"%s\" from \"%s\"", JOB_REMOTE_UUID_S, job_filename_s);
						}

					json_decref (remote_p);
				}		/* if (remote_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to load remote json data from \"%s\", error at line %d, col %d, \"%s\"", job_filename_s, e.line, e.column, e.text);
				}

			FreeCopiedString (job_filename_s);
		}		/* if (job_filename_s) */
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get output filename for \"%s\"", local_job_id_s);
		}

	return result_s;
}


/***********************************/
/******** STATIC FUNCTIONS *********/
/***********************************/


/**
 * Add a database if it is a non-database parameter or if
 * refers to a database at the paired service sent in as data_p.
 *
 * @param param_p
 * @param data_p
 * @return
 */
static bool AddRemoteServiceParametersToJSON (const Parameter *param_p, void *data_p)
{
	bool add_flag = true;
	PairedService *paired_service_p = (PairedService *) data_p;

	if (param_p -> pa_group_p)
		{
			char *paired_service_group_name_s = CreateGroupName (paired_service_p -> ps_server_name_s);

			if (paired_service_group_name_s)
				{
					const char *param_group_name_s = param_p -> pa_group_p -> pg_name_s;

					if (strcmp (paired_service_group_name_s, param_group_name_s) == 0)
						{
							add_flag = true;
						}

					FreeCopiedString (paired_service_group_name_s);
				}		/* if (paired_service_group_name_s) */

		}		/* if (param_p -> pa_group_p) */

	return add_flag;
}




static char *GetLocalJobFilename (const char *uuid_s, const BlastServiceData *blast_data_p)
{
	char *output_filename_s = NULL;
	char *output_filename_stem_s = MakeFilename (blast_data_p -> bsd_working_dir_s, uuid_s);

	if (output_filename_stem_s)
		{
			output_filename_s = ConcatenateStrings (output_filename_stem_s, s_remote_suffix_s);

			if (!output_filename_s)
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create filename from \"%s\" and \"%s\"", output_filename_stem_s, s_remote_suffix_s);
				}

			FreeCopiedString (output_filename_stem_s);
		}		/* if (output_filename_stem_s) */
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create filename stem from \"%s\" and \"%s\"", output_filename_stem_s, uuid_s);
		}

	return output_filename_s;
}


bool SaveRemoteBlastJobDetails (RemoteServiceJob *job_p, const ServiceData *service_data_p)
{
	const BlastServiceData *blast_data_p = (const BlastServiceData *) service_data_p;
	bool success_flag = false;
	char uuid_s [UUID_STRING_BUFFER_SIZE];
	char *output_filename_s = NULL;

	ConvertUUIDToString (job_p -> rsj_job.sj_id, uuid_s);

	output_filename_s = GetLocalJobFilename (uuid_s, blast_data_p);

	if (output_filename_s)
		{
			json_error_t error;
			json_t *remote_p = NULL;

			ConvertUUIDToString (job_p -> rsj_job_id, uuid_s);

			remote_p = json_pack_ex (&error, 0, "{s:s,s:s,s:s}", JOB_REMOTE_URI_S, job_p -> rsj_uri_s, JOB_REMOTE_UUID_S, uuid_s, JOB_SERVICE_S, job_p -> rsj_service_name_s);

			if (remote_p)
				{
					int res = json_dump_file (remote_p, output_filename_s, JSON_INDENT (2));

					if (res == 0)
						{
							success_flag = true;
						}

					json_decref (remote_p);
				}		/* if (remote_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create remote json data, error at line %d, col %d,  \"%s\"", error.line, error.column, error.text);
				}

			FreeCopiedString (output_filename_s);
		}		/* if (output_filename_s) */

	return success_flag;
}





static char *GetRemoteBlastResultByUUIDString (const BlastServiceData *data_p, const char *job_id_s, const uint32 output_format_code)
{
	char *result_s = NULL;
	char *job_output_filename_s = GetPreviousJobFilename (data_p, job_id_s, s_remote_suffix_s);

	if (job_output_filename_s)
		{

			FreeCopiedString (job_output_filename_s);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Couldn't get previous job filename for \"%s\"", job_output_filename_s);
		}

	return result_s;
}


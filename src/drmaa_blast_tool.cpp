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
 * drmaa_blast_tool.cpp
 *
 *  Created on: 22 Apr 2015
 *      Author: tyrrells
 */

#include "../../blast-service/include/drmaa_blast_tool.hpp"

#include <new>
#include <cstring>
#include <stdexcept>

#include "../../blast-service/include/blast_service_job.h"
#include "../../blast-service/include/drmaa_tool_args_processor.hpp"
#include "streams.h"
#include "string_utils.h"
#include "jobs_manager.h"
#include "alloc_failure.hpp"

#ifdef _DEBUG
	#define DRMAA_BLAST_TOOL_DEBUG	(STM_LEVEL_FINEST)
#else
	#define DRMAA_BLAST_TOOL_DEBUG (STM_LEVEL_NONE)
#endif


const char * const DrmaaBlastTool :: DBT_DRMAA_S = "drmaa";




static bool UpdateDrmaaBlastServiceJob (struct ServiceJob *job_p);



void DrmaaBlastTool :: SetDrmaaOptions (DrmaaTool *drmaa_p, bool async_flag)
{
	dbt_drmaa_tool_p = drmaa_p;
	dbt_async_flag = async_flag;
}



DrmaaBlastTool :: DrmaaBlastTool (BlastServiceJob *job_p, const char *name_s, const char *factory_s, const BlastServiceData *data_p, const char *blast_program_name_s, const char *queue_name_s, const char *const output_path_s, bool async_flag)
: ExternalBlastTool (job_p, name_s, factory_s, data_p, blast_program_name_s, async_flag)
{
	const char *error_s = 0;
	DrmaaTool *drmaa_tool_p = AllocateDrmaaTool (blast_program_name_s, job_p -> bsj_job.sj_id);

	if (drmaa_tool_p)
		{
			if ((queue_name_s == NULL) || (SetDrmaaToolQueueName (drmaa_tool_p, queue_name_s)))
				{
					if (SetDrmaaToolJobName (drmaa_tool_p, name_s))
						{
							if (SetDrmaaToolOutputFilename (drmaa_tool_p, output_path_s))
								{
									SetDrmaaOptions (drmaa_tool_p, async_flag);
									SetServiceJobUpdateFunction (& (job_p -> bsj_job), UpdateDrmaaBlastServiceJob);

									dbt_args_processor_p = new DrmaaToolArgsProcessor (drmaa_tool_p);

									return;
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetDrmaaToolOutputFilename failed for \"%s\"", output_path_s);
									error_s = "SetDrmaaToolOutputFilename failed";
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetDrmaaToolJobName failed for \"%s\"", name_s);
							error_s = "SetDrmaaToolJobName failed";
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetDrmaaToolQueueName failed for \"%s\"", queue_name_s);
					error_s = "SetDrmaaToolQueueName failed";
				}

			FreeDrmaaTool (drmaa_tool_p);

		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate drmaa tool");
			error_s = "Failed to allocate drmaa tool";
		}

	throw AllocFailure (error_s);
}


DrmaaBlastTool :: DrmaaBlastTool (BlastServiceJob *job_p, const BlastServiceData *data_p, const json_t *root_p)
	: ExternalBlastTool (job_p, data_p, root_p)
{
	json_t *drmaa_json_p = json_object_get (root_p, DBT_DRMAA_S);

	if (drmaa_json_p)
		{
			dbt_drmaa_tool_p = ConvertDrmaaToolFromJSON (drmaa_json_p);

			if (dbt_drmaa_tool_p)
				{
					dbt_args_processor_p = new DrmaaToolArgsProcessor (dbt_drmaa_tool_p);
					dbt_async_flag = true;
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, drmaa_json_p, "Failed to get drmaa tool");
					throw std :: invalid_argument ("Failed to create drmaa tool");
				}
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, drmaa_json_p, "Failed to get drmaa json");
			throw std :: invalid_argument ("Failed to create drmaa tool");
		}


}


DrmaaBlastTool :: ~DrmaaBlastTool ()
{
	#if DRMAA_BLAST_TOOL_DEBUG >= STM_LEVEL_FINEST
	char uuid_s [UUID_STRING_BUFFER_SIZE];

	ConvertUUIDToString (bt_job_p -> bsj_job.sj_id, uuid_s);
	PrintLog (STM_LEVEL_FINEST, __FILE__, __LINE__, "Entering ~DrmaaBlastTool for %s with job id %s", uuid_s, dbt_drmaa_tool_p -> dt_id_s);
	#endif

	FreeDrmaaTool (dbt_drmaa_tool_p);

	delete dbt_args_processor_p;


	#if DRMAA_BLAST_TOOL_DEBUG >= STM_LEVEL_FINEST
	PrintLog (STM_LEVEL_FINEST, __FILE__, __LINE__, "Exiting ~DrmaaBlastTool for %s", uuid_s);
	#endif
}


void DrmaaBlastTool :: SetCoresPerSearch (uint32 cores)
{
	dbt_drmaa_tool_p -> dt_num_cores = cores;
}



bool DrmaaBlastTool :: SetEmailNotifications (const char **email_addresses_ss)
{
	return SetDrmaaToolEmailNotifications (dbt_drmaa_tool_p, email_addresses_ss);
}


OperationStatus DrmaaBlastTool :: Run ()
{
	char *job_id_filename_s = GetJobFilename (NULL, ".job");
	OperationStatus status = OS_IDLE;
	char uuid_s [UUID_STRING_BUFFER_SIZE];

	ConvertUUIDToString (bt_job_p -> bsj_job.sj_id, uuid_s);


	if (!job_id_filename_s)
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create job filename for ", uuid_s);
		}


	if (RunDrmaaTool (dbt_drmaa_tool_p, ebt_async_flag, job_id_filename_s))
		{

			#if DRMAA_BLAST_TOOL_DEBUG >= STM_LEVEL_FINE
				{
					PrintLog (STM_LEVEL_FINE, __FILE__, __LINE__, "Added drmaa blast tool %s with job id %s", uuid_s, dbt_drmaa_tool_p -> dt_id_s);
				}
			#endif
		}
	else
		{
			AddErrorToBlastServiceJob (bt_job_p);

			SetServiceJobStatus (& (bt_job_p -> bsj_job), OS_FAILED);
		}

	if (job_id_filename_s)
		{
			FreeCopiedString (job_id_filename_s);
		}

	status = GetServiceJobStatus (& (bt_job_p -> bsj_job));

	if (status == OS_FAILED)
		{
			AddErrorToBlastServiceJob (bt_job_p);
		}

	return status;
}


bool DrmaaBlastTool :: AddBlastArg (const char * const arg_s, const bool hyphen_flag)
{
	bool success_flag = AddDrmaaToolArgument (dbt_drmaa_tool_p, arg_s);

	if (hyphen_flag)
		{
			char *value_s = ConcatenateStrings ("-", arg_s);

			if (value_s)
				{
					success_flag = AddDrmaaToolArgument (dbt_drmaa_tool_p, value_s);
					FreeCopiedString (value_s);
				}
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to concatenate \"-\" and \"%s\"", arg_s);
				}
		}
	else
		{
			success_flag = AddDrmaaToolArgument (dbt_drmaa_tool_p, arg_s);
		}


	return success_flag;
}


OperationStatus DrmaaBlastTool :: GetStatus (bool update_flag)
{
	OperationStatus status = OS_ERROR;

	if (update_flag)
		{
			status = GetDrmaaToolStatus (dbt_drmaa_tool_p);

			SetServiceJobStatus (& (bt_job_p -> bsj_job), status);

			if (status == OS_SUCCEEDED || status == OS_PARTIALLY_SUCCEEDED)
				{
					DetermineBlastResult (bt_job_p);
				}
		}
	else
		{
			status = GetCachedServiceJobStatus (& (bt_job_p -> bsj_job));
		}

	return status;
}



bool DrmaaBlastTool :: SetUpOutputFile ()
{
	bool success_flag = false;

	if (ExternalBlastTool :: SetUpOutputFile ())
		{
			char *logfile_s = GetJobFilename (":", BS_LOG_SUFFIX_S);

			if (logfile_s)
				{
					if (SetDrmaaToolOutputFilename (dbt_drmaa_tool_p, logfile_s))
						{
							success_flag = true;
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set drmaa logfile name to \"%s\"", logfile_s);
						}

					FreeCopiedString (logfile_s);
				}
			else
				{
					char *uuid_s = GetUUIDAsString (bt_job_p -> bsj_job.sj_id);

					if (uuid_s)
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get logfile name for \"%s\"", uuid_s);
							FreeUUIDString (uuid_s);
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get logfile name");
						}
				}
		}		/* if (ExternalBlastTool :: SetOutputFile ()) */

	return success_flag;
}


bool DrmaaBlastTool :: AddToJSON (json_t *root_p)
{
	bool success_flag = ExternalBlastTool :: AddToJSON (root_p);

	if (success_flag)
		{
			char uuid_s [UUID_STRING_BUFFER_SIZE];

			ConvertUUIDToString (bt_job_p -> bsj_job.sj_id, uuid_s);


			json_t *drmaa_json_p = ConvertDrmaaToolToJSON (dbt_drmaa_tool_p);

			if (drmaa_json_p)
				{
					if (json_object_set_new (root_p, DBT_DRMAA_S, drmaa_json_p) == 0)
						{
							success_flag = true;
						}		/* if (json_object_set_new (root_p, DBT_DRMAA_S, drmaa_json_p) == 0) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed add get drmaa json for %s", uuid_s);

							json_decref (drmaa_json_p);
						}		/* if (json_object_set_new (root_p, DBT_DRMAA_S, drmaa_json_p) == 0) else */

				}		/* if (drmaa_json_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create drmaa json for %s", uuid_s);
				}

		}		/* if (success_flag) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "DrmaaBlastTool :: AddToJSON failed");
		}

	return success_flag;
}


ArgsProcessor *DrmaaBlastTool :: GetArgsProcessor ()
{
	return dbt_args_processor_p;
}


static bool UpdateDrmaaBlastServiceJob (struct ServiceJob *job_p)
{
	BlastServiceJob *blast_job_p = reinterpret_cast <BlastServiceJob *> (job_p);
	DrmaaBlastTool *tool_p = static_cast <DrmaaBlastTool *> (blast_job_p -> bsj_tool_p);

	tool_p -> GetStatus (true);

	return true;
}

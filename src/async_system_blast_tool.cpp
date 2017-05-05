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
 * async_system_blast_tool.cpp
 *
 *  Created on: 9 Mar 2017
 *      Author: billy
 *
 * @file
 * @brief
 */


#include "async_system_blast_tool.hpp"
#include "blast_service_job.h"

#include "string_utils.h"
#include "jobs_manager.h"

#include "system_async_task.h"


#ifdef _DEBUG
	#define ASYNC_SYSTEM_BLAST_TOOL_DEBUG (STM_LEVEL_FINE)
#else
	#define ASYNC_SYSTEM_BLAST_TOOL_DEBUG (STM_LEVEL_NONE)
#endif

const char * const AsyncSystemBlastTool :: ASBT_ASYNC_S = "async";

const char * const AsyncSystemBlastTool :: ASBT_LOGFILE_S = "logfile";

static bool UpdateAsyncBlastServiceJob (struct ServiceJob *job_p);



AsyncSystemBlastTool :: AsyncSystemBlastTool (BlastServiceJob *job_p, const char *name_s, const char *factory_s, const BlastServiceData *data_p, const char *blast_program_name_s)
: SystemBlastTool (job_p, name_s, factory_s, data_p, blast_program_name_s),
	asbt_async_logfile_s (0)
{
	SetServiceJobUpdateFunction (& (job_p -> bsj_job), UpdateAsyncBlastServiceJob);
	ebt_async_flag = true;
}


AsyncSystemBlastTool :: ~AsyncSystemBlastTool ()
{

}



AsyncSystemBlastTool :: AsyncSystemBlastTool (BlastServiceJob *job_p, const BlastServiceData *data_p, const json_t *root_p)
: SystemBlastTool (job_p, data_p, root_p),
	asbt_async_logfile_s (0)
{
	bool alloc_flag = false;

	if (Init (ebt_blast_s))
		{
			bool async_flag = false;

			if (GetJSONBoolean (root_p, AsyncSystemBlastTool :: ASBT_ASYNC_S, &async_flag))
				{
					const char *value_s = GetJSONString (root_p, AsyncSystemBlastTool :: ASBT_LOGFILE_S);

					alloc_flag = true;

					if (value_s)
						{
							asbt_async_logfile_s = CopyToNewString (value_s, 0, false);

							if (!asbt_async_logfile_s)
								{
									alloc_flag = false;
								}
						}
				}

		}

	if (!alloc_flag)
		{
			throw std :: bad_alloc ();
		}
}


OperationStatus AsyncSystemBlastTool :: Run ()
{
	OperationStatus status = OS_FAILED_TO_START;
	const char *command_line_s = sbt_args_processor_p -> GetArgsAsString ();

	#if ASYNC_SYSTEM_BLAST_TOOL_DEBUG >= STM_LEVEL_FINE
	PrintLog (STM_LEVEL_FINE, __FILE__, __LINE__, "About to run SystemBlastTool with \"%s\"", command_line_s);
	#endif


	if (command_line_s)
		{
			SystemTaskData *task_data_p = CreateSystemTaskData (& (bt_job_p -> bsj_job), command_line_s);

			if (task_data_p)
				{
					if (RunAsyncSystemTask (task_data_p))
						{
							status = GetStatus ();
						}

					#if ASYNC_SYSTEM_BLAST_TOOL_DEBUG >= STM_LEVEL_FINE
						{
							char uuid_s [UUID_STRING_BUFFER_SIZE];

							ConvertUUIDToString (bt_job_p -> bsj_job.sj_id, uuid_s);
							PrintLog (STM_LEVEL_FINE, __FILE__, __LINE__, "Created async task for uuid %s", uuid_s);
						}
					#endif

				}		/* if (task_data_p) */
			else
				{

				}
		}


	if ((status == OS_ERROR) || (status == OS_FAILED_TO_START) || (status == OS_FAILED))
		{
			char *log_s = GetLog ();

			status = OS_FAILED;
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "\"%s\" returned %d", command_line_s, status);

			if (log_s)
				{
					if (!AddErrorToServiceJob (& (bt_job_p -> bsj_job), ERROR_S, log_s))
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add error \"%s\" to service job");
						}

					FreeCopiedString (log_s);
				}		/* if (log_s) */
		}

	SetServiceJobStatus (& (bt_job_p -> bsj_job), status);

	return status;
}



bool AsyncSystemBlastTool :: AddToJSON (json_t *root_p)
{
	bool success_flag = ExternalBlastTool :: AddToJSON (root_p);

	if (success_flag)
		{
			if (json_object_set_new (root_p, AsyncSystemBlastTool :: ASBT_ASYNC_S, json_true ()) != 0)
				{
					success_flag = false;
				}

			if (asbt_async_logfile_s)
				{
					if (json_object_set_new (root_p, AsyncSystemBlastTool :: ASBT_LOGFILE_S, json_string (asbt_async_logfile_s)) != 0)
						{
							success_flag = false;
						}
				}

		}		/* if (success_flag) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SystemBlastTool :: AddToJSON failed");
		}

	return success_flag;
}



OperationStatus AsyncSystemBlastTool :: GetStatus (bool update_flag)
{
	OperationStatus status = OS_ERROR;

	if (update_flag)
		{
			/*
			 * If the job has already finished, then
			 * no need to check for any status updates
			 */
			OperationStatus old_status = GetCachedServiceJobStatus (& (bt_job_p -> bsj_job));

			#if ASYNC_SYSTEM_BLAST_TOOL_DEBUG >= STM_LEVEL_FINE
				{
					char uuid_s [UUID_STRING_BUFFER_SIZE];

					ConvertUUIDToString (bt_job_p -> bsj_job.sj_id, uuid_s);
					PrintLog (STM_LEVEL_FINE, __FILE__, __LINE__, "Old status " INT32_FMT " for uuid %s", old_status, uuid_s);
				}
			#endif

			if ((old_status != OS_SUCCEEDED) && (old_status != OS_PARTIALLY_SUCCEEDED) && (old_status != OS_FINISHED))
				{
					status = GetServiceJobStatus (& (bt_job_p -> bsj_job));

					#if ASYNC_SYSTEM_BLAST_TOOL_DEBUG >= STM_LEVEL_FINE
						{
							char uuid_s [UUID_STRING_BUFFER_SIZE];

							ConvertUUIDToString (bt_job_p -> bsj_job.sj_id, uuid_s);
							PrintLog (STM_LEVEL_FINE, __FILE__, __LINE__, "Got status " INT32_FMT " for uuid %s", status, uuid_s);
						}
					#endif

					if ((status == OS_SUCCEEDED) || (status == OS_PARTIALLY_SUCCEEDED) || (status == OS_FINISHED))
						{
							if (!DetermineBlastResult (bt_job_p))
								{

								}
						}
				}
			else
				{
					status = old_status;
				}
		}
	else
		{
			status = GetCachedServiceJobStatus (& (bt_job_p -> bsj_job));
		}

	return status;
}




char *AsyncSystemBlastTool :: GetResults (BlastFormatter *formatter_p)
{
	char *results_s = ExternalBlastTool :: GetResults (formatter_p);
	JobsManager *jobs_manager_p = GetJobsManager ();

	/*
	 * Remove the ServiceJob from the JobsManager
	 */
	RemoveServiceJobFromJobsManager (jobs_manager_p, bt_job_p -> bsj_job.sj_id, false);


	return results_s;
}





static bool UpdateAsyncBlastServiceJob (struct ServiceJob *job_p)
{
	BlastServiceJob *blast_job_p = reinterpret_cast <BlastServiceJob *> (job_p);
	AsyncSystemBlastTool *tool_p = static_cast <AsyncSystemBlastTool *> (blast_job_p -> bsj_tool_p);

	tool_p -> GetStatus (true);

	return true;
}

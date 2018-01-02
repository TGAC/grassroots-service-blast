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

	asbt_task_p = AllocateSystemAsyncTask (& (job_p -> bsj_job), name_s, data_p -> bsd_task_manager_p, true, blast_program_name_s, BlastServiceJobCompleted);

	if (!asbt_task_p)
		{
			throw std :: bad_alloc ();
		}
}


AsyncSystemBlastTool :: ~AsyncSystemBlastTool ()
{
	if (asbt_async_logfile_s)
		{
			FreeCopiedString (asbt_async_logfile_s);
		}

	FreeSystemAsyncTask (asbt_task_p);
}


AsyncSystemBlastTool :: AsyncSystemBlastTool (BlastServiceJob *job_p, const BlastServiceData *data_p, const json_t *root_p)
: SystemBlastTool (job_p, data_p, root_p),
	asbt_async_logfile_s (0)
{
	bool alloc_flag = false;
	bool async_flag;

	if (GetJSONBoolean (root_p, AsyncSystemBlastTool :: ASBT_ASYNC_S, &async_flag))
		{
			char *name_s = NULL;
			char *blast_program_name_s = NULL;
			bool continue_flag = true;
			const char *value_s = GetJSONString (root_p, AsyncSystemBlastTool :: ASBT_LOGFILE_S);

			if (value_s)
				{
					asbt_async_logfile_s = CopyToNewString (value_s, 0, false);

					if (!asbt_async_logfile_s)
						{
							continue_flag = false;
						}
				}
			else
				{
					asbt_async_logfile_s = NULL;
				}

			if (continue_flag)
				{
					asbt_task_p = AllocateSystemAsyncTask (& (job_p -> bsj_job), name_s, data_p -> bsd_task_manager_p, true, blast_program_name_s, BlastServiceJobCompleted);

					if (asbt_task_p)
						{
							alloc_flag = true;
						}
				}

			if (!alloc_flag)
				{
					FreeCopiedString (asbt_async_logfile_s);
				}
		}

	if (!alloc_flag)
		{
			throw std :: bad_alloc ();
		}
}



bool AsyncSystemBlastTool :: PreRun ()
{
	bool b = BlastTool :: PreRun ();

	if (b)
		{
			SetServiceJobStatus (& (bt_job_p -> bsj_job), OS_STARTED);
		}

	return b;
}


OperationStatus AsyncSystemBlastTool :: Run ()
{
	OperationStatus status = OS_FAILED_TO_START;
	const char *command_line_s = sbt_args_processor_p -> GetArgsAsString ();
	char uuid_s [UUID_STRING_BUFFER_SIZE];

	ConvertUUIDToString (bt_job_p -> bsj_job.sj_id, uuid_s);

	SetServiceJobStatus (& (bt_job_p -> bsj_job), status);

	if (command_line_s)
		{
			if (SetSystemAsyncTaskCommand	(asbt_task_p, command_line_s))
				{
					JobsManager *manager_p = GetJobsManager ();

					#if ASYNC_SYSTEM_BLAST_TOOL_DEBUG >= STM_LEVEL_FINE
					PrintLog (STM_LEVEL_FINE, __FILE__, __LINE__, "About to run SystemBlastTool with \"%s\"", command_line_s);
					#endif

					if (AddServiceJobToJobsManager (manager_p, bt_job_p -> bsj_job.sj_id, (ServiceJob *) bt_job_p))
						{
							status = OS_PENDING;
							SetServiceJobStatus (& (bt_job_p -> bsj_job), status);

							if (RunSystemAsyncTask (asbt_task_p))
								{
									/*
									 * The ServiceJob should now only be writeable by the SystemAsyncTask that it is running under.
									 */
									status = OS_STARTED;
								}
							else
								{
									status = OS_FAILED_TO_START;
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to run async task for uuid %s", uuid_s);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add Blast Service Job \"%s\" to jobs manager", uuid_s);
						}

					#if ASYNC_SYSTEM_BLAST_TOOL_DEBUG >= STM_LEVEL_FINE
					PrintLog (STM_LEVEL_FINE, __FILE__, __LINE__, "Created async task for uuid %s", uuid_s);
					#endif
				}
			else
				{
					SetServiceJobStatus (& (bt_job_p -> bsj_job), status);
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set command \"%s\" for uuid \"%s\"", command_line_s, uuid_s);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get command to run for uuid \"%s\"", uuid_s);
		}

	if ((status == OS_ERROR) || (status == OS_FAILED_TO_START) || (status == OS_FAILED))
		{
			char *log_s = GetLog ();

			SetServiceJobStatus (& (bt_job_p -> bsj_job), status);

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
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AsyncSystemBlastTool :: AddToJSON failed");
		}

	return success_flag;
}



OperationStatus AsyncSystemBlastTool :: GetStatus (bool update_flag)
{
	OperationStatus status = GetCachedServiceJobStatus (& (bt_job_p -> bsj_job));

/*
	if (update_flag)
		{
			JobsManager *manager_p = GetJobsManager ();
			ServiceJob *job_p = GetServiceJobFromJobsManager (manager_p, bt_job_p -> bsj_job.sj_id);

			if (job_p)
				{
					status = job_p -> sj_status;
					SetServiceJobStatus (& (bt_job_p -> bsj_job), status);
				}

			#if ASYNC_SYSTEM_BLAST_TOOL_DEBUG >= STM_LEVEL_FINE
				{
					char uuid_s [UUID_STRING_BUFFER_SIZE];

					ConvertUUIDToString (bt_job_p -> bsj_job.sj_id, uuid_s);
					PrintLog (STM_LEVEL_FINE, __FILE__, __LINE__, "Status " INT32_FMT " for uuid %s", status, uuid_s);
				}
			#endif


			if ((status == OS_SUCCEEDED) || (status == OS_PARTIALLY_SUCCEEDED) || (status == OS_FINISHED))
				{
					if (bt_job_p -> bsj_job.sj_result_p == NULL)
						{
							if (!DetermineBlastResult (bt_job_p))
								{

								}
						}
				}
		}
	else
		{
			status = GetCachedServiceJobStatus (& (bt_job_p -> bsj_job));
		}
*/

	return status;
}




char *AsyncSystemBlastTool :: GetResults (BlastFormatter *formatter_p)
{
	char *results_s = ExternalBlastTool :: GetResults (formatter_p);
	JobsManager *jobs_manager_p = GetJobsManager ();

	/*
	 * Remove the ServiceJob from the JobsManager
	 */
	//RemoveServiceJobFromJobsManager (jobs_manager_p, bt_job_p -> bsj_job.sj_id, false);


	return results_s;
}



static bool UpdateAsyncBlastServiceJob (struct ServiceJob *job_p)
{
	BlastServiceJob *blast_job_p = reinterpret_cast <BlastServiceJob *> (job_p);
	AsyncSystemBlastTool *tool_p = static_cast <AsyncSystemBlastTool *> (blast_job_p -> bsj_tool_p);

	tool_p -> GetStatus (true);

	return true;
}

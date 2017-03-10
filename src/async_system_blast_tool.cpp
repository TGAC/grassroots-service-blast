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
#include "process.h"


const char * const AsyncSystemBlastTool :: ASBT_PROCESS_ID_S = "process_id";


AsyncSystemBlastTool :: AsyncSystemBlastTool (BlastServiceJob *job_p, const char *name_s, const char *factory_s, const BlastServiceData *data_p, const char *blast_program_name_s)
: SystemBlastTool (job_p, name_s, factory_s, data_p, blast_program_name_s),
	asbt_async_logfile_s (0)
{

}


AsyncSystemBlastTool :: ~AsyncSystemBlastTool ()
{

}



AsyncSystemBlastTool :: AsyncSystemBlastTool (BlastServiceJob *job_p, const BlastServiceData *data_p, const json_t *root_p)
: SystemBlastTool (job_p, data_p, root_p), asbt_async_logfile_s (0)
{
	if (!Init (ebt_blast_s))
		{
			throw std :: bad_alloc ();
		}
}


OperationStatus AsyncSystemBlastTool :: Run ()
{
	int res;
	OperationStatus status = OS_FAILED_TO_START;
	const char *command_line_s = sbt_args_processor_p -> GetArgsAsString ();

	#if SYSTEM_BLAST_TOOL_DEBUG >= STM_LEVEL_FINE
	PrintLog (STM_LEVEL_FINE, __FILE__, __LINE__, "About to run SystemBlastTool with \"%s\"", command_line_s);
	#endif


	if (command_line_s)
		{
			char *copied_cl_s = CopyToNewString (command_line_s, 0, false);

			if (copied_cl_s)
				{
					asbt_process_id = CreateProcess (copied_cl_s, 0, asbt_async_logfile_s);
					status = GetProcessStatus (asbt_process_id);

					FreeCopiedString (copied_cl_s);
				}
		}


	if ((status = OS_ERROR) || (status = OS_FAILED_TO_START) || (status == OS_FAILED))
		{
			char *log_s = GetLog ();

			status = OS_FAILED;
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "\"%s\" returned %d", command_line_s, res);

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
			if (asbt_process_id  != -1)
				{
					if (json_object_set_new (root_p, AsyncSystemBlastTool :: ASBT_PROCESS_ID_S, json_integer (asbt_process_id)) != 0)
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
			status = GetProcessStatus (asbt_process_id);
			SetServiceJobStatus (& (bt_job_p -> bsj_job), status);
		}
	else
		{
			status = GetCachedServiceJobStatus (& (bt_job_p -> bsj_job));
		}

	return status;
}






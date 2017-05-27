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
 * system_blast_tool.cpp
 *
 *  Created on: 22 Apr 2015
 *      Author: tyrrells
 */

#include "system_blast_tool.hpp"
#include "streams.h"
#include "string_utils.h"
#include "blast_service_job.h"


#ifdef _DEBUG
	#define SYSTEM_BLAST_TOOL_DEBUG	(STM_LEVEL_FINER)
#else
	#define SYSTEM_BLAST_TOOL_DEBUG	(STM_LEVEL_NONE)
#endif




SystemBlastTool :: SystemBlastTool (BlastServiceJob *job_p, const char *name_s, const char *factory_s, const BlastServiceData *data_p, const char *blast_program_name_s)
: ExternalBlastTool (job_p, name_s, factory_s, data_p, blast_program_name_s, false)
{
	if (!Init (blast_program_name_s))
		{
			throw std :: bad_alloc ();
		}
}


SystemBlastTool :: SystemBlastTool (BlastServiceJob *job_p, const BlastServiceData *data_p, const json_t *root_p)
	: ExternalBlastTool (job_p, data_p, root_p),  sbt_args_processor_p (0)
{
	if (!Init (ebt_blast_s))
		{
			throw std :: bad_alloc ();
		}
}


bool SystemBlastTool :: Init (const char *prog_s)
{
	bool success_flag = false;

	sbt_args_processor_p = new ByteBufferArgsProcessor ();

	if (AddBlastArg (prog_s, false))
		{
			success_flag = true;
		}

	return success_flag;
}



SystemBlastTool :: ~SystemBlastTool ()
{
	if (sbt_args_processor_p)
		{
			delete sbt_args_processor_p;
		}
}


bool SystemBlastTool :: ParseParameters (ParameterSet *params_p, BlastAppParameters *app_params_p)
{
	bool success_flag = false;

	if (ExternalBlastTool :: ParseParameters (params_p, app_params_p))
		{
			char *logfile_s = GetJobFilename (NULL, BS_LOG_SUFFIX_S);

			if (logfile_s)
				{
					if (AddBlastArg (">>", false))
						{
							if (AddBlastArg (logfile_s, false))
								{
									if (AddBlastArg ("2>&1", false))
										{
											success_flag = true;
										}
								}
						}

					FreeCopiedString (logfile_s);
				}

		}		/* if (ExternalBlastTool :: ParseParameters (params_p)) */

	return success_flag;
}


ArgsProcessor *SystemBlastTool :: GetArgsProcessor ()
{
	return sbt_args_processor_p;
}


OperationStatus SystemBlastTool :: Run ()
{
	int res;
	OperationStatus status = OS_FAILED_TO_START;
	const char *command_line_s = sbt_args_processor_p -> GetArgsAsString ();
	OperationStatus cached_status;

	#if SYSTEM_BLAST_TOOL_DEBUG >= STM_LEVEL_FINE
	PrintLog (STM_LEVEL_FINE, __FILE__, __LINE__, "About to run SystemBlastTool with \"%s\"", command_line_s);
	#endif

	status = OS_STARTED;

	res = system (command_line_s);

	if (res == 0)
		{
			status = OS_SUCCEEDED;

			SetServiceJobStatus (& (bt_job_p -> bsj_job), status);

			if (!DetermineBlastResult (bt_job_p))
				{
					char job_id_s [UUID_STRING_BUFFER_SIZE];

					ConvertUUIDToString (bt_job_p -> bsj_job.sj_id, job_id_s);
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add result to service job  \"%s\"", job_id_s);

					status = OS_FAILED;
				}
		}
	else
		{
			char *log_s = GetLog ();

			status = OS_FAILED;
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "\"%s\" returned %d", command_line_s, res);

			if (log_s)
				{
					if (!AddErrorToServiceJob (& (bt_job_p -> bsj_job), ERROR_S, log_s))
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add error \"%s\" to service job", log_s);
						}

					FreeCopiedString (log_s);
				}		/* if (log_s) */
		}

	cached_status = GetCachedServiceJobStatus (& (bt_job_p -> bsj_job));

	if (cached_status != status)
		{
			SetServiceJobStatus (& (bt_job_p -> bsj_job), status);
		}

	return status;
}





OperationStatus SystemBlastTool :: GetStatus (bool update_flag)
{
	OperationStatus status = OS_ERROR;

	if (update_flag)
		{
			status = GetServiceJobStatus (& (bt_job_p -> bsj_job));
		}
	else
		{
			status = GetCachedServiceJobStatus (& (bt_job_p -> bsj_job));
		}

	return status;
}



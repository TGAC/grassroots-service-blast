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
#include "blast_tool.hpp"

#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <syslog.h>

#include "blast_service.h"
#include "blast_service_job.h"
#include "io_utils.h"

#include "byte_buffer.h"
#include "string_utils.h"
#include "jansson.h"
#include "json_util.h"


const char * const BlastTool :: BT_NAME_S = "name";

const char * const BlastTool :: BT_FACTORY_NAME_S = "factory";

const char * const BlastTool :: BT_OUTPUT_FORMAT_S = "output_format";

const char * const BlastTool :: BT_CUSTOM_OUTPUT_COLUMNS_S = "custom_output_format_columns";



#ifdef _DEBUG
	#define BLAST_TOOL_DEBUG	(STM_LEVEL_INFO)
#else
	#define BLAST_TOOL_DEBUG	(STM_LEVEL_NONE)
#endif



void FreeBlastTool (BlastTool *tool_p)
{
	delete tool_p;
}


OperationStatus RunBlast (BlastTool *tool_p)
{
	OperationStatus status = OS_IDLE;

	if (tool_p -> PreRun  ())
		{
			status = tool_p -> Run ();

			tool_p -> PostRun  ();
		}
	else
		{
			status = OS_FAILED_TO_START;
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to prepare run of BlastTool %s", tool_p -> GetName ());
		}

	return status;
}


OperationStatus GetBlastStatus (BlastTool *tool_p)
{
	return (tool_p -> GetStatus ());
}


/******************************/



BlastTool :: BlastTool (BlastServiceJob *service_job_p, const char *name_s, const char *factory_s, const BlastServiceData *data_p, const uint32 output_format, const char *custom_output_s)
{
	bt_job_p = service_job_p;

	bt_service_data_p = data_p;
	bt_output_format = output_format;

	bt_factory_name_s = EasyCopyToNewString (factory_s);
	if (!bt_factory_name_s)
		{
			throw std :: invalid_argument ("factory name not set");
		}

	bt_name_s = EasyCopyToNewString (name_s);
	if (!bt_name_s)
		{
			throw std :: invalid_argument ("name not set");
		}

	if (custom_output_s)
		{
			bt_custom_output_columns_s = EasyCopyToNewString (custom_output_s);
			if (!bt_custom_output_columns_s)
				{
					throw std :: invalid_argument ("bt_custom_output_columns_s not set");
				}
		}
	else
		{
			bt_custom_output_columns_s = nullptr;
		}

/*
	if (service_job_p)
		{
			SetServiceJobName (& (bt_job_p -> bsj_job), name_s);
		}
*/

	#if BLAST_TOOL_DEBUG >= STM_LEVEL_FINEST
	PrintLog (STM_LEVEL_FINEST, __FILE__, __LINE__, "BlastTool constructor at 0x%.16X for job 0x.16X", this, service_job_p);
	#endif

}



BlastTool :: BlastTool (BlastServiceJob *job_p, const BlastServiceData *data_p, const json_t *root_p)
{
	uint32 output_format = BS_DEFAULT_OUTPUT_FORMAT;

	bt_factory_name_s = GetCopiedJSONString (root_p, BT_FACTORY_NAME_S);
	if (!bt_factory_name_s)
		{
			throw std :: invalid_argument ("factory name not set");
		}

	bt_name_s = GetCopiedJSONString (root_p, BT_NAME_S);
	if (!bt_name_s)
		{
			throw std :: invalid_argument ("name not set");
		}

	const char *custom_columns_s = GetJSONString (root_p, BT_CUSTOM_OUTPUT_COLUMNS_S);

	if (custom_columns_s)
		{
			bt_custom_output_columns_s = EasyCopyToNewString (custom_columns_s);
			if (!bt_custom_output_columns_s)
				{
					throw std :: invalid_argument ("bt_custom_output_columns_s not set");
				}
		}
	else
		{
			bt_custom_output_columns_s = nullptr;
		}

	GetJSONInteger (root_p, BT_OUTPUT_FORMAT_S, (int *) &output_format);
	bt_output_format = output_format;


	bt_job_p = job_p;
	bt_service_data_p = data_p;




	#if BLAST_TOOL_DEBUG >= STM_LEVEL_FINEST
	PrintLog (STM_LEVEL_FINEST, __FILE__, __LINE__, "BlastTool constructor at 0x%.16X for job 0x%.16X", this, job_p);
	#endif
}




OperationStatus BlastTool :: GetStatus (bool update_flag)
{
	OperationStatus status;

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

const uuid_t &BlastTool :: GetUUID () const
{
	return bt_job_p -> bsj_job.sj_id;
}


const char *BlastTool :: GetName () const
{
	return bt_name_s;
}


const char *BlastTool :: GetFactoryName () const
{
	return bt_factory_name_s;
}


BlastTool :: ~BlastTool ()
{
	#if BLAST_TOOL_DEBUG >= STM_LEVEL_FINEST
	PrintLog (STM_LEVEL_FINEST, __FILE__, __LINE__, "BlastTool destructor for 0x%.16X", this);
	#endif

	if (bt_factory_name_s)
		{
			FreeCopiedString (bt_factory_name_s);
		}

	if (bt_name_s)
		{
			FreeCopiedString (bt_name_s);
		}

	if (bt_custom_output_columns_s)
		{
			FreeCopiedString (bt_custom_output_columns_s);
		}
}


bool BlastTool :: PreRun ()
{
	SetServiceJobStatus (& (bt_job_p -> bsj_job), OS_STARTED);

	return true;
}


void BlastTool :: PostRun ()
{
}


uint32 BlastTool :: GetOutputFormat () const
{
	return bt_output_format;
}



json_t *BlastTool :: GetAsJSON ()
{
	json_t *blast_tool_json_p = json_object ();

	if (blast_tool_json_p)
		{
			if (!AddToJSON (blast_tool_json_p))
				{
					json_decref (blast_tool_json_p);
					blast_tool_json_p = 0;
				}
		}

	return blast_tool_json_p;
}




bool BlastTool :: AddToJSON (json_t *root_p)
{
	bool success_flag = false;

	if (json_object_set_new (root_p, BT_NAME_S, json_string (bt_name_s)) == 0)
		{
			if (json_object_set_new (root_p, BT_FACTORY_NAME_S, json_string (bt_factory_name_s)) == 0)
				{
					if (json_object_set_new (root_p, BT_OUTPUT_FORMAT_S, json_integer (bt_output_format)) == 0)
						{
							if (bt_custom_output_columns_s)
								{
									success_flag = SetJSONString (root_p, BT_CUSTOM_OUTPUT_COLUMNS_S, bt_custom_output_columns_s);
								}
							else
								{
									success_flag = SetJSONNull (root_p, BT_CUSTOM_OUTPUT_COLUMNS_S);
								}

							if (!success_flag)
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s:%s to BlastTool json", BT_CUSTOM_OUTPUT_COLUMNS_S, bt_custom_output_columns_s ? bt_custom_output_columns_s : "null");
								}

						}		/* if (json_object_set_new (root_p, BT_OUTPUT_FORMAT_S, json_integer (bt_output_format)) == 0) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s:%d to BlastTool json", BT_OUTPUT_FORMAT_S, bt_output_format);
						}
				}		/* if (json_object_set_new (root_p, BT_FACTORY_NAME_S, json_string (bt_factory_name_s)) == 0) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s:%s to BlastTool json", BT_FACTORY_NAME_S, bt_factory_name_s);
				}

		}		/* if (json_object_set_new (root_p, BT_NAME_S, json_string (bt_name_s)) == 0) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s:%s to BlastTool json", BT_NAME_S, bt_name_s);
		}

	return success_flag;
}




bool BlastTool :: AddErrorDetails ()
{
	bool success_flag = false;
	char *log_s = GetLog ();

	if (log_s)
		{
			if (AddGeneralErrorMessageToServiceJob (& (bt_job_p -> bsj_job), log_s))
				{
					success_flag = true;
				}
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add error \"%s\" to service job", log_s);
				}

			FreeCopiedString (log_s);
		}		/* if (log_s) */
	else
		{
			char uuid_s [UUID_STRING_BUFFER_SIZE];

			ConvertUUIDToString (bt_job_p -> bsj_job.sj_id, uuid_s);

			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "GetLog () failed for \"%s\"", uuid_s);
		}

	return success_flag;
}


bool BlastTool :: SetCustomOutputColumns (const char *custom_columns_s)
{
	bool success_flag = false;

	if (custom_columns_s)
		{
			char *temp_s = EasyCopyToNewString (custom_columns_s);

			if (temp_s)
				{
					if (bt_custom_output_columns_s)
						{
							FreeCopiedString (bt_custom_output_columns_s);
						}

					bt_custom_output_columns_s = temp_s;
					success_flag = true;
				}
		}
	else
		{
			if (bt_custom_output_columns_s)
				{
					FreeCopiedString (bt_custom_output_columns_s);
				}

			bt_custom_output_columns_s = nullptr;
			success_flag = true;
		}

	return success_flag;
}


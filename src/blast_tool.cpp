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
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <syslog.h>

#include "blast_tool.hpp"
#include "blast_service.h"

#include "io_utils.h"

#include "byte_buffer.h"
#include "string_utils.h"
#include "jansson.h"
#include "blast_service_job.h"
#include "json_util.h"


const char * const BlastTool :: BT_NAME_S = "name";

const char * const BlastTool :: BT_FACTORY_NAME_S = "factory";

const char * const BlastTool :: BT_OUTPUT_FORMAT_S = "output_format";



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



BlastTool :: BlastTool (BlastServiceJob *service_job_p, const char *name_s, const char *factory_s, const BlastServiceData *data_p, const uint32 output_format)
{
	bt_job_p = service_job_p;
	bt_name_s = name_s;
	bt_service_data_p = data_p;
	bt_factory_name_s = factory_s;
	bt_output_format = output_format;

/*
	if (service_job_p)
		{
			SetServiceJobName (& (bt_job_p -> bsj_job), name_s);
		}
*/
}



BlastTool :: BlastTool (BlastServiceJob *job_p, const BlastServiceData *data_p, const json_t *root_p)
{
	uint32 output_format = BS_DEFAULT_OUTPUT_FORMAT;

	bt_factory_name_s = GetJSONString (root_p, BT_FACTORY_NAME_S);
	if (!bt_factory_name_s)
		{
			throw std :: invalid_argument ("factory name not set");
		}

	bt_name_s = GetJSONString (root_p, BT_NAME_S);
	if (!bt_name_s)
		{
			throw std :: invalid_argument ("name not set");
		}

	GetJSONInteger (root_p, BT_OUTPUT_FORMAT_S, (int *) &output_format);
	bt_output_format = output_format;


	bt_job_p = job_p;
	bt_service_data_p = data_p;
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
							success_flag = true;
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


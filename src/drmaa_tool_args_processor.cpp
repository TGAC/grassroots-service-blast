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
 * drmaa_args_processor.cpp
 *
 *  Created on: 27 Oct 2016
 *      Author: billy
 */





#include "drmaa_tool_args_processor.hpp"

#include "string_utils.h"
#include "streams.h"



DrmaaToolArgsProcessor :: DrmaaToolArgsProcessor (DrmaaTool *drmaa_p)
	: dtap_drmaa_p (drmaa_p)
{

}


DrmaaToolArgsProcessor :: ~DrmaaToolArgsProcessor ()
{

}


bool DrmaaToolArgsProcessor :: AddArg (const char *arg_s, const bool hyphen_flag)
{
	bool success_flag = false;

	if (hyphen_flag)
		{
			char *value_s = ConcatenateStrings ("-", arg_s);

			if (value_s)
				{
					success_flag = AddDrmaaToolArgument (dtap_drmaa_p, value_s);
					FreeCopiedString (value_s);
				}
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to concatenate \"-\" and \"%s\"", arg_s);
				}
		}
	else
		{
			success_flag = AddDrmaaToolArgument (dtap_drmaa_p, arg_s);
		}

	return success_flag;
}

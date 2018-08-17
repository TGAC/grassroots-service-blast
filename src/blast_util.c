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
 * blast_util.c
 *
 *  Created on: 19 Oct 2016
 *      Author: billy
 */

#include <string.h>

#include "blast_util.h"
#include "math_utils.h"
#include "string_utils.h"


bool AddArgsPair (const char *key_s, const char *value_s, ArgsProcessor *ap_p)
{
	bool success_flag = false;

	if (ap_p -> AddArg (key_s, true))
		{
			if (ap_p -> AddArg (value_s, false))
				{
					success_flag = true;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add value arg for \"%s\"=\"%s\"", key_s, value_s);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add key arg for \"%s\"=\"%s\"", key_s, value_s);
		}

	return success_flag;
}


bool GetAndAddBlastArgs (const ParameterSet *param_set_p, const char *param_name_s, bool required_flag, ArgsProcessor *ap_p)
{
	bool success_flag = !required_flag;
	const Parameter *param_p = GetParameterFromParameterSetByName (param_set_p, param_name_s);

	if (param_p)
		{
			success_flag = AddBlastArgs (param_p, ap_p);
		}

	return success_flag;
}


bool AddBlastArgs (const Parameter *param_p, ArgsProcessor *ap_p)
{
	bool success_flag = false;

	switch (param_p -> pa_type)
		{
			case PT_STRING:
			case PT_DIRECTORY:
			case PT_FILE_TO_READ:
			case PT_FILE_TO_WRITE:
			case PT_LARGE_STRING:
			case PT_KEYWORD:
			case PT_PASSWORD:
				success_flag = AddArgsPair (param_p -> pa_name_s, param_p -> pa_current_value.st_string_value_s, ap_p);
				break;

			case PT_SIGNED_INT:
			case PT_NEGATIVE_INT:
				{
					char *value_s = ConvertIntegerToString (param_p -> pa_current_value.st_long_value);

					if (value_s)
						{
							success_flag = AddArgsPair (param_p -> pa_name_s, value_s, ap_p);
							FreeCopiedString (value_s);
						}		/* if (value_s) */
				}
				break;

			case PT_UNSIGNED_INT:
				{
					char *value_s = ConvertIntegerToString (param_p -> pa_current_value.st_ulong_value);

					if (value_s)
						{
							success_flag = AddArgsPair (param_p -> pa_name_s, value_s, ap_p);
							FreeCopiedString (value_s);
						}		/* if (value_s) */
				}
				break;

			case PT_SIGNED_REAL:
			case PT_UNSIGNED_REAL:
				{
					char *value_s = ConvertDoubleToString (param_p -> pa_current_value.st_data_value);

					if (value_s)
						{
							success_flag = AddArgsPair (param_p -> pa_name_s, value_s, ap_p);
							FreeCopiedString (value_s);
						}		/* if (value_s) */

				}
				break;

			case PT_BOOLEAN:
				success_flag = ap_p -> AddArg (param_p -> pa_name_s, true);
				break;

			default:
				break;

		}		/* switch (param_p -> pa_type) */

	return success_flag;
}


bool AddArgsPairFromStringParameter (const ParameterSet *params_p, const char * const param_name_s, const char *key_s, ArgsProcessor *ap_p,  const bool required_flag)
{
	bool success_flag = !required_flag;
	SharedType value;

	InitSharedType (&value);

	if (GetParameterValueFromParameterSet (params_p, param_name_s, &value, true))
		{
			success_flag = AddArgsPair (key_s, value.st_string_value_s, ap_p);
		}

	return success_flag;
}


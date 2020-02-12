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

#include "blast_util.h"

#include <string.h>

#include "math_utils.h"
#include "string_utils.h"

#include "boolean_parameter.h"
#include "double_parameter.h"
#include "signed_int_parameter.h"
#include "string_parameter.h"
#include "unsigned_int_parameter.h"


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

	if (IsStringParameter (param_p))
		{
			StringParameter *str_param_p = (StringParameter *) param_p;
			const char *value_s = GetStringParameterCurrentValue (str_param_p);

			if (value_s)
				{
					success_flag = AddArgsPair (param_p -> pa_name_s, value_s, ap_p);
				}
		}
	else if (IsSignedIntParameter (param_p))
		{
			SignedIntParameter *int_param_p = (SignedIntParameter *) param_p;
			const int32 *value_p = GetSignedIntParameterCurrentValue (int_param_p);

			if (value_p)
				{
					char *value_s = ConvertIntegerToString (*value_p);

					if (value_s)
						{
							success_flag = AddArgsPair (param_p -> pa_name_s, value_s, ap_p);
							FreeCopiedString (value_s);
						}
				}		/* if (value_s) */
		}
	else if (IsUnsignedIntParameter (param_p))
		{
			UnsignedIntParameter *int_param_p = (UnsignedIntParameter *) param_p;
			const uint32 *value_p = GetUnsignedIntParameterCurrentValue (int_param_p);

			if (value_p)
				{
					char *value_s = ConvertUnsignedIntegerToString (*value_p);

					if (value_s)
						{
							success_flag = AddArgsPair (param_p -> pa_name_s, value_s, ap_p);
							FreeCopiedString (value_s);
						}
				}		/* if (value_s) */
		}
	else if (IsDoubleParameter (param_p))
		{
			DoubleParameter *dbl_param_p = (DoubleParameter *) param_p;
			const double64 *value_p = GetDoubleParameterCurrentValue (dbl_param_p);

			if (value_p)
				{
					char *value_s = ConvertDoubleToString (*value_p);

					if (value_s)
						{
							success_flag = AddArgsPair (param_p -> pa_name_s, value_s, ap_p);
							FreeCopiedString (value_s);
						}
				}		/* if (value_s) */
		}
	else if (IsBooleanParameter (param_p))
		{
			BooleanParameter *bool_param_p = (BooleanParameter *) param_p;
			const bool *value_p = GetBooleanParameterCurrentValue (bool_param_p);

			if (value_p)
				{
					success_flag = ap_p -> AddArg (param_p -> pa_name_s, *value_p);
				}		/* if (value_s) */

		}

	return success_flag;
}


bool AddArgsPairFromStringParameter (const ParameterSet *params_p, const char * const param_name_s, const char *key_s, ArgsProcessor *ap_p,  const bool required_flag)
{
	bool success_flag = !required_flag;
	const char *value_s = NULL;

	if (GetCurrentStringParameterValueFromParameterSet (params_p, param_name_s, &value_s))
		{
			success_flag = AddArgsPair (key_s, value_s, ap_p);
		}

	return success_flag;
}


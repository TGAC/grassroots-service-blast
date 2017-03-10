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
 * byte_buffer_args_processor.cpp
 *
 *  Created on: 27 Oct 2016
 *      Author: billy
 */


#include "byte_buffer_args_processor.hpp"
#include "string_utils.h"
#include "alloc_failure.hpp"


ByteBufferArgsProcessor :: ByteBufferArgsProcessor ()
{
	bbap_buffer_p = AllocateByteBuffer (1024);

	if (!bbap_buffer_p)
		{
			throw AllocFailure ("Failed to create data for ByteBufferArgsProcessor");
		}
}


ByteBufferArgsProcessor :: ~ByteBufferArgsProcessor ()
{
	FreeByteBuffer (bbap_buffer_p);
}


bool ByteBufferArgsProcessor :: AddArg (const char *arg_s, const bool hyphen_flag)
{
	bool success_flag = true;
	const bool add_quotes_flag = DoesStringContainWhitespace (arg_s);

	if (bbap_buffer_p -> bb_current_index != 0)
		{
			success_flag = AppendStringToByteBuffer (bbap_buffer_p, " ");
		}

	if (success_flag && hyphen_flag)
		{
			success_flag = AppendStringToByteBuffer (bbap_buffer_p, "-");
		}

	if (success_flag)
		{
			if (add_quotes_flag)
				{
					success_flag = AppendStringsToByteBuffer (bbap_buffer_p, "\"", arg_s, "\"", NULL);
				}
			else
				{
					success_flag = AppendStringToByteBuffer (bbap_buffer_p, arg_s);
				}
		}

	return success_flag;
}


const char *ByteBufferArgsProcessor :: GetArgsAsString ()
{
	return GetByteBufferData (bbap_buffer_p);
}

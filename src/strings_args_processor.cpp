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
 * strings_args_processor.cpp
 *
 *  Created on: 8 Mar 2017
 *      Author: billy
 */

#include "strings_args_processor.hpp"

#include "string_utils.h"
#include "alloc_failure.hpp"


StringsArgsProcessor ::	StringsArgsProcessor ()
{
	sap_args_p = AllocateStringLinkedList ();

	if (!sap_args_p)
		{
			throw new AllocFailure ("Couldn't allocate StringLinkedList");
		}

}


StringsArgsProcessor :: ~StringsArgsProcessor ()
{
	FreeLinkedList (sap_args_p);
}


bool StringsArgsProcessor :: AddArg (const char *arg_s, const bool hyphen_flag)
{
	bool success_flag = true;
	const bool add_quotes_flag = DoesStringContainWhitespace (arg_s);
	char *value_s = NULL;

	if (hyphen_flag)
		{
			if (add_quotes_flag)
				{
					value_s = ConcatenateVarargsStrings ("-\"", arg_s, "\"", NULL);
				}
			else
				{
					value_s = ConcatenateVarargsStrings ("-", arg_s, NULL);
				}
		}
	else
		{
			if (add_quotes_flag)
				{
					value_s = ConcatenateVarargsStrings ("\"", arg_s, "\"", NULL);
				}
			else
				{
					value_s = CopyToNewString (arg_s, 0, false);
				}
		}


	if (value_s)
		{
			StringListNode *node_p = AllocateStringListNode (value_s, MF_SHALLOW_COPY);

			if (node_p)
				{
					LinkedListAddTail (sap_args_p, & (node_p -> sln_node));
					success_flag = true;
				}
		}

	return success_flag;
}


char **StringsArgsProcessor :: GetArgsAsStrings ()
{
	char **args_ss = static_cast <char **> (AllocMemoryArray (1 + sap_args_p -> ll_size, sizeof (char *)));

	if (args_ss)
		{
			char **arg_ss = args_ss;
			StringListNode *node_p = reinterpret_cast <StringListNode *> (sap_args_p -> ll_head_p);

			while (node_p)
				{
					*arg_ss = node_p -> sln_string_s;

					++ arg_ss;
					node_p = reinterpret_cast <StringListNode *> (node_p -> sln_node.ln_next_p);
				}		/* while (node_p) */

			*arg_ss = 0;
		}


	return args_ss;
}



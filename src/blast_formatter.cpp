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
 * blast_formatter.cpp
 *
 *  Created on: 8 Jan 2016
 *      Author: billy
 */

#include <new>

#include "blast_formatter.h"
#include "byte_buffer.h"
#include "string_utils.h"
#include "streams.h"
#include "math_utils.h"


BlastFormatter :: BlastFormatter ()
{}


BlastFormatter :: ~BlastFormatter ()
{}


char *BlastFormatter :: GetConvertedOutputFilename (const char * const filename_s, const int output_format_code, char **output_format_code_ss)
{
	char *output_filename_s = NULL;
	char *output_format_code_s = ConvertIntegerToString (output_format_code);

	if (output_format_code_s)
		{
			output_filename_s = ConcatenateVarargsStrings (filename_s, ".", output_format_code_s, NULL);

			if (output_format_code_ss)
				{
					*output_format_code_ss = output_format_code_s;
				}
			else
				{
					FreeCopiedString (output_format_code_s);
				}
		}

	return output_filename_s;
}



SystemBlastFormatter *SystemBlastFormatter :: Create (const json_t *config_p)
{
	SystemBlastFormatter *formatter_p = 0;
	const char *formatter_command_s = GetJSONString (config_p, "command");

	if (formatter_command_s)
		{
			try
				{
					formatter_p = new SystemBlastFormatter (formatter_command_s);
				}
			catch (std :: bad_alloc &alloc_r)
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate SystemBlastFormatter");
				}
		}

	return formatter_p;
}


SystemBlastFormatter :: SystemBlastFormatter (const char * const blast_formatter_command_s)
 : BlastFormatter ()
{
	sbf_blast_formatter_command_s = blast_formatter_command_s;
}


SystemBlastFormatter :: ~SystemBlastFormatter ()
{
}


char *SystemBlastFormatter :: GetConvertedOutput (const char * const input_filename_s, const uint32 output_format_code)
{
	char *result_s = NULL;

	if (sbf_blast_formatter_command_s)
		{
			ByteBuffer *buffer_p = AllocateByteBuffer (1024);

			if (buffer_p)
				{
					char *output_format_code_s = NULL;
					char *output_filename_s = BlastFormatter :: GetConvertedOutputFilename (input_filename_s, output_format_code, &output_format_code_s);

					if (output_filename_s)
						{
							if (AppendStringsToByteBuffer (buffer_p, sbf_blast_formatter_command_s, " -archive ", input_filename_s, " -outfmt ", output_format_code_s, " -out ", output_filename_s, NULL))
								{
									const char *command_line_s = GetByteBufferData (buffer_p);
									int res = system (command_line_s);

									if (res == 0)
										{
											FILE *converted_output_f = fopen (output_filename_s, "r");

											if (converted_output_f)
												{
													result_s = GetFileContentsAsString (converted_output_f);

													if (!result_s)
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get contents of \"%s\"", output_filename_s);
														}		/* if (!result_s) */

													if (fclose (converted_output_f) != 0)
														{
															PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to close \"%s\"", output_filename_s);
														}

												}		/* if (converted_output_f) */
											else
												{
													PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to open \"%s\"", output_filename_s);
												}

										}		/* if (res == 0) */
									else
										{
											PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to run \"%s\" with code %d", command_line_s, res);
										}

								}		/* if (AppendStringToByteBuffer (buffer_p, sbf_blast_formatter_command_s, " -archive ", input_filename_s, " -outfmt ", output_format_code_s, " -out ", output_filename_s, NULL)) */
							else
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to build command line buffer for \"%s\"", input_filename_s);
								}

							FreeCopiedString (output_format_code_s);
							FreeCopiedString (output_filename_s);
						}		/* if (output_filename_s) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get create output filename for \"%s\" with format code %d", input_filename_s, output_format_code);
						}

					FreeByteBuffer (buffer_p);
				}		/* if (buffer_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate ByteBuffer");
				}

		}		/* if (sbf_blast_formatter_command_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Formatter command is NULL");
		}

	return result_s;
}



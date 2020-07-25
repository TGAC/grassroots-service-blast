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

#include "blast_formatter.h"

#include <new>

#include "byte_buffer.h"
#include "string_utils.h"
#include "streams.h"
#include "math_utils.h"
#include "blast_service_params.h"
#include "blast_util.h"
#include "blast_service.h"



const char * const SystemBlastFormatter :: SBF_LOG_SUFFIX_S = ".formatter.log";


BlastFormatter :: BlastFormatter ()
{}


BlastFormatter :: ~BlastFormatter ()
{}



char *BlastFormatter :: GetConvertedOutputFilename (const char * const filename_s, const uint32 output_format_code)
{
	char *output_filename_s = NULL;
	char *output_format_code_s = ConvertUnsignedIntegerToString (output_format_code);

	if (output_format_code_s)
		{
			output_filename_s = ConcatenateVarargsStrings (filename_s, ".", output_format_code_s, NULL);
			FreeCopiedString (output_format_code_s);
		}

	return output_filename_s;
}


bool BlastFormatter :: IsCustomisableOutputFormat (const uint32 output_format_code)
{
	return ((output_format_code == BOF_TABULAR) || (output_format_code == BOF_TABULAR_WITH_COMMENTS) || (output_format_code == BOF_CSV));
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



char *SystemBlastFormatter :: GetOutputFormatAsString (const uint32 output_format_code, const char *custom_output_formats_s)
{
	char *output_format_code_s = ConvertUnsignedIntegerToString (output_format_code);

	if (output_format_code_s)
		{
			if (custom_output_formats_s && (BlastFormatter :: IsCustomisableOutputFormat (output_format_code)))
				{
					char *temp_s = ConcatenateVarargsStrings (output_format_code_s, " ", custom_output_formats_s, NULL);

					FreeCopiedString (output_format_code_s);
					output_format_code_s = temp_s;
				}
		}


	return output_format_code_s;
}




bool SystemBlastFormatter :: SaveCommandLine (const char *filename_s, const char *command_line_s)
{
	bool success_flag = false;
	char *formatter_command_s = ConcatenateStrings (filename_s, ".formatter");

	if (formatter_command_s)
		{
			success_flag = WriteCommandLineToFile (command_line_s, formatter_command_s);

			FreeCopiedString (formatter_command_s);
		}
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create formatter filename for ", filename_s);
		}

	return success_flag;
}



char *SystemBlastFormatter :: GetConvertedOutput (const char *job_id_s, const uint32 output_format_code, const char *custom_format_s, const BlastServiceData *data_p)
{
	char *result_s = NULL;

	if (sbf_blast_formatter_command_s)
		{
			ByteBuffer *buffer_p = AllocateByteBuffer (1024);

			if (buffer_p)
				{
					char *input_filename_s = GetBlastJobFilename (data_p -> bsd_working_dir_s, job_id_s, BS_OUTPUT_SUFFIX_S);

					if (input_filename_s)
						{
							char *output_filename_s = BlastFormatter :: GetConvertedOutputFilename (input_filename_s, output_format_code);

							if (output_filename_s)
								{
									char *output_format_params_s = GetOutputFormatAsString (output_format_code, custom_format_s);

									if (output_format_params_s)
										{
											char *logfile_s = GetBlastJobFilename (data_p -> bsd_working_dir_s, job_id_s, SystemBlastFormatter :: SBF_LOG_SUFFIX_S);

											if (logfile_s)
												{
													if (AppendStringsToByteBuffer (buffer_p, sbf_blast_formatter_command_s, " -archive ", input_filename_s, " -outfmt \"", output_format_params_s, "\" -out ", output_filename_s, " >> ", logfile_s, " 2>&1", NULL))
														{
															const char *command_line_s = GetByteBufferData (buffer_p);
															int res;

															if (!SaveCommandLine (input_filename_s, command_line_s))
																{
																	PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "SaveCommandLine failed for \"%s\" to file \"%s\"", command_line_s, input_filename_s);

																}

															res = system (command_line_s);

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


													FreeCopiedString (logfile_s);
												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get create logfile filename for job \"%s\"", job_id_s);
												}


											FreeCopiedString (output_format_params_s);
										}		/* if (output_format_params_s) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetOutputFormatAsString () failed  for " UINT32_FMT " and \"%s\"", output_format_code, custom_format_s ? custom_format_s : "null");
										}

									FreeCopiedString (output_filename_s);
								}		/* if (output_filename_s) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get create output filename for \"%s\" with format code %d", input_filename_s, output_format_code);
								}

							FreeCopiedString (input_filename_s);
						}		/* if (input_filename_s) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get create input filename for job \"%s\"", job_id_s);
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



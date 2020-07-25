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

/**
 * @file
 * @brief
 */

/*
 * blast_formatter.h
 *
 *  Created on: 8 Jan 2016
 *      Author: billy
 */

#ifndef BLAST_FORMATTER_H_
#define BLAST_FORMATTER_H_

#include "blast_service_api.h"
#include "jansson.h"


// forward declarations
struct BlastServiceData;


/**
 * This class is for converting the output of a blast
 * job between the available different formats. The
 * input format must be "BLAST archive format (ASN.1)"
 *
 * @ingroup blast_service
 */
class BLAST_SERVICE_LOCAL BlastFormatter
{
public:
	BlastFormatter ();

	/**
	 * The BlastFormatter destructor.
	 */
	virtual ~BlastFormatter ();

	/**
	 * Get the output filename for the converted output in a given format.
	 *
	 * @param filename_s The full path to the filename to convert.
	 * @param output_format_code The required output format code.
	 * @param output_format_code_ss The string for the format code gets stored here.
	 * @return The full path to the output filename for the given output format code
	 * or 0 upon error.
	 */
	static char *GetConvertedOutputFilename (const char * const filename_s, const uint32 output_format_code);

	/**
	 * The function to get the converted output as a c-style string.
	 *
	 * @param input_filename_s The file that is in "BLAST archive format (ASN.1)" format.
	 * @param output_format_code The required output code.
	 * @return The converted output or <code>NULL</code> upon error.
	 * @see BlastOutputFormat
	 */
	virtual char *GetConvertedOutput (const char *job_id_s, const uint32 output_format_code, const char *custom_format_s, const struct BlastServiceData *data_p) = 0;


	static bool IsCustomisableOutputFormat (const uint32 output_format_code);
};


/**
 * This class calls a command line executable to convert a blast
 * job between the available different formats. The executable used is
 * the value specified in the global Grassroots server configuration file by
 * the
 *
 * 		"services" -> "Blast service" -> "system_formatter_config" -> "command"
 *
 * key.
 */
class BLAST_SERVICE_LOCAL SystemBlastFormatter : public BlastFormatter
{
public:
	/**
	 * Create a SystemBlastFormatter using the given configuration.
	 *
	 * @param config_p The JSON fragment that is the value of "system_formatter_config"
	 * from within the Blast service section of the global Server configuration file.
	 * @return A new SystemBlastFormatter or 0 upon error.
	 */
	static SystemBlastFormatter *Create (const json_t *config_p);

	static const char * const SBF_LOG_SUFFIX_S;

	/**
	 * The SystemBlastFormatter destructor.
	 */
	virtual ~SystemBlastFormatter ();

	/**
	 * The function gets the converted output as a c-style string using its
	 * specified command line executable.
	 *
	 * @param input_filename_s The file that is in "BLAST archive format (ASN.1)" format.
	 * @param output_format_code The required output code.
	 * @return The converted outpur or <code>NULL</code> upon error.
	 * @see BlastFormatter::GetConvertedOutput
	 * @see BlastOutputFormat
	 */
	virtual char *GetConvertedOutput (const char *job_id_s, const uint32 output_format_code, const char *custom_format_s, const BlastServiceData *data_p);


	static char *GetOutputFormatAsString (const uint32 output_format_code, const char *custom_output_formats_s);



protected:
	/** The command line executable used to convert between the different formats */
	const char *sbf_blast_formatter_command_s;

	bool SaveCommandLine (const char *filename_s, const char *command_line_s);


private:
	/**
	 * Create a SystemBlastFormatter with the given command line executable
	 *
	 * @param blast_format_command_s The command line executable used to
	 * convert between the different formats.
	 */
	SystemBlastFormatter (const char * const blast_format_command_s);

};

#endif /* BLAST_FORMATTER_H_ */

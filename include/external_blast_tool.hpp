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
 * external_blast_tool.hpp
 *
 *  Created on: 22 Apr 2015
 *      Author: tyrrells
 */

#ifndef EXTERNAL_BLAST_TOOL_HPP_
#define EXTERNAL_BLAST_TOOL_HPP_

#include "blast_tool.hpp"
#include "byte_buffer.h"
#include "temp_file.hpp"


/**
 * A class that will run Blast using an external application such as blastn
 * rather than calling Blast library routines directly.
 *
 * @ingroup blast_service
 */
class BLAST_SERVICE_LOCAL ExternalBlastTool : public BlastTool
{
public:
	/**
	 * This is the string that will get appended to each
	 * input file used by the ExternalBlastTool.
	 */
	static const char * const EBT_INPUT_SUFFIX_S;

	/**
	 * This is the string that will get appended to each
	 * log file created when an ExternalBlastTool is ran.
	 */
	static const char * const EBT_LOG_SUFFIX_S;

	static const char * const EBT_ASYNC_S;


	/**
	 * Create a ExternalBlastTool for a given ServiceJob.
	 *
	 * @param job_p The ServiceJob to associate with this ExternalBlastTool.
	 * @param name_s The name to give to this ExternalBlastTool.
	 * @param factory_s The name of the BlastToolFactory that is creating this ExternalBlastTool.
	 * @param data_p The BlastServiceData for the Service that will run this ExternalBlastTool.
	 * @param blast_program_name_s The name of blast command line executable that this ExternalBlastTool
	 * will call to run its blast job.
	 */
	ExternalBlastTool (BlastServiceJob *job_p, const char *name_s, const char *factory_s, const BlastServiceData *data_p, const char *blast_program_name_s, const bool async_flag);


	/**
	 * Create a ExternalBlastTool for a given ServiceJob using the configuration details from
	 * a serialised JSON fragment.
	 *
	 * @param job_p The ServiceJob to associate with this ExternalBlastTool.
	 * @param data_p The BlastServiceData for the Service that will run this ExternalBlastTool.
	 * @param json_p The JSON fragment to fill in the serialised values such as job name, etc.
	 */
	ExternalBlastTool (BlastServiceJob *job_p, const BlastServiceData *data_p, const json_t *json_p);


	/**
	 * The ExternalBlastTool destructor.
	 */
	virtual ~ExternalBlastTool ();

	/**
	 * Parse a ParameterSet to configure a ExternalBlastTool prior
	 * to it being ran.
	 *
	 * @param param_set_p The ParameterSet to parse.
	 * @param app_params_p The BlastAppParameters to use process the
	 * values from the given ParameterSet.
	 * @return <code>true</code> if the BlastTool was configured
	 * successfully and is ready to be ran, <code>false</code>
	 * otherwise.
	 * @see BlastTool::ParseParameters
	 */
	virtual bool ParseParameters (ParameterSet *param_set_p, BlastAppParameters *app_params_p);


	/**
	 * Set the input filename for the BlastTool to use.
	 *
	 * @param filename_s The filename.
	 * @return <code>true</code> if the input filename was set
	 * successfully, <code>false</code> otherwise.
	 */
	virtual bool SetInputFilename (const char * const filename_s);


	/**
	 * Set the output file parameter.
	 *
	 * @return <code>true</code> if the input filename was set
	 * successfully, <code>false</code> otherwise.
	 */
	virtual bool SetUpOutputFile ();


	/**
	 * Get the results after the ExternalBlastTool has finished
	 * running.
	 *
	 * @param formatter_p The BlastFormatter to convert the results
	 * into a different format. If this is 0, then the results will be
	 * returned without any conversion.
	 * @return The results as a c-style string or 0 upon error.
	 */
	virtual char *GetResults (BlastFormatter *formatter_p);


	/**
	 * Get a copy of the log data from the run of this ExternalBlastTool's
	 * run.
	 *
	 * @return The log data or <code>0</code> upon error. If a valid value is
	 * returned, this will need to be freed by <code>FreeCopiedString</code>
	 * to avoid a memory leak.
	 * @see FreeCopiedString
	 */
	virtual char *GetLog ();

protected:

	/**
	 * The filename which the Blast results will be written to if the
	 * job runs successfully.
	 */
	char *ebt_results_filename_s;

	/**
	 *
	 * The working directory where all of the
	 * input, output and log files will be
	 * created.
	 */
	const char *ebt_working_directory_s;

	/**
	 *
	 * The name of the blast command line
	 * executable to use.
	 */
	const char *ebt_blast_s;



	bool ebt_async_flag;


	/**
	 * Get the ArgsProcessor that this BlastTool will use
	 * to parse the input ParameterSet prior to running its
	 * job.
	 *
	 * @return The ArgsProcessor for this BlastTool or
	 * <code>0</code> upon error.
	 */
	virtual ArgsProcessor *GetArgsProcessor () = 0;


	/**
	 * Add the argument to the command line arguments
	 * that this BlastTool will run with.
	 *
	 * @param arg_s The argument to add.
	 * @param hyphen_flag If this is <code>true</code> then a hyphen
	 * will be prefixed the given value e.g. turning "foo" into "-foo"
	 * before being added, if this is <code>false</code> then the argument
	 * is added unaltered.
	 * @return <code>true</code> if the argument was added
	 * successfully, <code>false</code> otherwise.
	 */
	virtual bool AddBlastArg (const char * const arg_s, const bool hyphen_flag);


	/**
	 * Add a pair of arguments to the command line arguments
	 * that this BlastTool will run with.
	 *
	 * @param key_s The first argument to add.
	 * @param value_s The second argument to add.
	 * @return <code>true</code> if the arguments were added
	 * successfully, <code>false</code> otherwise.
	 */
	virtual bool AddBlastArgsPair (const char *key_s, const char *value_s);



	/**
	 * Create the filename to use for a ServiceJob.
	 *
	 * @param prefix_s The prefix to use for the filename.
	 * @param suffix_s The suffix to use from a set of constants.
	 * @return The filename or 0 upon error.
	 * @see EBT_INPUT_SUFFIX_S
	 * @see EBT_LOG_SUFFIX_S
	 * @see BS_OUTPUT_SUFFIX_S
	 */
	char *GetJobFilename (const char * const prefix_s, const char * const suffix_s);


	/**
	 * This method is used to serialise this ExternalBlastTool so that
	 * it can be recreated from another calling process when required.
	 * This is used to store the ExternalBlastTool in the JobsManager.
	 *
	 * @param root_p The json object to add the serialisation of this ExternalBlastTool to.
	 * @return <code>true</code> if the ExternalBlastTool was serialised
	 * successfully, <code>false</code>
	 * otherwise.
	 */
	virtual bool AddToJSON (json_t *root_p);



private:
	static const char * const EBT_COMMAND_LINE_EXECUTABLE_S;
	static const char * const EBT_WORKING_DIR_S;
	static const char * const EBT_RESULTS_FILE_S;
};
#endif /* EXTERNAL_BLAST_TOOL_HPP_ */


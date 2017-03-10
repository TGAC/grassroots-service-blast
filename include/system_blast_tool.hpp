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
 * system_blast_tool.hpp
 *
 *  Created on: 22 Apr 2015
 *      Author: tyrrells
 */

#ifndef SYSTEM_BLAST_TOOL_HPP_
#define SYSTEM_BLAST_TOOL_HPP_

#include "external_blast_tool.hpp"
#include "byte_buffer_args_processor.hpp"

/**
 * A class that will run Blast as a system process.
 *
 * @ingroup blast_service
 */
class BLAST_SERVICE_LOCAL SystemBlastTool : public ExternalBlastTool
{
public:
	/**
	 * Create a SystemBlastTool for a given ServiceJob.
	 *
	 * @param service_job_p The ServiceJob to associate with this SystemBlastTool.
	 * @param name_s The name to give to this SystemBlastTool.
	 * @param factory_s The name of the DrmaaBlastFactory that is creating this DrmaaBlastTool.
	 * @param data_p The BlastServiceData for the Service that will run this SystemBlastTool.
	 * @param blast_program_name_s The name of blast command line executable that this SystemBlastTool
	 * will call to run its blast job.
	 */
	SystemBlastTool (BlastServiceJob *service_job_p, const char *name_s, const char *factory_s, const BlastServiceData *data_p, const char *blast_program_name_s);


	/**
	 * Create a SystemBlastTool for a given ServiceJob using the configuration details from
	 * a serialised JSON fragment.
	 *
	 * @param job_p The ServiceJob to associate with this SystemBlastTool.
	 * @param data_p The BlastServiceData for the Service that will run this SystemBlastTool.
	 * @param json_p The JSON fragment to fill in the serialised values such as job name, etc.
	 */
	SystemBlastTool (BlastServiceJob *job_p, const BlastServiceData *data_p, const json_t *json_p);


	/**
	 * The SystemBlastTool destructor.
	 */
	virtual ~SystemBlastTool ();

	/**
	 * Parse a ParameterSet to configure a BlastTool prior
	 * to it being ran.
	 *
	 * @param params_p The ParameterSet to parse.
	 * @param app_params_p The BlastAppParameters to use process the
	 * values from the given ParameterSet.
	 * @return <code>true</code> if the BlastTool was configured
	 * successfully and is ready to be ran, <code>false</code>
	 * otherwise.
	 * @see BlastTool::ParseParameters
	 */
	virtual bool ParseParameters (ParameterSet *params_p, BlastAppParameters *app_params_p);


	/**
	 * Run this BlastTool
	 *
	 * @return The OperationStatus of this BlastTool after
	 * it has started running.
	 */
	virtual OperationStatus Run ();


	/**
	 * Get the status of a BlastTool
	 *
	 * @param update_flag if this is <code>true</code> then the BlastTool
	 * will check the status of its running jobs if necessary, if this is
	 * <code>false</code> it will return the last cached value.
	 * @return The OperationStatus of this BlastTool.
	 */
	virtual OperationStatus GetStatus (bool update_flag = true);

protected:

	ByteBufferArgsProcessor *sbt_args_processor_p;

	/**
	 * Get the ArgsProcessor that this BlastTool will use
	 * to parse the input ParameterSet prior to running its
	 * job.
	 *
	 * @return The ArgsProcessor for this BlastTool or
	 * <code>0</code> upon error.
	 */
	virtual ArgsProcessor *GetArgsProcessor ();

	/**
	 * Initialise the SystemBlastTool prior to it being run.
	 *
	 * @param prog_s The name of the BLAST program that this SystemBlastTool will run.
	 * @return <code>true</code> if the SystemBlastTool was initialised successfully,
	 * <code>false</code> otherwise.
	 */
	bool Init (const char *prog_s);

};




#endif /* SYSTEM_BLAST_TOOL_HPP_ */

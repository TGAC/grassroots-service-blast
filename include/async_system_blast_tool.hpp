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
 * async_system_blast_tool.hpp
 *
 *  Created on: 9 Mar 2017
 *      Author: billy
 *
 * @file
 * @brief
 */

#ifndef SERVER_SRC_SERVICES_BLAST_INCLUDE_ASYNC_SYSTEM_BLAST_TOOL_HPP_
#define SERVER_SRC_SERVICES_BLAST_INCLUDE_ASYNC_SYSTEM_BLAST_TOOL_HPP_


#include "../../../core/server/task/include/count_async_task.h"
#include "system_blast_tool.hpp"


/**
 * A class that will run Blast as an asynchronous system process.
 *
 * @ingroup blast_service
 */
class BLAST_SERVICE_LOCAL AsyncSystemBlastTool : public SystemBlastTool
{
public:
	/**
	 * Create a AsyncSystemBlastTool for a given ServiceJob.
	 *
	 * @param service_job_p The ServiceJob to associate with this AsyncSystemBlastTool.
	 * @param name_s The name to give to this AsyncSystemBlastTool.
	 * @param factory_s The name of the DrmaaBlastFactory that is creating this AsyncSystemBlastTool.
	 * @param data_p The BlastServiceData for the Service that will run this AsyncSystemBlastTool.
	 * @param blast_program_name_s The name of blast command line executable that this AsyncSystemBlastTool
	 * will call to run its blast job.
	 */
	AsyncSystemBlastTool (BlastServiceJob *service_job_p, const char *name_s, const char *factory_s, const BlastServiceData *data_p, const char *blast_program_name_s);



	/**
	 * Create a AsyncSystemBlastTool for a given ServiceJob using the configuration details from
	 * a serialised JSON fragment.
	 *
	 * @param job_p The ServiceJob to associate with this AsyncSystemBlastTool.
	 * @param data_p The BlastServiceData for the Service that will run this AsyncSystemBlastTool.
	 * @param json_p The JSON fragment to fill in the serialised values such as job name, etc.
	 */
	AsyncSystemBlastTool (BlastServiceJob *job_p, const BlastServiceData *data_p, const json_t *json_p);


	virtual ~AsyncSystemBlastTool ();

	/**
	 * This method is used to serialise this AsyncSystemBlastTool so that
	 * it can be recreated from another calling process when required.
	 * This is used to store the AsyncSystemBlastTool in the JobsManager.
	 *
	 * @param root_p The json object to add the serialisation of this AsyncSystemBlastTool to.
	 * @return <code>true</code> if the BlastTool was serialised
	 * successfully, <code>false</code>
	 * otherwise.
	 */
	virtual bool AddToJSON (json_t *root_p);

	/**
	 * Run this AsyncSystemBlastTool
	 *
	 * @return The OperationStatus of this AsyncSystemBlastTool after
	 * it has started running.
	 */
	virtual OperationStatus Run ();


	/**
	 * Get the status of an AsyncSystemBlastTool
	 *
	 * @param update_flag if this is <code>true</code> then the AsyncSystemBlastTool
	 * will check the status of its running jobs if necessary, if this is
	 * <code>false</code> it will return the last cached value.
	 * @return The OperationStatus of this AsyncSystemBlastTool.
	 */
	virtual OperationStatus GetStatus (bool update_flag = true);


	/**
	 * Get the results after the AsyncSystemBlastTool has finished
	 * running.
	 *
	 * @param formatter_p The BlastFormatter to convert the results
	 * into a different format. If this is 0, then the results will be
	 * returned without any conversion.
	 * @return The results as a c-style string or 0 upon error.
	 */
	virtual char *GetResults (BlastFormatter *formatter_p);


private:
	static const char * const ASBT_ASYNC_S;
	static const char * const ASBT_LOGFILE_S;

	char *asbt_async_logfile_s;

	CountAsyncTaskProducer *asbt_async_producer_p;
};


#endif /* SERVER_SRC_SERVICES_BLAST_INCLUDE_ASYNC_SYSTEM_BLAST_TOOL_HPP_ */

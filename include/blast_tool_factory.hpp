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
 * blast_tool_factory.h
 *
 *  Created on: 24 Feb 2016
 *      Author: tyrrells
 */

#ifndef SERVICES_BLAST_INCLUDE_BLAST_TOOL_FACTORY_HPP_
#define SERVICES_BLAST_INCLUDE_BLAST_TOOL_FACTORY_HPP_

#include "blast_service_api.h"
#include "blast_service.h"

#include "blast_tool.hpp"
#include "blast_service_job.h"

#include "jansson.h"


/**
 * The base class for generating blast tools
 *
 * @ingroup blast_service
 */
class BLAST_SERVICE_LOCAL BlastToolFactory
{
public:

	BlastToolFactory (const json_t *service_config_p);

	/**
	 * Get the BlastToolFactory to use.
	 *
	 * @param service_config_p The configuration for the Blast service
	 * from the global server configuration.
	 * @return The new BlastToolFactory or 0 upon error.
	 */
	static BlastToolFactory *GetBlastToolFactory (const json_t *service_config_p);

	/**
	 * The BlastToolFactory destructor.
	 */
	virtual ~BlastToolFactory ();


	/**
	 * Get an identifying name for this BlastToolFactory.
	 *
	 * @return The name for this type of BlastToolFactory.
	 */
	virtual const char *GetName () = 0;


	/**
	 * Create a BlastTool.
	 *
	 * @param job_p The ServiceJob to associate with the newly generated BlastTool.
	 * @param name_s The name to give to the new BlastTool.
	 * @param data_p The BlastServiceData for the Service that will use this BlastTool.
	 * @return The new BlastTool or 0 upon error.
	 */
	virtual BlastTool *CreateBlastTool (BlastServiceJob *job_p, const char *name_s, const BlastServiceData *data_p) = 0;


	/**
	 * Create a BlastTool from serialised JSON.
	 *
	 * @param json_p The ServiceJob to associate with the newly generated BlastTool.
	 * @param blast_job_p The BlastServiceJob to be associated with the generated BlastTool.
	 * @param service_data_p The BlastServiceData for the Service that will use this BlastTool.
	 * @return The new BlastTool or 0 upon error.
	 */
	virtual BlastTool *CreateBlastTool (const json_t *json_p,  BlastServiceJob *blast_job_p, BlastServiceData *service_data_p) = 0;


	/**
	 * Are the BlastTools that this BlastToolFactory
	 * create able to run asynchronously?
	 *
	 * @return <code>true</code> if the BlastTools are able
	 * to run asynchronously, <code>false</code> otherwise.
	 */
	virtual bool AreToolsAsynchronous () const = 0;

protected:
	const json_t *btf_service_config_p;

};


#ifdef __cplusplus
extern "C"
{
#endif


/**
 * Get a newly created BlastTool
 *
 * @param factory_p The BlastToolFactory to use to create the BlastTool.
 * @param job_p The ServiceJob to associate with the newly generated BlastTool.
 * @param name_s The name to give to the new BlastTool.
 * @param data_p The BlastServiceData for the Service that will use this BlastTool.
 * @return The BlastTool or <code>NULL</code> upon error.
 */
BLAST_SERVICE_LOCAL BlastTool *CreateBlastToolFromFactory (BlastToolFactory *factory_p, BlastServiceJob *job_p, const char *name_s, const BlastServiceData *data_p);


/**
 * Free a BlastToolFactory
 *
 * @param factory_p The BlastToolFactory to deallocate.
 */
BLAST_SERVICE_LOCAL void FreeBlastToolFactory (BlastToolFactory *factory_p);


/**
 * Are the BlastTools that this BlastToolFactory
 * create able to run asynchronously?
 *
 * @param factory_p The BlastToolFactory to check.
 * @return <code>true</code> if the BlastTools are able
 * to run asynchronously, <code>false</code> otherwise.
 * @see BlastToolFactory::AreToolsAsynchronous
 */
BLAST_SERVICE_LOCAL bool IsBlastToolFactoryAsynchronous (BlastToolFactory *factory_p);


#ifdef __cplusplus
}
#endif





#endif /* SERVICES_BLAST_INCLUDE_BLAST_TOOL_FACTORY_HPP_ */

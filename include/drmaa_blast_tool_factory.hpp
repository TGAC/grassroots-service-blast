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
 * drmaa_blast_tool_factory.hpp
 *
 *  Created on: 24 Feb 2016
 *      Author: tyrrells
 */

#ifndef SERVICES_BLAST_INCLUDE_DRMAA_BLAST_TOOL_FACTORY_HPP_
#define SERVICES_BLAST_INCLUDE_DRMAA_BLAST_TOOL_FACTORY_HPP_


#include "external_blast_tool_factory.hpp"

/**
 * The base class for generating DrmaaBlastTools.
 *
 * @see DrmaaBlastTool
 * @ingroup blast_service
 */
class BLAST_SERVICE_LOCAL DrmaaBlastToolFactory : public ExternalBlastToolFactory
{
public:
	/**
	 * A thin wrapper around the constructor for DrmaaBlastToolFactory to catch any
	 * exceptions and return 0 instead.
	 *
	 * @param service_config_p The Blast Service configuration from the global
	 * server configuration.
	 * @return The new DrmaaBlastToolFactory or 0 upon error.
	 * @see  DrmaaBlastToolFactory::DrmaaBlastToolFactory
	 */
	static DrmaaBlastToolFactory	*CreateDrmaaBlastToolFactory (const json_t *service_config_p);

	/**
	 * The DrmaaBlastToolFactory destructor.
	 */
	virtual ~DrmaaBlastToolFactory ();


	/**
	 * Get an identifying name for this BlastToolFactory.
	 *
	 * @return The name for this type of BlastToolFactory.
	 */
	virtual const char *GetName ();


	/**
	 * Create a BlastTool.
	 *
	 * @param job_p The ServiceJob to associate with the newly generated BlastTool.
	 * @param name_s The name to give to the new BlastTool.
	 * @param data_p The BlastServiceData for the Service that will use this BlastTool.
	 * @return The new BlastTool or 0 upon error.
	 */
	virtual BlastTool *CreateBlastTool (BlastServiceJob *job_p, const char *name_s, const BlastServiceData *data_p);


	/**
	 * Create a BlastTool from serialised JSON.
	 *
	 * @param json_p The ServiceJob to associate with the newly generated BlastTool.
	 * @param blast_job_p The BlastServiceJob to be associated with the generated BlastTool.
	 * @param service_data_p The BlastServiceData for the Service that will use this BlastTool.
	 * @return The new BlastTool or 0 upon error.
	 */
	virtual BlastTool *CreateBlastTool (const json_t *json_p,  BlastServiceJob *blast_job_p, BlastServiceData *service_data_p);


	/**
	 * Are the BlastTools that this BlastToolFactory
	 * create able to run asynchronously?
	 *
	 * @return <code>true</code> if the BlastTools are able
	 * to run asynchronously, <code>false</code> otherwise.
	 */
	virtual Synchronicity GetToolsSynchronicity () const;

protected:

	/**
	 * The constructor for DrmaaBlastToolFactory.
	 *
	 * @param service_config_p The Blast Service configuration from the global
	 * server configuration.
	 */
	DrmaaBlastToolFactory (const json_t *service_config_p);
};


#endif /* SERVICES_BLAST_INCLUDE_DRMAA_BLAST_TOOL_FACTORY_HPP_ */

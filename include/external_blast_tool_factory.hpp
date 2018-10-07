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
 * external_blast_tool_factory.hpp
 *
 *  Created on: 25 Feb 2016
 *      Author: tyrrells
 */

#ifndef SERVICES_BLAST_SRC_EXTERNAL_BLAST_TOOL_FACTORY_HPP_
#define SERVICES_BLAST_SRC_EXTERNAL_BLAST_TOOL_FACTORY_HPP_



#include "../../blast-service/include/blast_tool_factory.hpp"

/**
 * The base class for running blast tools as a separate process
 *
 * @ingroup blast_service
 */
class BLAST_SERVICE_LOCAL ExternalBlastToolFactory : public BlastToolFactory
{
public:
	/**
	 * The ExternalBlastToolFactory destructor.
	 */
	virtual ~ExternalBlastToolFactory ();

protected:
	/**
	 * The command line executable to use to run blast jobs.
	 */
	const char *ebtf_program_name_s;

	/**
	 * The constructor for SystemBlastToolFactory.
	 *
	 * @param config_p The Blast Service configuration from the global
	 * server configuration.
	 */
	ExternalBlastToolFactory (const json_t *config_p);
};




#endif /* SERVICES_BLAST_SRC_EXTERNAL_BLAST_TOOL_FACTORY_HPP_ */

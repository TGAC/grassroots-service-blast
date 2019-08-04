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
 * blastx_service.h
 *
 *  Created on: 28 Oct 2016
 *      Author: billy
 */

#ifndef SERVER_SRC_SERVICES_BLAST_INCLUDE_BLASTX_SERVICE_H_
#define SERVER_SRC_SERVICES_BLAST_INCLUDE_BLASTX_SERVICE_H_


#include "blast_service_api.h"
#include "service.h"


#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Get a Service capable of running BlastX searches.
 *
 * @return The BlastX Service or <code>NULL</code> upon error.
 * @ingroup blast_service
 */

BLAST_SERVICE_LOCAL Service *GetBlastXService (GrassrootsServer *grassroots_p);


#ifdef __cplusplus
}
#endif


#endif /* SERVER_SRC_SERVICES_BLAST_INCLUDE_BLASTX_SERVICE_H_ */

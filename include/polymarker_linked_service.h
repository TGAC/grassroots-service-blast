/*
** Copyright 2014-2018 The Earlham Institute
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
 * polymarker_linked_service.h
 *
 *  Created on: 29 Jan 2019
 *      Author: billy
 */

#ifndef SERVICES_BLAST_SERVICE_INCLUDE_POLYMARKER_LINKED_SERVICE_H_
#define SERVICES_BLAST_SERVICE_INCLUDE_POLYMARKER_LINKED_SERVICE_H_


#include "blast_service.h"
#include "typedefs.h"
#include "linked_service.h"
#include "service_job.h"



#ifdef __cplusplus
extern "C"
{
#endif

BLAST_SERVICE_API bool PolymarkerServiceGenerator (LinkedService *linked_service_p, json_t *data_p, struct ServiceJob *job_p);

#ifdef __cplusplus
}
#endif

#endif /* SERVICES_BLAST_SERVICE_INCLUDE_POLYMARKER_LINKED_SERVICE_H_ */

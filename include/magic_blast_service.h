/*
 * magic_blast_service.h
 *
 *  Created on: 5 Jun 2018
 *      Author: billy
 */

#ifndef SERVICES_BLAST_SERVICE_INCLUDE_MAGIC_BLAST_SERVICE_H_
#define SERVICES_BLAST_SERVICE_INCLUDE_MAGIC_BLAST_SERVICE_H_

#include "blast_service_api.h"
#include "service.h"


#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Get a Service capable of running Magic-Blast searches.
 *
 * @return The Magic-Blast Service or <code>NULL</code> upon error.
 * @ingroup blast_service
 */

BLAST_SERVICE_LOCAL Service *GetMagicBlastService (void);


#ifdef __cplusplus
}
#endif



#endif /* SERVICES_BLAST_SERVICE_INCLUDE_MAGIC_BLAST_SERVICE_H_ */

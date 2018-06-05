/*
 * magic_blast_service.c
 *
 *  Created on: 5 Jun 2018
 *      Author: billy
 */

#include "magic_blast_service.h"


static const char *GetMagicBlastServiceName (Service *service_p);

static const char *GetMagicBlastServiceDescription (Service *service_p);





Service *GetMagicBlastService (void)
{
	Service *service_p = NULL;

	return service_p;
}




static const char *GetMagicBlastServiceName (Service * UNUSED_PARAM (service_p))
{
 	return "Magic-Blast service";
}


static const char *GetMagicBlastServiceDescription (Service * UNUSED_PARAM (service_p))
{
	return "A service for mapping large next-generation RNA or DNA sequencing runs against a whole genome or transcriptome";
}


static ParameterSet *GetMagicBlastServiceParameters (Service *service_p, Resource * UNUSED_PARAM (resource_p), UserDetails * UNUSED_PARAM (user_p))
{
	ParameterSet *param_set_p = CreateProteinBlastServiceParameters (service_p, "Protein BlastX service parameters", "A service to search protein databases with nucleotide queries", AddBlastXQuerySequenceParameters, s_tasks_p, S_NUM_TASKS);

	return param_set_p;
}

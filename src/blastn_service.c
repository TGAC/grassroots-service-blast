/*
 * nucleotide_blast_service.c
 *
 *  Created on: 13 Oct 2016
 *      Author: billy
 */

#include <string.h>

#include "blastn_service.h"

#include "args_processor.hpp"
#include "blast_service.h"
#include "blast_service_params.h"
#include "blast_util.h"

#include "unsigned_int_parameter.h"


/*******************************/
/***** STATIC DECLARATIONS *****/
/*******************************/


static NamedParameterType S_MATCH_SCORE = { "reward", PT_UNSIGNED_INT };
static NamedParameterType S_MISMATCH_SCORE = { "penalty", PT_NEGATIVE_INT };



static const size_t S_NUM_TASKS = 5;

static const BlastTask s_tasks_p [S_NUM_TASKS] =
{
  { "megablast", "megablast: Traditional megablast used to find very similar (e.g., intraspecies or closely related species) sequences" },
  { "dc-megablast", "dc-megablast: Discontiguous megablast used to find more distant (e.g., interspecies) sequences" },
  { "blastn", "blastn: Traditional BLASTN requiring an exact match of 11" },
  { "blastn-short", "blastn-short: BLASTN program optimized for sequences shorter than 50 bases" },
  { "rmblastn", "rmblastn: BLASTN with complexity adjusted scoring and masklevel" },
};


static const char *GetBlastNServiceName (const Service *service_p);

static const char *GetBlastNServiceDescription (const Service *service_p);

static ParameterSet *GetBlastNServiceParameters (Service *service_p, Resource *resource_p, UserDetails *user_p);

static bool GetBlastNServiceParameterTypeForNamedParameter (const Service *service_p, const char *param_name_s, ParameterType *pt_p);

static ServiceJobSet *RunNucleotideBlastService (Service *service_p, ParameterSet *param_set_p, UserDetails *user_p, ProvidersStateTable *providers_p);

static ServiceMetadata *GetBlastNServiceMetadata (Service *service_p);


static bool AddNucleotideBlastParameters (BlastServiceData *data_p, ParameterSet *param_set_p);

static bool AddScoringParams (BlastServiceData *data_p, ParameterSet *param_set_p);

static bool ParseNucleotideBlastParameters (const BlastServiceData *data_p, ParameterSet *params_p, ArgsProcessor *ap_p);

static bool GetNucleotideBlastParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p);




/*******************************/
/******* API DEFINITIONS *******/
/*******************************/


Service *GetBlastNService (GrassrootsServer *grassroots_p)
{
	Service *nucleotide_blast_service_p = (Service *) AllocMemory (sizeof (Service));

	if (nucleotide_blast_service_p)
		{
			BlastServiceData *data_p = AllocateBlastServiceData (nucleotide_blast_service_p, DT_NUCLEOTIDE);

			if (data_p)
				{
					if (InitialiseService (nucleotide_blast_service_p,
														 GetBlastNServiceName,
														 GetBlastNServiceDescription,
														 NULL,
														 RunNucleotideBlastService,
														 IsResourceForBlastService,
														 GetBlastNServiceParameters,
														 GetBlastNServiceParameterTypeForNamedParameter,
														 ReleaseBlastServiceParameters,
														 CloseBlastService,
														 CustomiseBlastServiceJob,
														 true,
														 SY_SYNCHRONOUS,
														 (ServiceData *) data_p,
														 GetBlastNServiceMetadata,
														 GetBlastIndexingData,
														 grassroots_p))
						{
							if (GetBlastServiceConfig (data_p))
								{
									return nucleotide_blast_service_p;
								}
						}
				}

			FreeService (nucleotide_blast_service_p);
		}

	return NULL;
}



static const char *GetBlastNServiceName (const Service * UNUSED_PARAM (service_p))
{
	return "BlastN";
}


static const char *GetBlastNServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "Search nucleotide databases with nucleotide queries";
}


static ServiceMetadata *GetBlastNServiceMetadata (Service *service_p)
{
	ServiceMetadata *metadata_p = GetGeneralBlastServiceMetadata (service_p);

	if (metadata_p)
		{
			const char *term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S  "data_2977";
			SchemaTerm *input_p = AllocateSchemaTerm (term_url_s, "Nucleic acid sequence", "One or more nucleic acid sequences, possibly with associated annotation.");

			if (input_p)
				{
					if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p))
						{
							return metadata_p;
						}		/* if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p)) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add input term %s to service metadata", term_url_s);
							FreeSchemaTerm (input_p);
						}

				}		/* if (input_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate input term %s for service metadata", term_url_s);
				}

			FreeServiceMetadata (metadata_p);
		}		/* if (metadata_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate base service metadata");
		}

	return NULL;
}


static ParameterSet *GetBlastNServiceParameters (Service *service_p, Resource * UNUSED_PARAM (resource_p), UserDetails * UNUSED_PARAM (user_p))
{
	ParameterSet *param_set_p = AllocateParameterSet ("Nucleotide Blast service parameters", "A service to run nucleotide Blast searches");

	if (param_set_p)
		{
			if (AddBaseBlastServiceParameters (service_p, param_set_p, DT_NUCLEOTIDE, NULL, NULL))
				{
				  BlastServiceData *blast_data_p = (BlastServiceData *) (service_p -> se_data_p);
				  uint32 word_size = 11;

					if (AddGeneralAlgorithmParams (blast_data_p, param_set_p, AddProteinGeneralAlgorithmParameters, &word_size))
						{
							if (AddProgramSelectionParameters (blast_data_p, param_set_p, s_tasks_p, S_NUM_TASKS))
								{
									if (AddNucleotideBlastParameters (blast_data_p, param_set_p))
										{
											return param_set_p;
										}
								}
						}
				}		/* if (AddBaseBlastServiceParameters (service_p, param_set_p, DT_NUCLEOTIDE)) */

			/*
			 * task: 'blastn' 'blastn-short' 'dc-megablast' 'megablast' 'rmblastn
			 */

			FreeParameterSet (param_set_p);
		}		/* if (param_set_p) */


	return NULL;
}



static bool GetBlastNServiceParameterTypeForNamedParameter (const Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (!GetBaseBlastServiceParameterTypeForNamedParameter (service_p, param_name_s, pt_p))
		{
			if (!GetGeneralAlgorithmParameterTypeForNamedParameter (param_name_s, pt_p))
				{
					if (!GetProteinGeneralAlgorithmParameterTypeForNamedParameter (param_name_s, pt_p))
						{
							if (!GetProgramSelectionParameterTypeForNamedParameter (param_name_s, pt_p))
								{
									if (!GetNucleotideBlastParameterTypeForNamedParameter (param_name_s, pt_p))
										{
											success_flag = false;
										}		/* if (!GetNucleotideBlastParameterTypeForNamedParameter (param_name_s, pt_p)) */

								}		/* if (!GetProgramSelectionParameterTypeForNamedParameter (param_name_s, pt_p)) */
						}

				}		/* if (!GetGeneralAlgorithmParameterTypeForNamedParameter (param_name_s, pt_p)) */

		}		/* if (!GetBaseBlastServiceParameterTypeForNamedParameter (param_name_s, pt_p)) */

	return success_flag;
}




static bool AddNucleotideBlastParameters (BlastServiceData *data_p, ParameterSet *param_set_p)
{
	bool success_flag = false;

	if (AddScoringParams (data_p, param_set_p))
		{
			success_flag = true;
		}

	return success_flag;
}


static bool AddScoringParams (BlastServiceData *data_p, ParameterSet *param_set_p)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	ServiceData *service_data_p = & (data_p -> bsd_base_data);
	uint32 def_score;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Scoring Parameters", false, & (data_p -> bsd_base_data), param_set_p);

	def_score = 2;

	if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (service_data_p, param_set_p, group_p, S_MATCH_SCORE.npt_name_s, "Reward", "The reward for matching bases", &def_score, PL_ADVANCED)) != NULL)
		{
			def_score = 3;

			if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (service_data_p, param_set_p, group_p, S_MISMATCH_SCORE.npt_name_s, "Penalty", "The penalty for mismatching bases", &def_score, PL_ADVANCED)) != NULL)
				{
					success_flag = true;
				}
		}


	return success_flag;
}


static bool GetNucleotideBlastParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (strcmp (param_name_s, S_MATCH_SCORE.npt_name_s) == 0)
		{
			*pt_p = S_MATCH_SCORE.npt_type;
		}
	else if (strcmp (param_name_s, S_MISMATCH_SCORE.npt_name_s) == 0)
		{
			*pt_p = S_MISMATCH_SCORE.npt_type;
		}
	else
		{
			success_flag = false;
		}

	return success_flag;
}




static bool ParseNucleotideBlastParameters (const BlastServiceData * UNUSED_PARAM (data_p), ParameterSet *params_p, ArgsProcessor *ap_p)
{
	bool success_flag = false;

	/* Word size */
	if (GetAndAddBlastArgs (params_p, BS_WORD_SIZE.npt_name_s, false, ap_p))
		{
			/* Reward */
			if (GetAndAddBlastArgs (params_p, S_MATCH_SCORE.npt_name_s, false, ap_p))
				{
					/* Penalty */
					if (GetAndAddBlastArgs (params_p, S_MISMATCH_SCORE.npt_name_s, false, ap_p))
						{
							success_flag = true;
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add \"%s\"", S_MISMATCH_SCORE.npt_name_s);
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add \"%s\"", S_MATCH_SCORE.npt_name_s);
				}

		}		/* if (GetAndAddBlastArgsToByteBuffer (params_p, BS_WORD_SIZE.npt_name_s, false, buffer_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add \"%s\"", BS_WORD_SIZE.npt_name_s);
		}

	return success_flag;
}



static ServiceJobSet *RunNucleotideBlastService (Service *service_p, ParameterSet *param_set_p, UserDetails *user_p, ProvidersStateTable *providers_p)
{
	BlastAppParameters app_params;
	ServiceJobSet *jobs_p = NULL;

	app_params.bap_parse_params_fn = ParseNucleotideBlastParameters;

	jobs_p = RunBlastService (service_p, param_set_p, user_p, providers_p, &app_params);

	return jobs_p;
}



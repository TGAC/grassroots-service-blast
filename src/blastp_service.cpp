/*
 * protein_blast_service.c
 *
 *  Created on: 13 Oct 2016
 *      Author: billy
 */

#include "blastp_service.h"

#include <string.h>

#include "blast_app_parameters.h"
#include "blast_service.h"
#include "blast_service_params.h"
#include "blast_util.h"


#include "unsigned_int_parameter.h"
#include "string_parameter.h"


/*******************************/
/***** STATIC DECLARATIONS *****/
/*******************************/



static NamedParameterType S_MATRIX = { "matrix", PT_STRING };
static NamedParameterType S_COMP_BASED_STATS = { "comp_based_stats", PT_UNSIGNED_INT };


static const size_t S_NUM_TASKS = 3;
static const BlastTask s_tasks_p [S_NUM_TASKS] =
{
  { "blastp", "blastp: Traditional BLASTP to compare a protein query to a protein database" },
  { "blastp-short", "blastp-short: BLASTP optimized for queries shorter than 30 residues" },
  { "blastp-fast", "blastp-fast: BLASTP optimized for faster runtime" }
};



static const uint32 S_NUM_MATRICES = 8;

static const char *S_MATRICES_SS [S_NUM_MATRICES] =
{
	"PAM30",
	"PAM70",
	"PAM250",
	"BLOSUM80",
	"BLOSUM62",
	"BLOSUM45",
	"BLOSUM50",
	"BLOSUM90"
};



static const uint32 S_NUM_COMP_BASED_STATS = 4;


static const char *GetProteinBlastServiceName (const Service *service_p);

static const char *GetProteinBlastServiceDescription (const Service *service_p);

static const char *GetProteinBlastServiceAlias (const Service *service_p);

static ServiceJobSet *RunProteinBlastService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);

static ParameterSet *GetProteinBlastServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);


static bool AddScoringParameters (BlastServiceData *data_p, ParameterSet *param_set_p);

static bool AddMatrixParameter (BlastServiceData *data_p, ParameterSet *param_set_p, ParameterGroup *group_p);

static bool AddCompositionalAdjustmentsParameter (BlastServiceData *data_p, ParameterSet *param_set_p, ParameterGroup *group_p);

static ServiceMetadata *GetBlastPServiceMetadata (Service *service_p);

static bool GetBlastPServiceParameterTypeForNamedParameter (const Service *service_p, const char *param_name_s, ParameterType *pt_p);



/*******************************/
/******* API DEFINITIONS *******/
/*******************************/


Service *GetBlastPService (GrassrootsServer *grassroots_p)
{
	Service *protein_blast_service_p = (Service *) AllocMemory (sizeof (Service));

	if (protein_blast_service_p)
		{
			BlastServiceData *data_p = AllocateBlastServiceData (protein_blast_service_p, DT_PROTEIN);

			if (data_p)
				{
					if (InitialiseService (protein_blast_service_p,
														 GetProteinBlastServiceName,
														 GetProteinBlastServiceDescription,
														 GetProteinBlastServiceAlias,
														 NULL,
														 RunProteinBlastService,
														 IsResourceForBlastService,
														 GetProteinBlastServiceParameters,
														 GetBlastPServiceParameterTypeForNamedParameter,
														 ReleaseBlastServiceParameters,
														 CloseBlastService,
														 CustomiseBlastServiceJob,
														 true,
														 SY_SYNCHRONOUS,
														 (ServiceData *) data_p,
														 GetBlastPServiceMetadata,
														 GetBlastIndexingData,
														 grassroots_p))
						{
							if (GetBlastServiceConfig (data_p))
								{
									return protein_blast_service_p;
								}
						}
				}

			FreeService (protein_blast_service_p);
		}

	return NULL;
}


ParameterSet *CreateProteinBlastServiceParameters (Service *service_p, DataResource *resource_p, const char *param_set_name_s, const char *param_set_description_s, AddAdditionalParamsFn query_sequence_callback_fn, void *callback_data_p, const BlastTask *tasks_p, const uint32 num_tasks)
{
	ParameterSet *param_set_p = AllocateParameterSet (param_set_name_s, param_set_description_s);

	if (param_set_p)
		{
			if (AddBaseBlastServiceParameters (service_p, param_set_p, resource_p, DT_PROTEIN, query_sequence_callback_fn, callback_data_p))
				{
          BlastServiceData *blast_data_p = (BlastServiceData *) (service_p -> se_data_p);
          uint32 word_size = 3;

					if (AddGeneralAlgorithmParams (blast_data_p, param_set_p, AddProteinGeneralAlgorithmParameters, &word_size))
						{
							if (AddProteinBlastParameters (blast_data_p, param_set_p))
								{
								  if (AddProgramSelectionParameters (blast_data_p, param_set_p, tasks_p, tasks_p, sizeof (*tasks_p), num_tasks))
                    {
                      return param_set_p;
                    }
								}
						}
				}		/* if (AddBaseBlastServiceParameters (service_p, param_set_p, DT_PROTEIN)) */

			/*
			 * task: 'blastn' 'blastn-short' 'dc-megablast' 'megablast' 'rmblastn
			 */

		}		/* if (param_set_p) */


	return param_set_p;
}



static bool GetBlastPServiceParameterTypeForNamedParameter (const Service *service_p, const char *param_name_s, ParameterType *pt_p)
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
									if (!GetProteinBlastParameterTypeForNamedParameter (param_name_s, pt_p))
										{
											success_flag = false;
										}		/* if (!GetProteinBlastParameterTypeForNamedParameter (param_name_s, pt_p)) */

								}		/* if (!GetProgramSelectionParameterTypeForNamedParameter (param_name_s, pt_p)) */

						}		/* if (!GetProteinGeneralAlgorithmParameterTypeForNamedParameter (param_name_s, pt_p)) */

				}		/* if (!GetGeneralAlgorithmParameterTypeForNamedParameter (param_name_s, pt_p)) */

		}		/* if (!GetBaseBlastServiceParameterTypeForNamedParameter (param_name_s, pt_p)) */

	return success_flag;
}


bool AddProteinBlastParameters (BlastServiceData *data_p, ParameterSet *param_set_p)
{
	bool success_flag = false;

	if (AddScoringParameters (data_p, param_set_p))
		{
			success_flag = true;
		}

	return success_flag;
}


bool GetProteinBlastParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (strcmp (param_name_s, S_MATRIX.npt_name_s) == 0)
		{
			*pt_p = S_MATRIX.npt_type;
		}
	else if (strcmp (param_name_s, S_COMP_BASED_STATS.npt_name_s) == 0)
		{
			*pt_p = S_COMP_BASED_STATS.npt_type;
		}
	else
		{
			success_flag = false;
		}

	return success_flag;
}


bool ParseBlastPParameters (const BlastServiceData * UNUSED_PARAM (data_p), ParameterSet *params_p, ArgsProcessor *ap_p)
{
	bool success_flag = false;

	/* matrix */
	if (GetAndAddBlastArgs (params_p, S_MATRIX.npt_name_s, false, ap_p))
		{
			/* Word Size */
			if (GetAndAddBlastArgs (params_p, S_COMP_BASED_STATS.npt_name_s, false, ap_p))
				{
					success_flag = true;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add \"%s\"", S_COMP_BASED_STATS.npt_name_s);
				}

		}		/* if (GetAndAddBlastArgs (params_p, S_MATRIX.npt_name_s, false, ap_p)) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add \"%s\"", S_MATRIX.npt_name_s);
		}

	return success_flag;
}


static const char *GetProteinBlastServiceName (const Service * UNUSED_PARAM (service_p))
{
 	return "BlastP";
}


static const char *GetProteinBlastServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "Search protein databases with protein queries";
}


static const char *GetProteinBlastServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return (BS_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "blastp");
}


static ParameterSet *GetProteinBlastServiceParameters (Service *service_p, DataResource *resource_p, User * UNUSED_PARAM (user_p))
{
	return CreateProteinBlastServiceParameters (service_p, resource_p, "Protein Blast service parameters", "A service to run Protein Blast searches", NULL, NULL, s_tasks_p, S_NUM_TASKS);
}


static ServiceJobSet *RunProteinBlastService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p)
{
	BlastAppParameters app_params;
	ServiceJobSet *jobs_p = NULL;

	app_params.bap_parse_params_fn = ParseBlastPParameters;

	jobs_p = RunBlastService (service_p, param_set_p, user_p, providers_p, &app_params);

	return jobs_p;
}





static bool AddScoringParameters (BlastServiceData *data_p, ParameterSet *param_set_p)
{
	bool success_flag = false;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Scoring Parameters", false, & (data_p -> bsd_base_data), param_set_p);

	if (AddMatrixParameter (data_p, param_set_p, group_p))
		{
			if (AddCompositionalAdjustmentsParameter (data_p, param_set_p, group_p))
				{
					success_flag = true;
				}		/* if (AddCompositionalAdjustmentsParameter (data_p, param_set_p, group_p)) */
			else
				{

				}
		}		/* if (AddMatrixParameter (data_p, param_set_p, group_p)) */
	else
		{

		}

	return success_flag;
}


static bool AddMatrixParameter (BlastServiceData *data_p, ParameterSet *param_set_p, ParameterGroup *group_p)
{
	bool success_flag = false;
	Parameter *param_p = NULL;

	/* set BLOSUM62 as default */
	const char *def_s = (* (S_MATRICES_SS + 4));

	if ((param_p = EasyCreateAndAddStringParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, S_MATRIX.npt_type, S_MATRIX.npt_name_s, "Matrix", "The Scoring matrix to use", def_s, PL_ADVANCED)) != NULL)
		{
			uint32 i;

			success_flag = true;

			for (i = 0; i < S_NUM_MATRICES; ++ i)
				{
					if (!CreateAndAddStringParameterOption ((StringParameter *) param_p, * (S_MATRICES_SS + i), * (S_MATRICES_SS + i)))
						{
							i = S_NUM_MATRICES;
							success_flag = false;
						}
				}

			if (!success_flag)
				{
					DetachParameter (param_set_p, param_p);
					FreeParameter (param_p);
				}
		}
	else
		{

		}


	return success_flag;
}



static bool AddCompositionalAdjustmentsParameter (BlastServiceData *data_p, ParameterSet *param_set_p, ParameterGroup *group_p)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	uint32 def_value = 2;
	const char *descriptions_ss [] =
		{
			"No composition-based statistics",
			"Composition-based statistics as in NAR 29:2994-3005, 2001",
			"Composition-based score adjustment as in Bioinformatics 21:902-911, 2005, conditioned on sequence properties",
			"Composition-based score adjustment as in Bioinformatics 21:902-911, 2005, unconditionally"
		};

	if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, S_COMP_BASED_STATS.npt_name_s, "Compositional adjustments", "Matrix adjustment method to compensate for amino acid composition of sequences.", &def_value, PL_ADVANCED)) != NULL)
		{
			uint32 i = 0;
			success_flag = true;

			for (i = 0; i < S_NUM_COMP_BASED_STATS; ++ i)
				{
					if (!CreateAndAddUnsignedIntParameterOption ((UnsignedIntParameter *) param_p, i, * (descriptions_ss + i)))
						{
							i = S_NUM_COMP_BASED_STATS;
							success_flag = false;
						}
				}

			if (!success_flag)
				{
					DetachParameter (param_set_p, param_p);
					FreeParameter (param_p);
				}
		}
	else
		{
		}

	return success_flag;
}



static ServiceMetadata *GetBlastPServiceMetadata (Service *service_p)
{
	ServiceMetadata *metadata_p = GetGeneralBlastServiceMetadata (service_p);

	if (metadata_p)
		{
			const char *term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "data_2976";
			SchemaTerm *input_p = AllocateSchemaTerm (term_url_s, "Protein sequence", "One or more protein sequences, possibly with associated annotation.");

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

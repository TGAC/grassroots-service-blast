/*
 * nucleotide_blast_service.c
 *
 *  Created on: 13 Oct 2016
 *      Author: billy
 */

#include <string.h>

#include "args_processor.hpp"
#include "blastn_service.h"

#include "blast_service.h"
#include "blast_service_params.h"
#include "blast_util.h"

#include "unsigned_int_parameter.h"
#include "signed_int_parameter.h"


/*******************************/
/***** STATIC DECLARATIONS *****/
/*******************************/


static NamedParameterType S_MATCH_SCORE = { "reward", PT_UNSIGNED_INT };
static NamedParameterType S_MISMATCH_SCORE = { "penalty", PT_NEGATIVE_INT };
static NamedParameterType S_GAP_OPEN_SCORE = { "gapopen", PT_UNSIGNED_INT };
static NamedParameterType S_GAP_EXTEND_SCORE = { "gapextend", PT_UNSIGNED_INT };


static const size_t S_NUM_TASKS = 4;



typedef struct BlastNTaskDefaults
{
	uint32 btd_word_size;
	uint32 btd_gapopen;
	uint32 btd_gapextend;
	uint32 btd_reward;
	int32 btd_penalty;
} BlastNTaskDefaults;




static const BlastNTaskDefaults s_megablast_defaults =
{
		/* uint32 btd_word_size; */
		28,

		/* uint32 btd_gapopen; */
		0,

		/* uint32 btd_gapextend; */
		0,

		/* uint32 btd_reward; */
		1,

		/* int32 btd_penalty; */
		-2
};

static const BlastNTaskDefaults s_blastn_defaults =
{
		/* uint32 btd_word_size; */
		11,

		/* uint32 btd_gapopen; */
		5,

		/* uint32 btd_gapextend; */
		2,

		/* uint32 btd_reward; */
		2,

		/* int32 btd_penalty; */
		-3
};


static const BlastNTaskDefaults s_blastn_short_defaults =
{
		/* uint32 btd_word_size; */
		7,

		/* uint32 btd_gapopen; */
		5,

		/* uint32 btd_gapextend; */
		2,

		/* uint32 btd_reward; */
		1,

		/* int32 btd_penalty; */
		-3
};



static const BlastNTaskDefaults s_dc_megablast_defaults =
{
		/* uint32 btd_word_size; */
		11,

		/* uint32 btd_gapopen; */
		5,

		/* uint32 btd_gapextend; */
		2,

		/* uint32 btd_reward; */
		2,

		/* int32 btd_penalty; */
		-3
};


typedef struct BlastNTask
{
	BlastTask bnt_task;

	const BlastNTaskDefaults * const bnt_defaults_p;
} BlastNTask;



static const BlastNTask s_tasks_p [S_NUM_TASKS] =
{
	{ { "megablast", "megablast: Traditional megablast used to find very similar (e.g., intraspecies or closely related species) sequences" }, &s_megablast_defaults },
  { { "dc-megablast", "dc-megablast: Discontiguous megablast used to find more distant (e.g., interspecies) sequences" }, &s_dc_megablast_defaults },
  { { "blastn", "blastn: The traditional program used for inter-species comparison" }, &s_blastn_defaults },
  { { "blastn-short", "blastn-short: Optimized for sequences shorter than 30 nucelotides" }, &s_blastn_short_defaults }
};




static const char *GetBlastNServiceName (const Service *service_p);

static const char *GetBlastNServiceDescription (const Service *service_p);

static const char *GetBlastNServiceAlias (const Service *service_p);

static ParameterSet *GetBlastNServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);

static bool GetBlastNServiceParameterTypeForNamedParameter (const Service *service_p, const char *param_name_s, ParameterType *pt_p);

static ServiceJobSet *RunNucleotideBlastService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);

static ServiceMetadata *GetBlastNServiceMetadata (Service *service_p);

static bool AddNucleotideBlastParameters (BlastServiceData *data_p, ParameterSet *param_set_p, const BlastNTaskDefaults *defaults_p);

static bool AddScoringParams (BlastServiceData *data_p, ParameterSet *param_set_p, const BlastNTaskDefaults *defaults_p);

static bool ParseNucleotideBlastParameters (const BlastServiceData *data_p, ParameterSet *params_p, ArgsProcessor *ap_p);

static bool GetNucleotideBlastParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p);


static const BlastNTask *GetBlastNTaskFromResource (DataResource *resource_p, const NamedParameterType task_param_type);


static const BlastNTask *GetDefaultBlastNTask (void);



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
														 GetBlastNServiceAlias,
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


static const char *GetBlastNServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return (BS_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "blastn");
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


static ParameterSet *GetBlastNServiceParameters (Service *service_p, DataResource *resource_p, User * UNUSED_PARAM (user_p))
{
	ParameterSet *param_set_p = AllocateParameterSet ("Nucleotide Blast service parameters", "A service to run nucleotide Blast searches");

	if (param_set_p)
		{
			if (AddBaseBlastServiceParameters (service_p, param_set_p, resource_p, DT_NUCLEOTIDE, NULL, NULL))
				{
				  BlastServiceData *blast_data_p = (BlastServiceData *) (service_p -> se_data_p);
				  const BlastNTask *default_task_p = GetBlastNTaskFromResource (resource_p, BS_TASK);

				  if (!default_task_p)
				  	{
				  		default_task_p = GetDefaultBlastNTask ();
				  	}

					if (AddGeneralAlgorithmParams (blast_data_p, param_set_p, AddProteinGeneralAlgorithmParameters, & (default_task_p -> bnt_defaults_p -> btd_word_size)))
						{
							if (AddProgramSelectionParameters (blast_data_p, param_set_p, (const BlastTask *) s_tasks_p, & (default_task_p -> bnt_task), sizeof (BlastNTask), S_NUM_TASKS))
								{
									if (AddNucleotideBlastParameters (blast_data_p, param_set_p, default_task_p -> bnt_defaults_p))
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




static bool AddNucleotideBlastParameters (BlastServiceData *data_p, ParameterSet *param_set_p, const BlastNTaskDefaults *defaults_p)
{
	bool success_flag = false;

	if (AddScoringParams (data_p, param_set_p, defaults_p))
		{
			success_flag = true;
		}

	return success_flag;
}


static bool AddScoringParams (BlastServiceData *data_p, ParameterSet *param_set_p, const BlastNTaskDefaults *defaults_p)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	ServiceData *service_data_p = & (data_p -> bsd_base_data);
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Scoring Parameters", false, & (data_p -> bsd_base_data), param_set_p);

	if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (service_data_p, param_set_p, group_p, S_MATCH_SCORE.npt_name_s, "Reward", "The reward for matching bases", & (defaults_p -> btd_reward), PL_ADVANCED)) != NULL)
		{
			if ((param_p = EasyCreateAndAddSignedIntParameterToParameterSet (service_data_p, param_set_p, group_p, S_MISMATCH_SCORE.npt_type, S_MISMATCH_SCORE.npt_name_s, "Penalty", "The penalty for mismatching bases", & (defaults_p -> btd_penalty), PL_ADVANCED)) != NULL)
				{
					if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (service_data_p, param_set_p, group_p, S_GAP_OPEN_SCORE.npt_name_s, "Gap Open", "The cost to open a gap", & (defaults_p -> btd_gapopen), PL_ADVANCED)) != NULL)
						{
							if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (service_data_p, param_set_p, group_p, S_GAP_EXTEND_SCORE.npt_name_s, "Gap Extend", "The cost to extend a gap", & (defaults_p -> btd_gapextend), PL_ADVANCED)) != NULL)
								{
									success_flag = true;
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add parameter \"%s\"", S_GAP_EXTEND_SCORE.npt_name_s);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add parameter \"%s\"", S_GAP_OPEN_SCORE.npt_name_s);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add parameter \"%s\"", S_MISMATCH_SCORE.npt_name_s);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add parameter \"%s\"", S_MATCH_SCORE.npt_name_s);
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
	else if (strcmp (param_name_s, S_GAP_EXTEND_SCORE.npt_name_s) == 0)
			{
				*pt_p = S_GAP_EXTEND_SCORE.npt_type;
			}
		else if (strcmp (param_name_s, S_GAP_OPEN_SCORE.npt_name_s) == 0)
			{
				*pt_p = S_GAP_OPEN_SCORE.npt_type;
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
							/* Gap Open */
							if (GetAndAddBlastArgs (params_p, S_GAP_OPEN_SCORE.npt_name_s, false, ap_p))
								{
									/* Gap extend */
									if (GetAndAddBlastArgs (params_p, S_GAP_EXTEND_SCORE.npt_name_s, false, ap_p))
										{
											success_flag = true;
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add \"%s\"", S_GAP_EXTEND_SCORE.npt_name_s);
										}
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add \"%s\"", S_GAP_OPEN_SCORE.npt_name_s);
								}
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



static ServiceJobSet *RunNucleotideBlastService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p)
{
	BlastAppParameters app_params;
	ServiceJobSet *jobs_p = NULL;

	app_params.bap_parse_params_fn = ParseNucleotideBlastParameters;

	jobs_p = RunBlastService (service_p, param_set_p, user_p, providers_p, &app_params);

	return jobs_p;
}



static const BlastNTask *GetDefaultBlastNTask (void)
{
	return s_tasks_p;
}


static const BlastNTask *GetBlastNTaskFromResource (DataResource *resource_p, const NamedParameterType task_param_type)
{
	if (resource_p && (resource_p -> re_data_p))
		{
			const json_t *param_set_json_p = json_object_get (resource_p -> re_data_p, PARAM_SET_KEY_S);

			if (param_set_json_p)
				{
					json_t *params_json_p = json_object_get (param_set_json_p, PARAM_SET_PARAMS_S);

					if (params_json_p)
						{
							const size_t num_entries = json_array_size (params_json_p);
							size_t i;

							for (i = 0; i < num_entries; ++ i)
								{
									const json_t *param_json_p = json_array_get (params_json_p, i);
									const char *name_s = GetJSONString (param_json_p, PARAM_NAME_S);

									if (name_s)
										{
											if (strcmp (name_s, task_param_type.npt_name_s) == 0)
												{
													const char *task_s = GetJSONString (param_json_p, PARAM_CURRENT_VALUE_S);

													if (task_s)
														{
															const BlastNTask *task_p = s_tasks_p;
															size_t j = S_NUM_TASKS;

															while (j > 0)
																{
																	if (strcmp (task_s, task_p -> bnt_task.bt_name_s) == 0)
																		{
																			return task_p;
																		}

																	-- j;
																	++ task_p;
																}

														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, param_json_p, "Failed to get \"%s\" from \"%s\"", PARAM_CURRENT_VALUE_S, task_s);
														}

													/* force exit from loop */
													i = num_entries;
												}
										}		/* if (name_s) */

								}
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, param_set_json_p, "Failed to get params with key \"%s\"", PARAM_SET_PARAMS_S);
						}
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, resource_p -> re_data_p, "Failed to get param set with key \"%s\"", PARAM_SET_KEY_S);
				}

		}		/* if (resource_p && (resource_p -> re_data_p)) */

	return NULL;
}




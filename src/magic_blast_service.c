/*
 * magic_blast_service.c
 *
 *  Created on: 5 Jun 2018
 *      Author: billy
 */

#include <string.h>

#include "magic_blast_service.h"

#include "args_processor.hpp"
#include "blast_service.h"
#include "blast_service_params.h"
#include "blast_util.h"


/*
 * STATIC DECLARATIONS
 */


/* Input query sequences are paired */
static NamedParameterType S_PAIRED = { "paired", PT_BOOLEAN };


static NamedParameterType S_PERCENT_IDENTITY = { "perc_identity", PT_UNSIGNED_REAL };

static NamedParameterType S_SRA = { "sra", PT_KEYWORD };

/**
 * The different available output formats.
 *
 * @ingroup blast_service
 */
typedef enum MagicBlastOutputFormat
{
	/** SAM format */
	MBOF_SAM,

	/** Tabular Format */
	MBOF_TABULAR,

	/** Seqalign (Text ASN.1) */
	MBOF_TEXT_ASN1,

	/** The number of different output formats */
	MBOF_NUM_TYPES
} MagicBlastOutputFormat;


const char *S_OUTPUT_FORMATS_SS [MBOF_NUM_TYPES] =
{
	"sam",
	"tabular",
	"asn"
};



static const char *GetMagicBlastServiceName (Service *service_p);

static const char *GetMagicBlastServiceDescription (Service *service_p);

static ServiceJobSet *RunMagicBlastService (Service *service_p, ParameterSet *param_set_p, UserDetails *user_p, ProvidersStateTable *providers_p);

static ServiceMetadata *GetMagicBlastServiceMetadata (Service *service_p);

static bool ParseMagicBlastParameters (const BlastServiceData *data_p, ParameterSet *params_p, ArgsProcessor *ap_p);

static ParameterSet *GetMagicBlastServiceParameters (Service *service_p, Resource * UNUSED_PARAM (resource_p), UserDetails * UNUSED_PARAM (user_p));

static bool AddFormattingOptionsParameters (ParameterSet *param_set_p, BlastServiceData *data_p);

static bool GetMagicBlastServiceParameterTypeForNamedParameter (Service *service_p, const char *param_name_s, ParameterType *pt_p);

/*
 * API DEFINITIONS
 */

Service *GetMagicBlastService (void)
{
	Service *service_p = NULL;
	Service *magic_blast_service_p = (Service *) AllocMemory (sizeof (Service));

	if (magic_blast_service_p)
		{
			BlastServiceData *data_p = AllocateBlastServiceData (magic_blast_service_p, DT_NUCLEOTIDE);

			if (data_p)
				{
					if (InitialiseService (magic_blast_service_p,
																 GetMagicBlastServiceName,
																 GetMagicBlastServiceDescription,
																 NULL,
																 RunMagicBlastService,
																 IsResourceForBlastService,
																 GetMagicBlastServiceParameters,
																 GetMagicBlastServiceParameterTypeForNamedParameter,
																 ReleaseBlastServiceParameters,
																 CloseBlastService,
																 CustomiseBlastServiceJob,
																 true,
																 SY_SYNCHRONOUS,
																 (ServiceData *) data_p,
																 GetMagicBlastServiceMetadata))
						{
							if (GetBlastServiceConfig (data_p))
								{
									return magic_blast_service_p;
								}
						}
				}

			FreeService (magic_blast_service_p);
		}
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
	ParameterSet *param_set_p = AllocateParameterSet ("Magic Blast service parameters", "A service to run Magic Blast searches");
	BlastServiceData *data_p = (BlastServiceData *) service_p -> se_data_p;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Query Sequence Parameters", false, NULL, & (data_p -> bsd_base_data), param_set_p);
	Parameter *param_p;
	SharedType def;

	InitSharedType (&def);

	if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, S_PAIRED.npt_type, S_PAIRED.npt_name_s, "Paired query seqeunces", "Are the input query sequences paired?", def, PL_ALL)) != NULL)
		{
			if ((param_p = SetUpOutputFormatParameter (S_OUTPUT_FORMATS_SS, MBOF_NUM_TYPES, * (S_OUTPUT_FORMATS_SS + MBOF_SAM), data_p, param_set_p, group_p)) != NULL)
				{
					if (AddFormattingOptionsParameters (param_set_p, data_p))
						{
							return param_set_p;
						}
				}

		}		/* if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, S_PAIRED.npt_type, S_PAIRED.npt_name_s, "Paired query seqeunces", "Are the input query sequences paired?", def, PL_ALL)) != NULL) */

	FreeParameterSet (param_set_p);

	return NULL;
}



static bool GetMagicBlastServiceParameterTypeForNamedParameter (Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (strcmp (param_name_s, S_PAIRED.npt_name_s) == 0)
		{
			*pt_p = S_PAIRED.npt_type;
		}
	else
		{
			success_flag = false;
		}

	return success_flag;
}


static bool AddFormattingOptionsParameters (ParameterSet *param_set_p, BlastServiceData *data_p)
{
	bool success_flag = false;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Formatting Options", false, NULL, & (data_p -> bsd_base_data), param_set_p);
	Parameter *param_p;
	SharedType def;

	InitSharedType (&def);

	if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, S_PAIRED.npt_type, S_PAIRED.npt_name_s, "Paired query seqeunces", "Are the input query sequences paired?", def, PL_ALL)) != NULL)
		{
			success_flag = true;
		}

	return success_flag;
}


static ServiceJobSet *RunMagicBlastService (Service *service_p, ParameterSet *param_set_p, UserDetails *user_p, ProvidersStateTable *providers_p)
{
	BlastAppParameters app_params;
	ServiceJobSet *jobs_p = NULL;

	app_params.bap_parse_params_fn = ParseMagicBlastParameters;

	jobs_p = RunBlastService (service_p, param_set_p, user_p, providers_p, &app_params);

	return jobs_p;
}



static ServiceMetadata *GetMagicBlastServiceMetadata (Service *service_p)
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


static bool ParseMagicBlastParameters (const BlastServiceData *data_p, ParameterSet *params_p, ArgsProcessor *ap_p)
{
	bool success_flag = false;


	/* Percent identity cutoff for alignments */
	if (GetAndAddBlastArgs (params_p, S_PERCENT_IDENTITY.npt_name_s, false, ap_p))
		{
			/* sra */
			if (GetAndAddBlastArgs (params_p, S_SRA.npt_name_s, false, ap_p))
				{
					success_flag = true;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add \"%s\"", S_SRA.npt_name_s);
				}

		}		/* if (GetAndAddBlastArgs (params_p, S_PERCENT_IDENTITY.npt_name_s, false, ap_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add \"%s\"", S_PERCENT_IDENTITY.npt_name_s);
		}

	return success_flag;
}






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
 * blast_service_params.h
 *
 *  Created on: 13 Feb 2016
 *      Author: billy
 */

#ifndef SERVICES_BLAST_INCLUDE_BLAST_SERVICE_PARAMS_H_
#define SERVICES_BLAST_INCLUDE_BLAST_SERVICE_PARAMS_H_


#include "blast_service.h"
#include "blast_service_api.h"
#include "parameter_set.h"


/**
 * The different available output formats.
 *
 * @ingroup blast_service
 */
typedef enum BlastOutputFormat
{
	/** Pairwise */
	BOF_PAIRWISE,

	/** Query-anchored showing identities */
	BOF_QUERY_ANCHORED_WITH_IDENTITIES,

	/** Query-anchored no identities */
	BOF_QUERY_ANCHORED_NO_IDENTITIES,

	/** Flat query-anchored showing identities */
	BOF_FLAT_QUERY_ANCHORED_WITH_IDENTITIES,

	/** Flat query-anchored no identities */
	BOF_FLAT_QUERY_ANCHORED_NO_IDENTITIES,

	/** BLAST XML */
	BOF_XML_BLAST,

	/** Tabular */
	BOF_TABULAR,

	/** Tabular with comment lines */
	BOF_TABULAR_WITH_COMMENTS,

	/** Seqalign (Text ASN.1) */
	BOF_TEXT_ASN1,

	/** Seqalign (Binary ASN.1) */
	BOF_BINARY_ASN1,

	/** Comma-separated values */
	BOF_CSV,

	/** BLAST archive (ASN.1) */
	BOF_BLAST_ASN1,

	/** Seqalign (JSON) */
	BOF_JSON_SEQALIGN,

	/** Multiple-file BLAST JSON */
	BOF_MULTI_FILE_JSON_BLAST,

	/** Multiple-file BLAST XML2 */
	BOF_MULTI_FILE_XML2_BLAST,

	/** Single-file BLAST JSON */
	BOF_SINGLE_FILE_JSON_BLAST,

	/** Single-file BLAST XML2 */
	BOF_SINGLE_FILE_XML2_BLAST,

	/** Sequence Alignment/Map (SAM) */
	BOF_SEQUENCE_ALIGNMENT,

	/** Organism Report */
	BOF_ORGANISM_REPORT,

	/** Grassroots JSON */
	BOF_GRASSROOTS,

	/** The number of different output formats */
	BOF_NUM_TYPES
} BlastOutputFormat;



extern const char *BSP_OUTPUT_FORMATS_SS [BOF_NUM_TYPES];



/* Grassroots params */

/**
 * The Blast Service NamedParameterType for specifying the input filename.
 *
 * @ingroup blast_service
 */
BLAST_SERVICE_PREFIX NamedParameterType BS_INPUT_FILE BLAST_SERVICE_STRUCT_VAL ("input_file", PT_FILE_TO_READ);

/**
 * The Blast Service NamedParameterType for specifying previous job UUIDs.
 *
 * @ingroup blast_service
 */
BLAST_SERVICE_PREFIX NamedParameterType BS_JOB_ID BLAST_SERVICE_STRUCT_VAL ("job_id", PT_STRING);

/*
 * These become the -query_loc parameter with the value "<subrange_from>-<subrange_to>"
 */
/**
 * The Blast Service NamedParameterType for specifying the start of a subrange.
 *
 * @ingroup blast_service
 */
BLAST_SERVICE_PREFIX NamedParameterType BS_SUBRANGE_FROM BLAST_SERVICE_STRUCT_VAL ("subrange_from", PT_UNSIGNED_INT);

/**
 * The Blast Service NamedParameterType for specifying the end of a subrange.
 *
 * @ingroup blast_service
 */
BLAST_SERVICE_PREFIX NamedParameterType BS_SUBRANGE_TO BLAST_SERVICE_STRUCT_VAL ("subrange_to", PT_UNSIGNED_INT);

/* General Blast params */
/**
 * The Blast Service NamedParameterType for specifying the algorithm-specific task to use.
 *
 * @ingroup blast_service
 */
BLAST_SERVICE_PREFIX NamedParameterType BS_TASK BLAST_SERVICE_STRUCT_VAL ("task", PT_STRING);

/**
 * The Blast Service NamedParameterType for specifying the input query.
 *
 * @ingroup blast_service
 */
BLAST_SERVICE_PREFIX NamedParameterType BS_INPUT_QUERY BLAST_SERVICE_STRUCT_VAL ("query", PT_FASTA);

/**
 * The Blast Service NamedParameterType for specifying the maximum number of sequences to return.
 *
 * @ingroup blast_service
 */
BLAST_SERVICE_PREFIX NamedParameterType BS_MAX_SEQUENCES BLAST_SERVICE_STRUCT_VAL ("max_target_seqs", PT_UNSIGNED_INT);


/**
 * The Blast Service NamedParameterType for specifying the maximum number of e-value to use.
 *
 * @ingroup blast_service
 */
BLAST_SERVICE_PREFIX NamedParameterType BS_EXPECT_THRESHOLD BLAST_SERVICE_STRUCT_VAL ("evalue", PT_UNSIGNED_REAL);

/**
 * The Blast Service NamedParameterType for specifying the output format to use.
 *
 * @ingroup blast_service
 * @see BlastOutputFormat
 */
BLAST_SERVICE_PREFIX NamedParameterType BS_OUTPUT_FORMAT BLAST_SERVICE_STRUCT_VAL ("outfmt", PT_STRING);


/**
 * The Blast Service NamedParameterType for specifying the word size to use.
 *
 * @ingroup blast_service
 * @see BlastOutputFormat
 */
BLAST_SERVICE_PREFIX NamedParameterType BS_WORD_SIZE BLAST_SERVICE_STRUCT_VAL ("word_size", PT_UNSIGNED_INT);



/**
 * This datatype is used for describing the algorithms that some
 * of the BLAST tools allow the user to choose to optimise the
 * search procedure.
 *
 * @ingroup blast_service
 */
typedef struct BlastTask
{
	/**
	 * The name of the algorithm that will be passed to the BlastTool
	 * using the -task parameter.
	 */
  const char *bt_name_s;

  /**
   * A user-friendly descritpion of the algorithm.
   */
  const char *bt_description_s;
} BlastTask;





#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Add the common query sequence parameters for a Blast Service.
 *
 * @param data_p The configuration data for the Blast Service.
 * @param param_set_p The ParameterSet that the query sequence parameters will be added to.
 * @param callback_fn If the Blast Service wants to add extra Parameters to the query sequence group
 * of Parameters, it can do so via this callback_fn. This can be <code>NULL</code>
 * @return <code>true</code> if the query sequence parameters were added successfully, <code>
 * false</code> otherwise.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL bool AddQuerySequenceParams (BlastServiceData *data_p, ParameterSet *param_set_p, AddAdditionalParamsFn callback_fn, void *callback_data_p);


/**
 * Add the common general algorithm parameters for a Blast Service.
 *
 * @param data_p The configuration data for the Blast Service.
 * @param param_set_p The ParameterSet that the general algorithm parameters will be added to.
 * @param callback_fn If the Blast Service wants to add extra Parameters to the general algorithm group
 * of Parameters, it can do so via this callback_fn. This can be <code>NULL</code>
 * @return <code>true</code> if the general algorithm parameters were added successfully, <code>
 * false</code> otherwise.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL bool AddGeneralAlgorithmParams (BlastServiceData *data_p, ParameterSet *param_set_p, AddAdditionalParamsFn callback_fn, void *callback_data_p);


/**
 * Add the program selection parameters for a Blast Service.
 *
 * @param blast_data_p The configuration data for the Blast Service.
 * @param param_set_p The ParameterSet that the general algorithm parameters will be added to.
 * @param tasks_p An optional array of BlastTasks that be can be chosen to alter the search.
 * @param num_tasks The number of BlastTasks in the array pointed to by tasks_p.
 * @return <code>true</code> if the general algorithm parameters were added successfully, <code>
 * false</code> otherwise.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL bool AddProgramSelectionParameters (BlastServiceData *blast_data_p, ParameterSet *param_set_p, const BlastTask *tasks_p, const size_t num_tasks);


/**
 * Add the database parameters for a Blast Service.
 *
 * @param data_p The configuration data for the Blast Service.
 * @param param_set_p The ParameterSet that the general algorithm parameters will be added to.
 * @param db_type The type of databases to add.
 * @return The number of database parameters added.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL uint16 AddDatabaseParams (BlastServiceData *data_p, ParameterSet *param_set_p, const DatabaseType db_type);


/**
 * Get the number of databases of a given type that this BlastService has.
 *
 * @param data_p The configuration data for the BlastService to check.
 * @param dt The type of database to check.
 * @return The number of available databases of the given type.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL uint32 GetNumberOfDatabases (const BlastServiceData *data_p, const DatabaseType dt);



/**
 * Create the Parameter for specifying the UUIDs for any previous Blast searches.
 *
 * @param service_data_p The configuration data for the Blast Service.
 * @param param_set_p The ParameterSet that the UUID Parameter will be added to.
 * @param group_p The optional ParameterGroup to add the generated Parameter to. This can be <code>NULL</code>.
 * @return The UUID Parameter or <code>NULL</code> upon error.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL Parameter *SetUpPreviousJobUUIDParameter (const BlastServiceData *service_data_p, ParameterSet *param_set_p, ParameterGroup *group_p);


/**
 * Create the Parameter for specifying the output format from a Blast search.
 *
 * @param service_data_p The configuration data for the Blast Service.
 * @param param_set_p The ParameterSet that the output format Parameter will be added to.
 * @param group_p The optional ParameterGroup to add the generated Parameter to. This can be <code>NULL</code>.
 * @return The output format Parameter or <code>NULL</code> upon error.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL Parameter *SetUpOutputFormatParameter (const char **formats_ss, const uint32 num_formats, const char *default_format_s, const BlastServiceData *service_data_p, ParameterSet *param_set_p, ParameterGroup *group_p);


/**
 * Create the group name to use for available databases from a given named Server.
 *
 * @param server_s The name of the Server.
 * @return The newly-allocated group name that will need to be freed with
 * FreeCopiedString to avoid a memory leak or <code>NULL</code> upon error.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL char *CreateGroupName (const char *server_s);


/**
 * Get the output format code corresponding to the given string representation.
 *
 * @param output_format_s The output format string.
 * @return Upon success the BlastOutputFormat cast to an int8. -1 if the string
 * did not represent a valid output format string.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL int8 GetOutputFormatCodeForString (const char *output_format_s);



BLAST_SERVICE_LOCAL bool GetDatabaseParameterTypeForNamedParameter (BlastServiceData *data_p, const char *param_name_s, ParameterType *pt_p);


BLAST_SERVICE_LOCAL bool GetQuerySequenceParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p);


BLAST_SERVICE_LOCAL bool GetGeneralAlgorithmParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p);


BLAST_SERVICE_LOCAL bool GetProgramSelectionParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p);


BLAST_SERVICE_LOCAL bool GetProteinGeneralAlgorithmParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p);


BLAST_SERVICE_LOCAL char *GetLocalDatabaseGroupName (void);


BLAST_SERVICE_LOCAL char *GetFullyQualifiedDatabaseName (const char *group_s, const char *db_s);


BLAST_SERVICE_LOCAL const char *GetLocalDatabaseName (const char *fully_qualified_db_s);


BLAST_SERVICE_LOCAL bool AddProteinGeneralAlgorithmParameters (BlastServiceData *data_p, ParameterSet *param_set_p, ParameterGroup *group_p, void *callback_data_p);



#ifdef __cplusplus
}
#endif

#endif /* SERVICES_BLAST_INCLUDE_BLAST_SERVICE_PARAMS_H_ */

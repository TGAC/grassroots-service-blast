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


#ifndef BLAST_SERVICE_H
#define BLAST_SERVICE_H


#include "blast_app_parameters.h"
#include "blast_formatter.h"
#include "blast_service_api.h"
#include "parameter_set.h"
#include "temp_file.hpp"
#include "async_tasks_manager.h"


/**
 * An enumeration for differentiating between
 * the different types of database that the
 * BLAST algorithms can be used with.
 *
 * @ingroup blast_service
 */
typedef enum BLAST_SERVICE_LOCAL DatabaseType
{
	/** The database is a nucleotide sequence. */
	DT_NUCLEOTIDE,

	/** The database is a protein sequence. */
	DT_PROTEIN,

	/** The number of different database types. */
	DT_NUM_TYPES
} DatabaseType;


/**
 * An enumeration for differentiating between
 * the differentBLAST algorithms that can be
 * used.
 *
 * @ingroup blast_service
 */
typedef enum BLAST_SERVICE_LOCAL BlastServiceType
{
	/** A search using the BlastN program. */
	BST_BLASTN,

	/** A search using the BlastP program. */
	BST_BLASTP,

	/** A search using the BlastX program. */
	BST_BLASTX,

	/** The number of different Blast service types. */
	BST_NUM_TYPES
} BlastServiceType;

/**
 * A datatype describing the details of each database available
 * to search against.
 *
 * @ingroup blast_service
 */
typedef struct BLAST_SERVICE_LOCAL DatabaseInfo
{
	/** The name of the database to display to the user. */
	const char *di_name_s;

	/** The filename for the database */
	const char *di_filename_s;

	/** The description of the database to display to the user. */
	const char *di_description_s;

	/** The URI to download any associated resources for this database. */
	const char *di_download_uri_s;

	/** The URI to for any supplementary information about the database. */
	const char *di_info_uri_s;

	/**
	 * Sets whether the database defaults to being searched against
	 * or not.
	 */
	bool di_active_flag;

	/** The DatabaseType for this database. */
	DatabaseType di_type;

	/**
	 * The key used to get the scaffold name for
	 * any hits from BLAST searches from within the
	 * "BlastOutput2.report.results.search.hits.description" field
	 * of the search result in single file JSON format.
	 * This defaults to "id".
	 */
	const char *di_scaffold_key_s;

	/**
	 * The regular expression used to get the scaffold name for the value
	 * associated with the value retrieved from di_scaffold_key_s. This
	 * defaults to NULL which means to treat the entire value as the
	 * scaffold name.
	 */
	const char *di_scaffold_regex_s;

} DatabaseInfo;



/* forward class declarations */
class BlastToolFactory;
struct BlastServiceJob;

/**
 * The configuration data for the Blast Service.
 *
 * @extends ServiceData
 * @ingroup blast_service
 */
typedef struct BLAST_SERVICE_LOCAL BlastServiceData
{
	/** The base ServiceData. */
	ServiceData bsd_base_data;

	/**
	 * The directory where the Blast input, output and log files
	 * will be stored.
	 */
	const char *bsd_working_dir_s;

	/**
	 * The BlastFormatter used to convert the Blast output
	 * between the different output formats.
	 */
	BlastFormatter *bsd_formatter_p;

	/** A NULL-terminated array of the databases available to search */
	DatabaseInfo *bsd_databases_p;

	/** The BlastToolFactory used to generate each BlastTool that actually run the Blast jobs. */
	BlastToolFactory *bsd_tool_factory_p;


	/** Specifies whether the databases are nucleotide or protein databases. */
	DatabaseType bsd_type;


	AsyncTasksManager *bsd_task_manager_p;


} BlastServiceData;


/**
 * A callback function used to amend a given ParameterSet.
 *
 * @param data_p The configuration data for the Blast Service.
 * @param param_set_p The ParameterSet that the callback function's Parameters will be added to.
 * @param group_p The optional ParameterGroup to add the generated Parameter to. This can be <code>NULL</code>.
 * @return <code>true</code> if the callback function's parameters were added successfully, <code>
 * false</code> otherwise.
 */
typedef bool (*AddAdditionalParamsFn) (BlastServiceData *data_p, ParameterSet *param_set_p, ParameterGroup *group_p, void *callback_data_p);


#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_BLAST_SERVICE_CONSTANTS
	#define BLAST_SERVICE_PREFIX BLAST_SERVICE_LOCAL
	#define BLAST_SERVICE_VAL(x)	= x
	#define BLAST_SERVICE_STRUCT_VAL(x,y)	= { x, y}
#else
	#define BLAST_SERVICE_PREFIX extern
	#define BLAST_SERVICE_VAL(x)
	#define BLAST_SERVICE_STRUCT_VAL(x,y)
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */


/**
 * The default output format as a number to use.
 *
 * @see BS_DEFAULT_OUTPUT_FORMAT_S
 */
#define BS_DEFAULT_OUTPUT_FORMAT (11)

/** The suffix to use for Blast Service input files. */
BLAST_SERVICE_PREFIX const char *BS_INPUT_SUFFIX_S BLAST_SERVICE_VAL (".input");

/** The suffix to use for Blast Service output files. */
BLAST_SERVICE_PREFIX const char *BS_OUTPUT_SUFFIX_S BLAST_SERVICE_VAL (".output");

/** The suffix to use for Blast Service log files. */
BLAST_SERVICE_PREFIX const char *BS_LOG_SUFFIX_S BLAST_SERVICE_VAL (".log");

/**
 * The default output format as a string to use.
 *
 * @see BS_DEFAULT_OUTPUT_FORMAT
 */
BLAST_SERVICE_PREFIX const char *BS_DEFAULT_OUTPUT_FORMAT_S BLAST_SERVICE_VAL ("11");

/** The prefix to use for the ParameterGroup names for available databases from all of the connected servers. */
BLAST_SERVICE_PREFIX const char *BS_DATABASE_GROUP_NAME_S BLAST_SERVICE_VAL ("Available Databases");


BLAST_SERVICE_PREFIX const char *BS_DATABASES_S BLAST_SERVICE_VAL ("databases");

/**
 * The configuration key used to declare which type of BlastTool to use.
 */
BLAST_SERVICE_PREFIX const char *BS_TOOL_TYPE_NAME_S BLAST_SERVICE_VAL ("blast_tool");

/**
 * The configuration key used to declare the Blast executable file to use.
 */
BLAST_SERVICE_PREFIX const char *BS_COMMAND_NAME_S BLAST_SERVICE_VAL ("blast_command");


/**
 * The configuration key used to declare the type of Blast application to use.
 */
BLAST_SERVICE_PREFIX const char *BS_APP_NAME_S BLAST_SERVICE_VAL ("blast_app_type");


/** The prefix to use for Blast Service aliases. */
#define BS_GROUP_ALIAS_PREFIX_S "blast/"


#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Get the Services available for running BLAST jobs.
 *
 * @param user_p The details for the user accessing the BLAST Services.
 * @return The ServicesArray containing all of the BLAST Services or
 * <code>NULL</code> upon error.
 * @ingroup blast_service
 */
BLAST_SERVICE_API ServicesArray *GetServices (UserDetails * user_p, GrassrootsServer *grassroots_p);


/**
 * Free the ServicesArray and its associated BLAST Services.
 *
 * @param services_p The ServicesArray to free.
 * @ingroup blast_service
 */
BLAST_SERVICE_API void ReleaseServices (ServicesArray *services_p);



/**
 * Run a Blast Service.
 *
 * @param service_p The Blast Service to run.
 * @param param_set_p The ParameterSet specifying the Parameters to use.
 * @param user_p The UserDetails of the user requesting to run the Service.
 * @param providers_p The details of ExternalServers for any paired or external Blast Services.
 * @param app_params_p The parser used for the current type of BlastTool.
 * @return The ServiceJobSet with the BlastServiceJobs or <code>NULL</code> upon error.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL ServiceJobSet *RunBlastService (Service *service_p, ParameterSet *param_set_p, UserDetails *user_p, ProvidersStateTable *providers_p, BlastAppParameters *app_params_p);


/**
 * Check whether the required configuration details contain the
 * required information for a Blast Service to be used.
 *
 * @param data_p The BlastServcieData to check.
 * @return <code>true</code> if the configuration details are sufficient,
 * <code>false</code> otherwise.
 * @memberof BlastServiceData * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL bool GetBlastServiceConfig (BlastServiceData *data_p);


/**
 * Allocate a BlastServiceData.
 *
 * @param blast_service_p The Blast Service that will own this BlastServiceData.
 * @param database_type The type of data that the given Blast Service can act upon.
 * @return The newly-created BlastServiceData or <code>NULL</code> upon error.
 * @memberof BlastServiceData
 */
BLAST_SERVICE_LOCAL BlastServiceData *AllocateBlastServiceData (Service *blast_service_p, DatabaseType database_type);


/**
 * Free a BlastServiceData.
 *
 * @param data_p The BlastServiceData to free.
 * @memberof BlastServiceData
 */
BLAST_SERVICE_LOCAL void FreeBlastServiceData (BlastServiceData *data_p);


/**
 * Close a Blast Service.
 *
 * @param service_p The Blast Service to close.
 * @return <code>true</code> if the Blast Service was closed successfully, <code>
 * false</code> otherwise.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL bool CloseBlastService (Service *service_p);


/**
 * Check whether a given Resource is suitable for running a Blast Service for and
 * if so return the partially-completed ParameterSet.
 *
 * @param service_p The Blast Service to check.
 * @param resource_p The given Resource to check for.
 * @param handler_p The appropriate Handler for accessing the given Resource.
 * @return The partially-completed ParameterSet if the Resource is an appropriate one
 * or <code>NULL</code> if it is not valid for running a Blast search against.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL ParameterSet *IsResourceForBlastService (Service *service_p, Resource *resource_p, Handler *handler_p);


/**
 * Add the common Blast Service parameters to a given ParameterSet.
 *
 * @param blast_service_p The Blast Service to add the Parameters for.
 * @param param_set_p The ParameterSet to add the common Parameters to.
 * @param db_type The type of databases that the given Blast Service will run against.
 * @param add_additional_params_fn If a Blast Service wishes to add additional parameters
 * this optional callback function can be used.
 * @return <code>true</code> if the Parameters were added successfully, <code>false</code>
 * otherwise.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL bool AddBaseBlastServiceParameters (Service *blast_service_p, ParameterSet *param_set_p, const DatabaseType db_type, AddAdditionalParamsFn query_sequence_callback_fn, void *callback_data_p);


BLAST_SERVICE_LOCAL bool GetBaseBlastServiceParameterTypeForNamedParameter (const Service *service_p, const char *param_name_s, ParameterType *pt_p);


/**
 * Free a ParameterSet of Blast Service parameters.
 *
 * @param service_p The BlastService of the same type that allocated the given ParameterSet.
 * @param params_p The ParameterSet to free.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL void ReleaseBlastServiceParameters (Service *service_p, ParameterSet *params_p);


/**
 * Get the results of a given BlastServiceJob and store them within it.
 *
 * @param job_p The BlastServiceJob to get the results for.
 * @return <code>true</code> if the results were added successfully, <code>false</code>
 * otherwise.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL bool DetermineBlastResult ( struct BlastServiceJob *job_p);


/**
 * Get the OperationStatus for a BlastServiceJob with the given job id.
 *
 * @param service_p The Blast Service of the type which ran the BlastServiceJob.
 * @param service_id The UUID for the given BlastServiceJob.
 * @return The OperationStatus for the given BlastServiceJob.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL OperationStatus GetBlastServiceStatus (Service *service_p, const uuid_t service_id);


/**
 * Get the TempFile detailing the query for a given BlastServiceJob UUID.
 *
 * @param params_p The ParameterSet used to run the ServiceJob.
 * @param working_directory_s The working directory specified in the Blast Service configuration.
 * @param job_id The UUID of the BlastServiceJob to check.
 * @return A newly-allocated TempFile with the relevant values or <code>NULL</code>
 * upon error.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL TempFile *GetInputTempFile (const ParameterSet *params_p, const char *working_directory_s, const uuid_t job_id);


/**
 * Get the result of a previously ran BlastServiceJob in a given output format.
 *
 * @param data_p The BlastServiceData of the Blast Service that ran the job.
 * @param job_id The ServiceJob identifier to get the results for.
 * @param output_format_code The required output format code.
 * @return A newly-allocated string containing the results in the requested format
 * or <code>NULL</code> upon error.
 * @see GetBlastResultByUUIDString
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL char *GetBlastResultByUUID (const BlastServiceData *data_p, const uuid_t job_id, const uint32 output_format_code);


/**
 * Get the result of a previously ran BlastServiceJob in a given output format.
 *
 * @param data_p The BlastServiceData of the Blast Service that ran the job.
 * @param job_id_s The ServiceJob identifier, as a string, to get the results for.
 * @param output_format_code The required output format code.
 * @return A newly-allocated string containing the results in the requested format
 * or <code>NULL</code> upon error.
 * @see GetBlastResultByUUID
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL char *GetBlastResultByUUIDString (const BlastServiceData *data_p, const char *job_id_s, const uint32 output_format_code);


/**
 * Get the results of previously ran BlastServiceJobs in a given output format.
 *
 * @param ids_p A LinkedList of StringListNodes each containing the UUIDs for the required
 * ServiceJobs.
 * @param blast_data_p The BlastServiceData of the Blast Service that ran the job.
 * @param output_format_code The required output format code.
 * @return A newly-allocated ServiceJobSet containing the results in the requested format
 * or <code>NULL</code> upon error.
 * @see GetBlastResultByUUIDString
 * @see CreateJobsForPreviousResults
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL ServiceJobSet *GetPreviousJobResults (LinkedList *ids_p, BlastServiceData *blast_data_p, const uint32 output_format_code);


/**
 * Get the results of previously ran BlastServiceJobs in a given output format.
 *
 * @param params_p The ParameterSet to get the output format from.
 * @param ids_s A string containing UUIDs separated by whitespace.
 * @param blast_data_p The Blast Service configuration data.
 * @return The ServiceJobSet containing the results in the requested output format
 * or <code>NULL</code> upon error.
 * @see GetPreviousJobResults
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL ServiceJobSet *CreateJobsForPreviousResults (ParameterSet *params_p, const char *ids_s, BlastServiceData *blast_data_p);


/**
 * Prepare a ServiceJobSet of BlastServiceJobs prior to running them.
 *
 * @param db_p The list of available databases terminated by <code>NULL</code>.
 * @param param_set_p The ParameterSet for specifying the configuration of the BlastServiceJobs.
 * @param service_p The BlastService.
 * @param data_p The Blast Service configuration data.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL void PrepareBlastServiceJobs (const DatabaseInfo *db_p, const ParameterSet * const param_set_p, Service *service_p, BlastServiceData *data_p);

/**
 * Create a BlastServiceJob from a JSON-based serialisation.
 *
 * @param service_p The type of Blast Service that previously created the BlastServiceJob.
 * @param service_job_json_p The JSON fragment representing the BlastServiceJob.
 * @return The BlastServiceJob or <code>NULL</code> upon error.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL ServiceJob *BuildBlastServiceJob (struct Service *service_p, const json_t *service_job_json_p);


/**
 * Get the JSON representation of a BlastServiceJob.
 *
 * @param service_p The Service that ran the BlastServiceJob.
 * @param service_job_p The BlastServiceJob to serialise.
 * @param omit_results_flag If this is <code>true</code> then just the minimal status information for
 * the ServiceJob will be returned. If it is <code>false</code> then the job results will be included too if possible.
 * @return The JSON fragment representing the BlastServiceJob or <code>NULL</code>
 * upon error.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL json_t *BuildBlastServiceJobJSON (Service * UNUSED_PARAM (service_p), ServiceJob *service_job_p, bool omit_results_flag);


/**
 * Initialise a BlastServiceJob with its required functions for
 * updating and freeing itself.
 *
 * @param service_p The BlastService that has created the BlastServiceJob.
 * @param job_p The BlastServiceJob.
 * @memberof BlastServiceJob
 */
BLAST_SERVICE_LOCAL void CustomiseBlastServiceJob (Service *service_p, ServiceJob *job_p);


/**
 * For a given service-configured name, find the corresponding BLAST database filename.
 *
 * @param data_p The BlastServiceData with the service configuration details.
 * @param name_s The service-configured name.
 * @return The corresponding BLAST database filename or <code>NULL</code> if it could
 * not be found.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL const char *GetMatchingDatabaseFilename (const BlastServiceData *data_p, const char *name_s);


/**
 * For a given BLAST database filename, find the corresponding service-configured name.
 *
 * @param data_p The BlastServiceData with the service configuration details.
 * @param filename_s The BLAST database filename.
 * @return The corresponding service-configured name or <code>NULL</code> if it could
 * not be found.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL const char *GetMatchingDatabaseName (const BlastServiceData *data_p, const char *filename_s);



BLAST_SERVICE_LOCAL ServiceMetadata *GetGeneralBlastServiceMetadata (Service *service_p);


/**
 * For a given BLAST database filename, find the corresponding DatabaseInfo object.
 *
 * @param data_p The BlastServiceData with the service configuration details.
 * @param filename_s The BLAST database filename.
 * @return The corresponding DatabaseInfo object or <code>NULL</code> if it could
 * not be found.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL const DatabaseInfo *GetMatchingDatabaseByFilename (const BlastServiceData *data_p, const char *filename_s);


BLAST_SERVICE_LOCAL void ReleaseBlastService (Service *service_p);


BLAST_SERVICE_LOCAL json_t *GetBlastIndexingData (struct Service *service_p);


#ifdef __cplusplus
}
#endif



#endif		/* #ifndef BLAST_SERVICE_H */

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
#ifndef BLAST_TOOL_HPP
#define BLAST_TOOL_HPP

#include <vector>

#include "blast_app_parameters.h"
#include "blast_service_api.h"

#include "byte_buffer.h"

#include "service_job.h"
#include "blast_formatter.h"


/* forward declaration */
struct BlastServiceData;
struct BlastServiceJob;

/**
 * The base class for running Blast.
 *
 * @ingroup blast_service
 */
class BLAST_SERVICE_LOCAL BlastTool
{
public:

	/**
	 * Create a BlastTool for a given ServiceJob.
	 *
	 * @param job_p The ServiceJob to associate with this BlastTool.
	 * @param name_s The name to give to this BlastTool.
	 * @param data_p The BlastServiceData for the Service that will run this BlastTool.
	 * @param factory_s The name of the BlastToolFactory that created this BlastTool.
	 * @param output_format The output format that this BlastTool will produce its results in.
	 * @see BlastServiceJob
	 */
	BlastTool (BlastServiceJob *job_p, const char *name_s, const char *factory_s, const BlastServiceData *data_p, const uint32 output_format);


	/**
	 * Create a BlastTool for a given ServiceJob.
	 *
	 * @param job_p The ServiceJob to associate with this BlastTool.
	 * @param data_p The BlastServiceData for the Service that will run this BlastTool.
	 * @param json_p The JSON fragment representing a serialised version of the BlastTool.
	 * @see BlastServiceJob
	 */
	BlastTool (BlastServiceJob *job_p, const BlastServiceData *data_p, const json_t *json_p);


	/**
	 * The BlastTool destructor,
	 */
	virtual ~BlastTool ();


	/**
	 * Get the output format code that this BlastTool will produce its
	 * results in.
	 *
	 * @return The output format code.
	 */
	uint32 GetOutputFormat () const;


	/**
	 * Run this BlastTool
	 *
	 * @return The OperationStatus of this BlastTool after
	 * it has started running.
	 */
	virtual OperationStatus Run () = 0;


	/**
	 * Parse a ParameterSet to configure a BlastTool prior
	 * to it being ran.
	 *
	 * @param param_set_p The ParameterSet to parse.
	 * @param app_params_p The BlastAppParameters to use process the
	 * values from the given ParameterSe
	 * @return <code>true</code> if the BlastTool was configured
	 * successfully and is ready to be ran, <code>false</code>
	 * otherwise.
	 */
	virtual bool ParseParameters (ParameterSet *param_set_p, BlastAppParameters *app_params_p) = 0;

	/**
	 * Set the input filename for the BlastTool to use.
	 *
	 * @param filename_s The filename.
	 * @return <code>true</code> if the input filename was set
	 * successfully, <code>false</code> otherwise.
	 */
	virtual bool SetInputFilename (const char * const filename_s) = 0;


	/**
	 * Set the output file parameter.
	 *
	 * @return <code>true</code> if the input filename was set
	 * successfully, <code>false</code> otherwise.
	 */
	virtual bool SetUpOutputFile () = 0;


	/**
	 * Any custom functionality required before running a BlastTool.
	 */
	virtual bool PreRun ();
	

	/**
	 * Any custom functionality required after running a BlastTool.
	 */
	virtual void PostRun ();

	/**
	 * Get the status of a BlastTool
	 *
	 * @param update_flag if this is <code>true</code> then the BlastTool
	 * will check the status of its running jobs if necessary, if this is
	 * <code>false</code> it will return the last cached value.
	 * @return The OperationStatus of this BlastTool.
	 */
	virtual OperationStatus GetStatus (bool update_flag = true);


	/**
	 * Get the results after the ExternalBlastTool has finished
	 * running.
	 *
	 * @param formatter_p The BlastFormatter to convert the results
	 * into a different format. If this is 0, then the results will be
	 * returned without any conversion.
	 * @return The results as a c-style string or 0 upon error.
	 */
	virtual char *GetResults (BlastFormatter *formatter_p) = 0;

	/**
	 * Get the log after the BlastTool has finished
	 * running.
	 *
	 * @return The results as a c-style string or 0 upon error.
	 */
	virtual char *GetLog () = 0;

	/**
	 * Get the uuid for the ServiceJob that this BlastTool
	 * is linked with.
	 * @return The uuid of the corresponding ServiceJob for this
	 * BlastTool.
	 */
	const uuid_t &GetUUID () const;

	/**
	 * Get the name asssociated with this BlastTool
	 *
	 * @return The name.
	 */
	const char *GetName () const;


	/**
	 * Get the JSON-based persistent description of this BlastTool.
	 *
	 * This allows the BlastTool to be recreated in another process or thread.
	 *
	 * @return The json_t description of this BlastTool or 0 upon error.
	 */
	json_t *GetAsJSON ();


	/**
	 * Get the name of the BlastToolFactory that created this BlastTool.
	 *
	 * @return The name of the BlastToolFactory.
	 */
	const char *GetFactoryName () const;


protected:


	/**
	 * @private
	 *
	 * The name of this BlastTool.
	 */
	const char *bt_name_s;

	/**
	 * @private
	 *
	 * The name of the BlastToolFactpry that created this BlastTool.
	 */
	const char *bt_factory_name_s;


	/**
	 *
	 * The output format code to use.
	 */
	uint32 bt_output_format;




	/**
	 * @private
	 *
	 * The ServiceJob associated with this BlastTool.
	 * @see BlastServiceJob
	 */
	BlastServiceJob *bt_job_p;

	/**
	 * @private
	 *
	 * The ServiceData for this BlastTool.
	 */
	const BlastServiceData *bt_service_data_p;


	/**
	 * This method is used to serialise this BlastTool so that
	 * it can be recreated from another calling process when required.
	 * This is used to store the BlastTool in the JobsManager.
	 *
	 * @param root_p The json object to add the serialisation of this BlastTool to.
	 * @return <code>true</code> if the BlastTool was serialised
	 * successfully, <code>false</code>
	 * otherwise.
	 */
	virtual bool AddToJSON (json_t *root_p);


private:
	static const char * const BT_NAME_S;
	static const char * const BT_FACTORY_NAME_S;
	static const char * const BT_OUTPUT_FORMAT_S;


};


/*
* I can see that potentially we could use a variety of methods:
* 
* 1. Fork a new process.
* 2. Create a DBApplication and run it inline.
* 3. Create a DBApplication and run it in a separate thread using pthreads or similar.
* 4. Create a job submission script and call it, this would be on HPC or similar.
* 
* So we need a way to call any of these and check on their status, the obvious answer
* is subclassing with virtual calls.
*/


#ifdef __cplusplus
extern "C" 
{
#endif


/**
 * Free a BlastTool
 * 
 * @param tool_p The BlastTool to deallocate.
 * @memberof BlastTool
 */
BLAST_SERVICE_API void FreeBlastTool (BlastTool *tool_p);



/**
 * Run Blast using the parameters that have been previously using ConvertArguments.
 * 
 * @param tool_p The BlastTool to use.
 * @return <code>true</code> if the tool completed successfully, <code>false</code>
 * otherwise.
 * @memberof BlastTool
 */
BLAST_SERVICE_API OperationStatus RunBlast (BlastTool *tool_p);

/**
 * Get the current OperationStatus of a BlastTool.
 *
 * @param tool_p The BlastTool to check.
 * @return The current OperationStatus.
 * @memberof BlastTool
 */
BLAST_SERVICE_API OperationStatus GetBlastStatus (BlastTool *tool_p);


/**
 * Is the BlastTool going to run asynchronously?
 *
 * @param tool_p The BlastTool to check.
 * @return <code>true</code> if the tool will run asynchronously, <code>false</code>
 * otherwise.
 * @memberof BlastTool
 */
BLAST_SERVICE_API bool IsBlastToolSynchronous (BlastTool *tool_p);


#ifdef __cplusplus
}
#endif


#endif		/* #ifndef BLAST_TOOL_HPP */


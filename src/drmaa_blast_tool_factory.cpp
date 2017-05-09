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
/*
 * drmaa_blast_tool_factory.cpp
 *
 *  Created on: 24 Feb 2016
 *      Author: tyrrells
 */

#include "json_util.h"
#include "streams.h"
#include "drmaa_blast_tool_factory.hpp"
#include "drmaa_blast_tool.hpp"




DrmaaBlastToolFactory	*DrmaaBlastToolFactory :: CreateDrmaaBlastToolFactory (const json_t *service_config_p)
{
	DrmaaBlastToolFactory *factory_p = 0;

	try
		{
			factory_p = new DrmaaBlastToolFactory (service_config_p);
		}
	catch (std :: bad_alloc &alloc_r)
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate DrmaaBlastToolFactory");
		}

	return factory_p;
}



DrmaaBlastToolFactory :: DrmaaBlastToolFactory (const json_t *service_config_p)
	: ExternalBlastToolFactory (service_config_p)
{

}


DrmaaBlastToolFactory :: ~DrmaaBlastToolFactory ()
{

}



BlastTool *DrmaaBlastToolFactory :: CreateBlastTool (BlastServiceJob *job_p, const char *name_s, const BlastServiceData *data_p)
{
	DrmaaBlastTool *drmaa_tool_p = 0;
	bool async_flag = true;
	const json_t *blast_config_p = data_p -> bsd_base_data.sd_config_p;
	const json_t *drmaa_blast_tool_config_p = json_object_get (blast_config_p, "drmaa_blast_tool_config");

	if (drmaa_blast_tool_config_p)
		{
			/* Set the queue to use */
			const char *queue_s = GetJSONString (drmaa_blast_tool_config_p, "queue");

			if (queue_s)
				{
					const char *output_path_s = GetJSONString (blast_config_p, "working_directory");

					if (output_path_s)
						{
							GetJSONBoolean (drmaa_blast_tool_config_p, ExternalBlastTool :: EBT_ASYNC_S, &async_flag);


							try
								{
									drmaa_tool_p = new DrmaaBlastTool (job_p, name_s, GetName (), data_p, ebtf_program_name_s, queue_s, output_path_s, async_flag);
								}
							catch (std :: exception &ex_r)
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create drmaa blast tool, error \"%s\"", ex_r.what ());
								}

							if (drmaa_tool_p)
								{
									int i = 0;
									const char *value_s = NULL;

									/* Set the number of cores per job */
									if (GetJSONInteger (drmaa_blast_tool_config_p, "drmaa_cores_per_search", &i))
										{
											drmaa_tool_p -> SetCoresPerSearch ((uint32) i);
										}

									/* Set up any email notifications */
									value_s = GetJSONString (drmaa_blast_tool_config_p, "email_notifications");

									if (value_s)
										{
											const char **addresses_ss = (const char **) AllocMemoryArray (2, sizeof (const char *));

											if (addresses_ss)
												{
													*addresses_ss = value_s;
												}

											if (! (drmaa_tool_p -> SetEmailNotifications (addresses_ss)))
												{
													PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to set email notifications for drmaa tool");
												}

											FreeMemory (addresses_ss);
										}		/* if (json_p) */

								}		/* if (drmaa_tool_p) */

						}		/* if (output_path_s) */
					else
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get working_directory from config");
						}

				}		/* if (queue_s) */
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get queue name from config");
				}
		}		/* if (drmaa_blast_tool_config_p) */
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get drmaa_blast_tool_config from config");
		}


	return drmaa_tool_p;
}


Synchronicity DrmaaBlastToolFactory :: GetToolsSynchronicity () const
{
	return SY_ASYNCHRONOUS_DETACHED;
}


const char *DrmaaBlastToolFactory :: GetName ()
{
	return "Drmaa Blast Tool Factory";
}


BlastTool *DrmaaBlastToolFactory :: CreateBlastTool (const json_t *root_p, BlastServiceJob *blast_job_p, BlastServiceData *service_data_p)
{
	DrmaaBlastTool *tool_p = new DrmaaBlastTool (blast_job_p, service_data_p, root_p);

	return tool_p;
}



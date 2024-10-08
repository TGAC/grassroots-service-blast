NAME 		:= blast_service
DIR_BUILD :=  $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/$(PLATFORM))
DIR_SRC := $(realpath $(DIR_BUILD)/../../../src)
DIR_INCLUDE := $(realpath $(DIR_BUILD)/../../../include)

ifeq ($(DIR_BUILD_CONFIG),)
export DIR_BUILD_CONFIG = $(realpath $(DIR_BUILD)/../../../../../build-config/unix/)
#export DIR_BUILD_CONFIG = /Users/tyrrells/Projects/grassroots/build-config/unix/
endif

include $(DIR_BUILD_CONFIG)/project.properties

export DIR_INSTALL := $(DIR_GRASSROOTS_INSTALL)/services

BUILD	:= debug

VPATH := \
	$(DIR_SRC)

INCLUDES := \
	-I$(DIR_INCLUDE) \
	-I$(DIR_GRASSROOTS_USERS_INC) \
	-I$(DIR_GRASSROOTS_UTIL_INC) \
	-I$(DIR_GRASSROOTS_UTIL_INC)/containers \
	-I$(DIR_GRASSROOTS_UTIL_INC)/io \
	-I$(DIR_GRASSROOTS_HANDLER_INC) \
	-I$(DIR_GRASSROOTS_NETWORK_INC) \
	-I$(DIR_GRASSROOTS_SERVER_INC) \
	-I$(DIR_GRASSROOTS_SERVICES_INC) \
	-I$(DIR_GRASSROOTS_SERVICES_INC)/parameters \
	-I$(DIR_GRASSROOTS_PLUGIN_INC) \
	-I$(DIR_GRASSROOTS_TASK_INC) \
	-I$(DIR_JANSSON_INC) \
	-I$(DIR_UUID_INC) \
	-I$(DIR_GRASSROOTS_UUID_INC) \
	-I$(DIR_BLAST_INC) \
	-I$(DIR_PCRE2_INC) \
	-I$(DIR_BSON_INC)
	
SRCS 	:= \
	alloc_failure.cpp \
	args_processor.cpp \
	async_system_blast_tool.cpp \
	blast_app_parameters.cpp \
	blast_service.cpp \
	blastn_service.cpp \
	blastp_service.cpp \
	blastx_service.cpp \
	blast_service_job.cpp \
	blast_service_job_markup.cpp \
	blast_service_params.cpp \
	blast_formatter.cpp \
	blast_tool.cpp \
	blast_tool_factory.cpp \
	blast_util.cpp \
	byte_buffer_args_processor.cpp \
	external_blast_tool.cpp \
	external_blast_tool_factory.cpp \
	magic_blast_service.cpp \
	paired_blast_service.cpp \
	polymarker_linked_service.cpp \
	strings_args_processor.cpp \
	system_blast_tool.cpp \
	system_blast_tool_factory.cpp \
	temp_file.cpp
	

CPPFLAGS += -DBLAST_LIBRARY_EXPORTS
CXXFLAGS += 

LDFLAGS += -L$(DIR_JANSSON_LIB) -ljansson \
	-L$(DIR_GRASSROOTS_UTIL_LIB) -l$(GRASSROOTS_UTIL_LIB_NAME) \
	-L$(DIR_GRASSROOTS_UUID_LIB) -l$(GRASSROOTS_UUID_LIB_NAME) \
	-L$(DIR_GRASSROOTS_SERVICES_LIB) -l$(GRASSROOTS_SERVICES_LIB_NAME) \
	-L$(DIR_GRASSROOTS_SERVER_LIB) -l$(GRASSROOTS_SERVER_LIB_NAME) \
	-L$(DIR_GRASSROOTS_NETWORK_LIB) -l$(GRASSROOTS_NETWORK_LIB_NAME) \
	-L$(DIR_GRASSROOTS_TASK_LIB) -l$(GRASSROOTS_TASK_LIB_NAME) 


add_drmaa = 0
ifeq ($(SLURM_DRMAA_ENABLED), 1)
    add_drmaa = 1
endif
ifeq ($(HTCONDOR_DRMAA_ENABLED), 1)
    add_drmaa = 1
endif
ifeq ($(LSF_DRMAA_ENABLED), 1)
    add_drmaa = 1
endif


ifeq ($(add_drmaa),1)
SRCS += \
	drmaa_blast_tool.cpp \
	drmaa_blast_tool_factory.cpp \
	drmaa_tool_args_processor.cpp 
	
LDFLAGS += -L$(DIR_GRASSROOTS_DRMAA_LIB) -l$(GRASSROOTS_DRMAA_LIB_NAME) -L$(DIR_DRMAA_IMPLEMENTATION_LIB) -l$(DRMAA_IMPLEMENTATION_LIB_NAME)
INCLUDES += -I$(DIR_LSF_DRMAA_INC) -I$(DIR_GRASSROOTS_DRMAA_INC)
CPPFLAGS += -DDRMAA_ENABLED=1
endif


BLAST_SEARCH_LDFLAGS = \
	-L$(DIR_BLAST_LIB) \
	-lblast_app_util  \
	-lblastinput-static  \
	-lncbi_xloader_blastdb-static  \
	-lncbi_xloader_blastdb_rmt-static  \
	-lxblastformat-static  \
	-lalign_format-static  \
	-ltaxon1-static  \
	-lblastdb_format-static  \
	-lgene_info-static  \
	-lxalnmgr-static  \
	-lblastxml-static  \
	-lxcgi-static  \
	-lxhtml-static  \
	-lxblast-static  \
	-lxalgoblastdbindex-static  \
	-lcomposition_adjustment-static  \
	-lxalgodustmask-static  \
	-lxalgowinmask-static  \
	-lseqmasks_io-static  \
	-lseqdb-static  \
	-lblast_services-static  \
	-lxobjutil-static  \
	-lxobjread-static  \
	-lvariation-static  \
	-lcreaders-static  \
	-lsubmit-static  \
	-lxnetblastcli-static  \
	-lxnetblast-static  \
	-lblastdb-static  \
	-lscoremat-static  \
	-ltables-static  \
	-lxalnmgr-static  \
	-lncbi_xloader_genbank-static  \
	-lncbi_xreader_id1-static  \
	-lncbi_xreader_id2-static  \
	-lncbi_xreader_cache-static  \
	-lxconnext-static  \
	-lxconnect-static  \
	-ldbapi_driver-static  \
	-lncbi_xreader-static  \
	-lxconnext-static  \
	-lxconnect-static  \
	-lid1-static  \
	-lid2-static  \
	-lseqsplit-static  \
	-lxcompress-static  \
	-lxobjmgr-static  \
	-lgenome_collection-static  \
	-lseqedit-static  \
	-lseqset-static  \
	-lseq-static  \
	-lseqcode-static  \
	-lsequtil-static  \
	-lpub-static  \
	-lmedline-static  \
	-lbiblio-static  \
	-lgeneral-static  \
	-lxser-static  \
	-lxutil-static  \
	-lxncbi-static  \
	-lz   \
	-lbz2    \
	-ldl  \
	-lnsl  \
	-lrt  \
	-lm   \
	-lpthread 	


include $(DIR_BUILD_CONFIG)/generic_makefiles/shared_library.makefile


/*
	Parameters.cpp

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 
#include "parameters.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>


using namespace std;
// Constructor
/*
 Parameters::Parameters(){
	reset()
	for(uint32_t index=0;index<MAX_PARAMETERS; index++){
		parameters[index].param_value=0;
		parameters[index].param_count=0;
		parameters[index].param_index=0;

		for(int a=0;a<16;a++){
			parameters[index].param_id[a]=0; 
		}

		parameters[index].param_type=0;
	}	
 }
 */
 Parameters::Parameters(std::string filePath, std::string fileName, DataMode_t mode){
	this->reset();
	/*
	if(filePath.back() == '/')
	{
		this->_filePath = filePath;
	}else
	{
		this->_filePath = filePath + "/";
	}
	*/
	if(fileName != ""){	
		this->_fileName = fileName;
	}
	
	this->fileMode = mode;
	if(this->fileMode == READ_ONLY){ // else skip load and thus main will ask for them.
		printf("Loading all parameters from file %s\n", this->_fileName.c_str());
		this->loadParameters();
	}
} 
 
 void Parameters::reset(void){
	for(uint32_t index=0;index<MAX_PARAMETERS; index++){
		parameters[index].param_value=0;
		parameters[index].param_count=0;
		parameters[index].param_index=0;

		for(int a=0;a<16;a++){
			parameters[index].param_id[a]=0; 
		}

		parameters[index].param_type=0;
	}		 
	this->totalParameterCount=0;
	char buf[100];
	time_t now = time(0);
	// convert now to tm struct for UTC
	tm *gmtm = gmtime(&now);
	sprintf(buf,"%02d-%02d-%04d_%02d-%02d-%02d_Parameters.txt",gmtm->tm_mday,gmtm->tm_mon,gmtm->tm_year+1900,gmtm->tm_hour,gmtm->tm_min,gmtm->tm_sec);
	this->_fileName = buf;
 }
 
 void Parameters::addParameter(mavlink_message_t msg){
	 if(msg.msgid == MAVLINK_MSG_ID_PARAM_VALUE){
		mavlink_param_value_t parm;
		mavlink_msg_param_value_decode(&msg, &parm);
		this->addParameter(parm);
		
	 }
 }
 
 void Parameters::addParameter(mavlink_param_value_t msg){
		
	// set the local memory.
	if(msg.param_index<MAX_PARAMETERS){
		parameters[msg.param_index].param_value=msg.param_value;
		parameters[msg.param_index].param_count=msg.param_count;
		parameters[msg.param_index].param_index=msg.param_index;

		for(int a=0;a<16;a++){
			parameters[msg.param_index].param_id[a]=msg.param_id[a]; 
		}

		parameters[msg.param_index].param_type=msg.param_type;
		
		if(totalParameterCount==0){
			totalParameterCount=msg.param_count;
			printf("Total parameter count in system=%d\n",totalParameterCount);
		}
		
		if(this->getFirstMissingParameter() == -1){ // done with all parameters, thus save them
			printf("Saving parameters to file %s\n",this->_fileName.c_str());
			this->saveParameters();		
		}
	}else{
		printf("Index Error, requested=%d but is only=%d\n",msg.param_index,MAX_PARAMETERS);
	}		
 }

 mavlink_param_value_t Parameters::getParameterByID(uint16_t id){
	 mavlink_param_value_t msg;
	 if(id < MAX_PARAMETERS){
		 msg = parameters[id];
	 }else{
		 msg = {};
	 }
	 return msg;
 }

 int16_t Parameters::getFirstMissingParameter(void){
	 
	 if(totalParameterCount == 0){
		 return 0;
	 }
	 
	 for(int16_t index=0;index<totalParameterCount; index++){
//		 printf("Checking parameter #%d which has value %d\n",index, parameters[index].param_count);
		if(parameters[index].param_count == 0){ // 0 means not set
//			printf("Parameter count=%d found to be zero\n",index);
			return index;
		}
	 }
	 printf("All parameters are there!\n");	 
	 return -1; // none missing.
 }
 
 DataMode_t Parameters::getParameterMode(void){
	return this->fileMode;
 }


 int16_t Parameters::getMissingParameters(void){
//	printf("Missing parameters...");
	if(totalParameterCount == 0){
		return 0;
	}

	int16_t numberOfParametersSet = 0;
	
	for(int16_t index=0;index<totalParameterCount; index++){
		if(parameters[index].param_count != 0){ // anything else than 0 means set
			numberOfParametersSet++; 
		}else{ // 0 means not set
//			printf(" #%d", index);		
		}
	 }
//	 printf("\n");
//	 printf("Total parameters set:%d\n", numberOfParametersSet);
 	return totalParameterCount-numberOfParametersSet;
 }

 /*
 mavlink_message_t Parameters::getMavlinkParameterByID(uint16_t id){
	 mavlink_message_t msg;
	 if(id < MAX_PARAMETERS){
		 mavlink_msg_param_value_encode(1, 1, &msg, &this->parameters[id]); // SYS.ID=1, COMP.ID=1 (Reply like we where a Pixhawk :-) )
	 }else{
		 msg = {};
	 }
	 return msg;
 }
 */
 bool Parameters::compareParameters(mavlink_param_value_t *msg1, mavlink_param_value_t *msg2){
	return false; 
 }

void Parameters::loadParameters(void){
	
	/*
	typedef struct __mavlink_param_value_t {
	float param_value; <  Onboard parameter value
	uint16_t param_count; <  Total number of onboard parameters
	uint16_t param_index; <  Index of this onboard parameter
	char param_id[16]; <  Onboard parameter id, terminated by NULL if the length is less than 16 human-readable chars and WITHOUT null termination (NULL) byte if the length is exactly 16 chars - applications have to provide 16+1 bytes storage if the ID is stored as string
	uint8_t param_type; <  Onboard parameter type.
	}) mavlink_param_value_t;*/
	
	ifstream myfile(_fileName);
	if (myfile.is_open()) {
		string line;
			
		int parameterIndex=0;
		int parameterCount=0;
		parameterIndex=PARAM_VALUE;
		while (getline(myfile, line)) {
			
			switch(parameterIndex)
			{
				case PARAM_VALUE:
					parameters[parameterCount].param_value = (float)std::stof(line);
					parameterIndex=PARAM_COUNT;					
				break;
				
				case PARAM_COUNT:
					parameters[parameterCount].param_count = (uint16_t)std::stof(line);
					parameterIndex=PARAM_INDEX;
				break;				
				

				case PARAM_INDEX:
					parameters[parameterCount].param_index = (uint16_t)std::stof(line);				
					parameterIndex=PARAM_ID;
				break;			
				
				case PARAM_ID:				
					strcpy(parameters[parameterCount].param_id, line.c_str());
					parameterIndex=PARAM_TYPE;
				break;
				
				case PARAM_TYPE:
					parameters[parameterCount].param_type = (uint8_t)std::stof(line);					
					parameterCount++;
				parameterIndex=PARAM_VALUE;
				break;

				default: 	
				break;
			}	
		//	printf(":%s:\n", line.c_str());	
		}
		myfile.close();
		this->totalParameterCount = parameterCount;
		printf("Loading %d parameters completed!\n", this->totalParameterCount);		
	}else{
		
		printf("Open parameter file failed!\n");		
	}
	
	
}

void Parameters::saveParameters(void){
	
	printf("Saving all paraemters to file.\n");		
	FILE * SystemLogFile;
	SystemLogFile = fopen(_fileName.c_str(),"w");

	// loop and save the array:
	for(uint32_t index=0;index<this->totalParameterCount; index++){
		std::string floatString = to_string(parameters[index].param_value);	
		
		// print to file:
		fprintf(SystemLogFile,"%s\n",floatString.c_str());                   // String to file
		fprintf(SystemLogFile,"%d\n",parameters[index].param_count);         // String to file
		fprintf(SystemLogFile,"%d\n",parameters[index].param_index);         // String to file
		
		// Handle that param_id string (char array9 will not have any NULL termination if all 16 chars are used.
		char buf[20];
		bzero(&buf, sizeof(buf));		
		for(int a=0;a<17;a++){
			if(parameters[index].param_id[a]==0){
				break;
			}else{
				buf[a]=parameters[index].param_id[a];
			}
		}
		//fprintf(SystemLogFile,parameters[index].param_id);
		fprintf(SystemLogFile,"%s",buf);
		
		fprintf(SystemLogFile,"\n");
		fprintf(SystemLogFile,"%d\n",parameters[index].param_type);           // String to file
	}	
			
	fclose (SystemLogFile); // must close after opening
	printf("Save complete!\n");	
}

uint16_t Parameters::getNumberOfParameters(void){
	return this->totalParameterCount;
}

 
 

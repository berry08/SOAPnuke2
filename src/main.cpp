#include <iostream>
#include <string>
#include <fstream>

#include "process_argv.h"
#include "global_parameter.h"
#include "peprocess.h"
#include "seprocess.h"
int main(int argc,char* argv[]){
	C_global_variable gv;	//global variable, include statistic information
	C_global_parameter gp;	//global parameter generated by commands
	check_module(argc,argv);	//check module
	global_parameter_initial(argc,argv,gp);	//initial global parameter
	//C_global_parameter gp=C_global_parameter(argc,argv);
	check_parameter(argc,argv,gp);	//check global parameter whether correct
	if(!gp.fq2_path.empty()){	//PE data
		peProcess new_task(gp);
		if(gp.mode=="ssd"){	//ssd mode
			new_task.process();
		}else{	//non ssd mode
			new_task.process_nonssd();
		}
	}else{	//SE data
		seProcess new_task(gp);
		if(gp.mode=="ssd"){
			new_task.process();
		}else{
			new_task.process_nonssd();
		}
	}
	return 0;
}
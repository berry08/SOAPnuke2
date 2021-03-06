#include <iostream>
#include <string>
#include <fstream>

#include "process_argv.h"
#include "global_parameter.h"
#include "peprocess.h"
#include "seprocess.h"
#include <exception>
#ifdef _PROCESSHTS
#include "processHts.h"
#endif
#include "processStLFR.h"
//#include "mGzip.h"

//vector<threadDataInfo> C_global_parameter::threadInfo;
int main(int argc,char* argv[]){
	//C_global_variable gv;	//global variable, include statistic information
	C_global_parameter gp;	//global parameter generated by commands
	check_module(argc,argv);	//check module
	global_parameter_initial(argc,argv,gp);	//initial global parameter
	//C_global_parameter gp=C_global_parameter(argc,argv);
	check_parameter(argc,argv,gp);	//check global parameter whether correct
	//check input format whether mGzip
//    gp.whether_mGzip=mGzip::check_mGzip(gp.fq1_path);
//    if(gp.whether_mGzip){
//        vector<string> inputFqList;
//        if(gp.inputAsList){
//            ifstream inList(gp.fq1_path);
//            string line;
//            while(getline(inList,line)){
//                inputFqList.push_back(line);
//            }
//        }else{
//            inputFqList.push_back(gp.fq1_path);
//        }
//        C_global_parameter::threadInfo=mGzip::allocate(gp.threads_num,inputFqList);
//
//    }
	if(gp.module_name=="filterHts"){
        #ifdef _PROCESSHTS
	    processHts new_task(gp);
	    if(new_task.pe){
	        new_task.processPE();
	    }else{
	        new_task.processSE();
	    }
        #else
	    cout<<"Error:filterHts module not installed, please re-make after set USEHTS true in Makefile, details see Install part in Readme"<<endl;
	    exit(1);
	    #endif
//	    new_task.process();
	}else if(gp.module_name=="filterStLFR") {
        peProcess* new_task=new processStLFR(gp);
        new_task->process();
	}else {
        if (!gp.fq2_path.empty()) {    //PE data
            peProcess new_task(gp);
            new_task.process();
//            cout<<"dup number:\t"<<new_task.dupNum<<endl;
        } else {    //SE data
            seProcess new_task(gp);
            new_task.process();
//            cout<<"dup number:\t"<<new_task.dupNum<<endl;
        }
	}
	return 0;
}

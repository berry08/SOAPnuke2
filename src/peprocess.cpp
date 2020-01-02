#include <iostream>
#include <string>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <time.h>
#include <dirent.h>
#include <math.h>
#include <unistd.h>
#include "peprocess.h"
#include "process_argv.h"
#include "zlib.h"
#include "gc.h"
#include "sequence.h"
using namespace::std;
#define READBUF 500
#define random(x) (rand()%x)


peProcess::peProcess(C_global_parameter m_gp){ //initialize
	gp=m_gp;
	gv=C_global_variable();
	used_threads_num=0;
	srand((unsigned)time(NULL));
	ostringstream tmpstring;
	tmpstring<<rand()%100;
	limit_end=0;
	multi_gzfq1=new gzFile[gp.threads_num];
	multi_gzfq2=new gzFile[gp.threads_num];
    multi_Nongzfq1=new FILE*[gp.threads_num];
    multi_Nongzfq2=new FILE*[gp.threads_num];
	gz_trim_out1=new gzFile[gp.threads_num];
	gz_trim_out2=new gzFile[gp.threads_num];
	gz_clean_out1=new gzFile[gp.threads_num];
	gz_clean_out2=new gzFile[gp.threads_num];
	buffer=1024*1024*2;
	local_fs=new C_filter_stat[gp.threads_num];
	local_raw_stat1=new C_fastq_file_stat[gp.threads_num];
	local_raw_stat2=new C_fastq_file_stat[gp.threads_num];
	local_trim_stat1=new C_fastq_file_stat[gp.threads_num];
	local_trim_stat2=new C_fastq_file_stat[gp.threads_num];
	local_clean_stat1=new C_fastq_file_stat[gp.threads_num];
	local_clean_stat2=new C_fastq_file_stat[gp.threads_num];
	t_start_pos=new off_t[gp.threads_num];
	t_end_pos=new off_t[gp.threads_num];
	sticky_reads1=new string[gp.threads_num];
	sticky_reads2=new string[gp.threads_num];
	nongz_clean_out1=new FILE*[gp.threads_num];
	nongz_clean_out2=new FILE*[gp.threads_num];
	bq_check=0;
	pair_check=0;
    cur_cat_cycle=0;
    readyTrimFiles1=new vector<string>[gp.threads_num];
    readyTrimFiles2=new vector<string>[gp.threads_num];
    readyCleanFiles1=new vector<string>[gp.threads_num];
    readyCleanFiles2=new vector<string>[gp.threads_num];
    clean_file_readsNum=new vector<int>[gp.threads_num];
    nongz_trim_out1=new FILE*[gp.threads_num];
    nongz_trim_out2=new FILE*[gp.threads_num];
    sub_thread_done=new int[gp.threads_num];
    for(int i=0;i<gp.threads_num;i++){
        sub_thread_done[i]=0;
    }
    end_sub_thread=0;
    patch=160/gp.threads_num;
}
void peProcess::print_stat(){	//print statistic information to the file
	string filter_out=gp.output_dir+"/Statistics_of_Filtered_Reads.txt";
	string general_out=gp.output_dir+"/Basic_Statistics_of_Sequencing_Quality.txt";
	string bs1_out=gp.output_dir+"/Base_distributions_by_read_position_1.txt";
	string bs2_out=gp.output_dir+"/Base_distributions_by_read_position_2.txt";
	string qs1_out=gp.output_dir+"/Base_quality_value_distribution_by_read_position_1.txt";
	string qs2_out=gp.output_dir+"/Base_quality_value_distribution_by_read_position_2.txt";
	string q20_out1=gp.output_dir+"/Distribution_of_Q20_Q30_bases_by_read_position_1.txt";
	string q20_out2=gp.output_dir+"/Distribution_of_Q20_Q30_bases_by_read_position_2.txt";
	string trim_stat1=gp.output_dir+"/Statistics_of_Trimming_Position_of_Reads_1.txt";
	string trim_stat2=gp.output_dir+"/Statistics_of_Trimming_Position_of_Reads_2.txt";
	ofstream of_filter_stat(filter_out.c_str());
	ofstream of_general_stat(general_out.c_str());
	ofstream of_readPos_base_stat1(bs1_out.c_str());
	ofstream of_readPos_base_stat2(bs2_out.c_str());
	ofstream of_readPos_qual_stat1(qs1_out.c_str());
	ofstream of_readPos_qual_stat2(qs2_out.c_str());
	ofstream of_q2030_stat1(q20_out1.c_str());
	ofstream of_q2030_stat2(q20_out2.c_str());
	ofstream of_trim_stat1(trim_stat1.c_str());
	ofstream of_trim_stat2(trim_stat2.c_str());
	if(!of_filter_stat){
		cerr<<"Error:cannot open such file,"<<filter_out<<endl;
		exit(1);
	}
	if(!of_general_stat){
		cerr<<"Error:cannot open such file,"<<general_out<<endl;
		exit(1);
	}
	if(!of_readPos_base_stat1 || !of_readPos_base_stat2){
		cerr<<"Error:cannot open such file,Base_distributions_by_read_position*.txt"<<endl;
		exit(1);
	}
	if(!of_readPos_qual_stat1 || !of_readPos_qual_stat2){
		cerr<<"Error:cannot open such file,Base_quality_value_distribution_by_read_position*.txt"<<endl;
		exit(1);
	}
	if(!of_q2030_stat1 || !of_q2030_stat2){
		cerr<<"Error:cannot open such file,Distribution_of_Q20_Q30_bases_by_read_position*.txt"<<endl;
		exit(1);
	}
	of_filter_stat<<"Item\t\t\t\tTotal\tPercentage\tfastq1\tfastq2\toverlap"<<endl;
	vector<string> filter_items;
	filter_items.emplace_back("Reads limited to output number");
	filter_items.emplace_back("Reads with filtered tile");
	filter_items.emplace_back("Reads with filtered fov");
	filter_items.emplace_back("Reads too short");
	filter_items.emplace_back("Reads too long");
	filter_items.emplace_back("Reads with global contam sequence");
	filter_items.emplace_back("Reads with contam sequence");
	filter_items.emplace_back("Reads with n rate exceed");
	filter_items.emplace_back("Reads with highA");
	filter_items.emplace_back("Reads with polyX");
	filter_items.emplace_back("Reads with low quality");
	filter_items.emplace_back("Reads with low mean quality");
	filter_items.emplace_back("Reads with small insert size");
	filter_items.emplace_back("Reads with adapter");

	map<string,long long> filter_number,filter_pe1,filter_pe2,filter_overlap;
	filter_number["Reads with global contam sequence"]=gv.fs.include_global_contam_seq_num;
	filter_number["Reads with contam sequence"]=gv.fs.include_contam_seq_num;
	filter_number["Reads too short"]=gv.fs.short_len_num;
	filter_number["Reads with adapter"]=gv.fs.include_adapter_seq_num;
	filter_number["Reads with low quality"]=gv.fs.low_qual_base_ratio_num;
	filter_number["Reads with low mean quality"]=gv.fs.mean_quality_num;
	filter_number["Reads with n rate exceed"]=gv.fs.n_ratio_num;
	filter_number["Reads with small insert size"]=gv.fs.over_lapped_num;
	filter_number["Reads with highA"]=gv.fs.highA_num;
	filter_number["Reads with polyX"]=gv.fs.polyX_num;
	filter_number["Reads with filtered tile"]=gv.fs.tile_num;
	filter_number["Reads with filtered fov"]=gv.fs.fov_num;
	filter_number["Reads too long"]=gv.fs.long_len_num;
	//filter_number["Reads limited to output number"]=gv.fs.output_reads_num;

	filter_pe1["Reads with contam sequence"]=gv.fs.include_contam_seq_num1;
	filter_pe1["Reads with global contam sequence"]=gv.fs.include_global_contam_seq_num1;
	filter_pe1["Reads too short"]=gv.fs.short_len_num1;
	filter_pe1["Reads with adapter"]=gv.fs.include_adapter_seq_num1;
	filter_pe1["Reads with low quality"]=gv.fs.low_qual_base_ratio_num1;
	filter_pe1["Reads with low mean quality"]=gv.fs.mean_quality_num1;
	filter_pe1["Reads with n rate exceed"]=gv.fs.n_ratio_num1;
	filter_pe1["Reads with small insert size"]=gv.fs.over_lapped_num;
	filter_pe1["Reads with highA"]=gv.fs.highA_num1;
	filter_pe1["Reads with polyX"]=gv.fs.polyX_num1;
	filter_pe1["Reads with filtered tile"]=gv.fs.tile_num;
	filter_pe1["Reads with filtered fov"]=gv.fs.fov_num;
	filter_pe1["Reads too long"]=gv.fs.long_len_num1;
	//filter_pe1["Reads limited to output number"]=gv.fs.output_reads_num;

	filter_pe2["Reads with contam sequence"]=gv.fs.include_contam_seq_num2;
	filter_pe2["Reads with global contam sequence"]=gv.fs.include_global_contam_seq_num2;
	filter_pe2["Reads too short"]=gv.fs.short_len_num2;
	filter_pe2["Reads with adapter"]=gv.fs.include_adapter_seq_num2;
	filter_pe2["Reads with low quality"]=gv.fs.low_qual_base_ratio_num2;
	filter_pe2["Reads with low mean quality"]=gv.fs.mean_quality_num2;
	filter_pe2["Reads with n rate exceed"]=gv.fs.n_ratio_num2;
	filter_pe2["Reads with small insert size"]=gv.fs.over_lapped_num;
	filter_pe2["Reads with highA"]=gv.fs.highA_num2;
	filter_pe2["Reads with polyX"]=gv.fs.polyX_num2;
	filter_pe2["Reads with filtered tile"]=gv.fs.tile_num;
	filter_pe2["Reads with filtered fov"]=gv.fs.fov_num;
	filter_pe2["Reads too long"]=gv.fs.long_len_num2;
	//filter_pe2["Reads limited to output number"]=gv.fs.output_reads_num;

	filter_overlap["Reads with contam sequence"]=gv.fs.include_contam_seq_num_overlap;
	filter_overlap["Reads with global contam sequence"]=gv.fs.include_global_contam_seq_num_overlap;
	filter_overlap["Reads too short"]=gv.fs.short_len_num_overlap;
	filter_overlap["Reads with adapter"]=gv.fs.include_adapter_seq_num_overlap;
	filter_overlap["Reads with low quality"]=gv.fs.low_qual_base_ratio_num_overlap;
	filter_overlap["Reads with low mean quality"]=gv.fs.mean_quality_num_overlap;
	filter_overlap["Reads with n rate exceed"]=gv.fs.n_ratio_num_overlap;
	filter_overlap["Reads with small insert size"]=gv.fs.over_lapped_num;
	filter_overlap["Reads with highA"]=gv.fs.highA_num_overlap;
	filter_overlap["Reads with polyX"]=gv.fs.polyX_num_overlap;
	filter_overlap["Reads with filtered tile"]=gv.fs.tile_num;
	filter_overlap["Reads with filtered fov"]=gv.fs.fov_num;
	filter_overlap["Reads too long"]=gv.fs.long_len_num_overlap;
	//filter_overlap["Reads limited to output number"]=gv.fs.output_reads_num;
	long long total_filter_fq1_num=0;
	for(map<string,long long>::iterator ix=filter_number.begin();ix!=filter_number.end();ix++){
		total_filter_fq1_num+=ix->second;
	}
	of_filter_stat<<setiosflags(ios::fixed);
	of_filter_stat<<"Total filtered read pair number\t"<<total_filter_fq1_num<<"\t100.00%\t\t"<<total_filter_fq1_num<<"\t"<<total_filter_fq1_num<<"\t"<<total_filter_fq1_num<<endl;
	for(vector<string>::iterator ix=filter_items.begin();ix!=filter_items.end();ix++){
		if(filter_number[*ix]>0){
			of_filter_stat<<*ix<<"\t"<<filter_number[*ix]<<"\t";
			of_filter_stat<<setprecision(2)<<100*(float)filter_number[*ix]/total_filter_fq1_num<<"%\t";
			of_filter_stat<<filter_pe1[*ix]<<"\t"<<filter_pe2[*ix]<<"\t"<<filter_overlap[*ix]<<endl;
		}
	}

	of_general_stat<<"Item\traw reads(fq1)\tclean reads(fq1)\traw reads(fq2)\tclean reads(fq2)"<<endl;
	float raw1_rl(0),raw2_rl(0),clean1_rl(0),clean2_rl(0);
	char filter_r1_ratio[100],filter_r2_ratio[100];
	char raw_r1[7][100];
	char clean_r1[7][100];
	char raw_r2[7][100];
	char clean_r2[7][100];
	/*for(int i=0;i!=7;i++){
		raw_r1[i]={0};
		clean_r1[i]={0};
		raw_r2[i]={0};
		clean_r2[i]={0};
	}*/
	if(gv.raw1_stat.gs.reads_number!=0){
		raw1_rl=1.0*gv.raw1_stat.gs.base_number/gv.raw1_stat.gs.reads_number;
		sprintf(filter_r1_ratio,"%.2f",100*(float)total_filter_fq1_num/gv.raw1_stat.gs.reads_number);
		sprintf(raw_r1[0],"%.2f",100*(float)gv.raw1_stat.gs.a_number/gv.raw1_stat.gs.base_number);
		sprintf(raw_r1[1],"%.2f",100*(float)gv.raw1_stat.gs.c_number/gv.raw1_stat.gs.base_number);
		sprintf(raw_r1[2],"%.2f",100*(float)gv.raw1_stat.gs.g_number/gv.raw1_stat.gs.base_number);
		sprintf(raw_r1[3],"%.2f",100*(float)gv.raw1_stat.gs.t_number/gv.raw1_stat.gs.base_number);
		sprintf(raw_r1[4],"%.2f",100*(float)gv.raw1_stat.gs.n_number/gv.raw1_stat.gs.base_number);
		sprintf(raw_r1[5],"%.2f",100*(float)gv.raw1_stat.gs.q20_num/gv.raw1_stat.gs.base_number);
		sprintf(raw_r1[6],"%.2f",100*(float)gv.raw1_stat.gs.q30_num/gv.raw1_stat.gs.base_number);
	}
	if(gv.clean1_stat.gs.reads_number!=0){
		clean1_rl=1.0*gv.clean1_stat.gs.base_number/gv.clean1_stat.gs.reads_number;
		sprintf(clean_r1[0],"%.2f",100*(float)gv.clean1_stat.gs.a_number/gv.clean1_stat.gs.base_number);
		sprintf(clean_r1[1],"%.2f",100*(float)gv.clean1_stat.gs.c_number/gv.clean1_stat.gs.base_number);
		sprintf(clean_r1[2],"%.2f",100*(float)gv.clean1_stat.gs.g_number/gv.clean1_stat.gs.base_number);
		sprintf(clean_r1[3],"%.2f",100*(float)gv.clean1_stat.gs.t_number/gv.clean1_stat.gs.base_number);
		sprintf(clean_r1[4],"%.2f",100*(float)gv.clean1_stat.gs.n_number/gv.clean1_stat.gs.base_number);
		sprintf(clean_r1[5],"%.2f",100*(float)gv.clean1_stat.gs.q20_num/gv.clean1_stat.gs.base_number);
		sprintf(clean_r1[6],"%.2f",100*(float)gv.clean1_stat.gs.q30_num/gv.clean1_stat.gs.base_number);
	}
	if(gv.raw2_stat.gs.reads_number!=0){
		raw2_rl=1.0*gv.raw2_stat.gs.base_number/gv.raw2_stat.gs.reads_number;
		sprintf(filter_r2_ratio,"%.2f",100*(float)total_filter_fq1_num/gv.raw2_stat.gs.reads_number);
		sprintf(raw_r2[0],"%.2f",100*(float)gv.raw2_stat.gs.a_number/gv.raw2_stat.gs.base_number);
		sprintf(raw_r2[1],"%.2f",100*(float)gv.raw2_stat.gs.c_number/gv.raw2_stat.gs.base_number);
		sprintf(raw_r2[2],"%.2f",100*(float)gv.raw2_stat.gs.g_number/gv.raw2_stat.gs.base_number);
		sprintf(raw_r2[3],"%.2f",100*(float)gv.raw2_stat.gs.t_number/gv.raw2_stat.gs.base_number);
		sprintf(raw_r2[4],"%.2f",100*(float)gv.raw2_stat.gs.n_number/gv.raw2_stat.gs.base_number);
		sprintf(raw_r2[5],"%.2f",100*(float)gv.raw2_stat.gs.q20_num/gv.raw2_stat.gs.base_number);
		sprintf(raw_r2[6],"%.2f",100*(float)gv.raw2_stat.gs.q30_num/gv.raw2_stat.gs.base_number);
	}
	if(gv.clean2_stat.gs.reads_number!=0){
		clean2_rl=1.0*gv.clean2_stat.gs.base_number/gv.clean2_stat.gs.reads_number;
		sprintf(clean_r2[0],"%.2f",100*(float)gv.clean2_stat.gs.a_number/gv.clean2_stat.gs.base_number);
		sprintf(clean_r2[1],"%.2f",100*(float)gv.clean2_stat.gs.c_number/gv.clean2_stat.gs.base_number);
		sprintf(clean_r2[2],"%.2f",100*(float)gv.clean2_stat.gs.g_number/gv.clean2_stat.gs.base_number);
		sprintf(clean_r2[3],"%.2f",100*(float)gv.clean2_stat.gs.t_number/gv.clean2_stat.gs.base_number);
		sprintf(clean_r2[4],"%.2f",100*(float)gv.clean2_stat.gs.n_number/gv.clean2_stat.gs.base_number);
		sprintf(clean_r2[5],"%.2f",100*(float)gv.clean2_stat.gs.q20_num/gv.clean2_stat.gs.base_number);
		sprintf(clean_r2[6],"%.2f",100*(float)gv.clean2_stat.gs.q30_num/gv.clean2_stat.gs.base_number);
	}
	of_general_stat<<setiosflags(ios::fixed)<<setprecision(1)<<"Read length\t"<<raw1_rl<<"\t"<<clean1_rl<<"\t"<<raw2_rl<<"\t"<<clean2_rl<<endl;
	of_general_stat<<"Total number of reads\t"<<setprecision(15)<<gv.raw1_stat.gs.reads_number<<" (100.00%)\t"<<gv.clean1_stat.gs.reads_number<<" (100.00%)\t"<<gv.raw2_stat.gs.reads_number<<" (100.00%)\t"<<gv.clean2_stat.gs.reads_number<<" (100.00%)"<<endl;
	of_general_stat<<"Number of filtered reads\t"<<total_filter_fq1_num<<" ("<<filter_r1_ratio<<"%)\t-\t"<<total_filter_fq1_num<<" ("<<filter_r2_ratio<<"%)\t-"<<endl;
	of_general_stat<<"Total number of bases\t"<<setprecision(15)<<gv.raw1_stat.gs.base_number<<" (100.00%)\t"<<gv.clean1_stat.gs.base_number<<" (100.00%)\t"<<gv.raw2_stat.gs.base_number<<" (100.00%)\t"<<gv.clean2_stat.gs.base_number<<" (100.00%)"<<endl;
	long long filter_base1=total_filter_fq1_num*gv.raw1_stat.gs.read_length;
	long long filter_base2=total_filter_fq1_num*gv.raw1_stat.gs.read_length;
	of_general_stat<<"Number of filtered bases\t"<<setprecision(15)<<filter_base1<<" ("<<filter_r1_ratio<<"%)\t-\t"<<filter_base2<<" ("<<filter_r2_ratio<<"%)\t-"<<endl;
	of_general_stat<<"Number of base A\t"<<setprecision(15)<<gv.raw1_stat.gs.a_number<<" ("<<raw_r1[0]<<"%)\t"<<gv.clean1_stat.gs.a_number<<" ("<<clean_r1[0]<<"%)\t"<<gv.raw2_stat.gs.a_number<<" ("<<raw_r2[0]<<"%)\t"<<gv.clean2_stat.gs.a_number<<" ("<<clean_r2[0]<<"%)"<<endl;
	of_general_stat<<"Number of base C\t"<<setprecision(15)<<gv.raw1_stat.gs.c_number<<" ("<<raw_r1[1]<<"%)\t"<<gv.clean1_stat.gs.c_number<<" ("<<clean_r1[1]<<"%)\t"<<gv.raw2_stat.gs.c_number<<" ("<<raw_r2[1]<<"%)\t"<<gv.clean2_stat.gs.c_number<<" ("<<clean_r2[1]<<"%)"<<endl;
	of_general_stat<<"Number of base G\t"<<setprecision(15)<<gv.raw1_stat.gs.g_number<<" ("<<raw_r1[2]<<"%)\t"<<gv.clean1_stat.gs.g_number<<" ("<<clean_r1[2]<<"%)\t"<<gv.raw2_stat.gs.g_number<<" ("<<raw_r2[2]<<"%)\t"<<gv.clean2_stat.gs.g_number<<" ("<<clean_r2[2]<<"%)"<<endl;
	of_general_stat<<"Number of base T\t"<<setprecision(15)<<gv.raw1_stat.gs.t_number<<" ("<<raw_r1[3]<<"%)\t"<<gv.clean1_stat.gs.t_number<<" ("<<clean_r1[3]<<"%)\t"<<gv.raw2_stat.gs.t_number<<" ("<<raw_r2[3]<<"%)\t"<<gv.clean2_stat.gs.t_number<<" ("<<clean_r2[3]<<"%)"<<endl;
	of_general_stat<<"Number of base N\t"<<setprecision(15)<<gv.raw1_stat.gs.n_number<<" ("<<raw_r1[4]<<"%)\t"<<gv.clean1_stat.gs.n_number<<" ("<<clean_r1[4]<<"%)\t"<<gv.raw2_stat.gs.n_number<<" ("<<raw_r2[4]<<"%)\t"<<gv.clean2_stat.gs.n_number<<" ("<<clean_r2[4]<<"%)"<<endl;
	of_general_stat<<"Q20 number\t"<<setprecision(15)<<gv.raw1_stat.gs.q20_num<<" ("<<raw_r1[5]<<"%)\t"<<gv.clean1_stat.gs.q20_num<<" ("<<clean_r1[5]<<"%)\t"<<gv.raw2_stat.gs.q20_num<<" ("<<raw_r2[5]<<"%)\t"<<gv.clean2_stat.gs.q20_num<<" ("<<clean_r2[5]<<"%)"<<endl;
	/*
	float clean1_q20_ratio(0),clean2_q20_ratio(0);
	if(gv.clean1_stat.gs.base_number!=0)
		clean1_q20_ratio=(float)gv.clean1_stat.gs.q20_num/gv.clean1_stat.gs.base_number;
	if(gv.clean2_stat.gs.base_number!=0)
		clean2_q20_ratio=(float)gv.clean2_stat.gs.q20_num/gv.clean2_stat.gs.base_number;
	of_general_stat<<"Q20 ratio\t"<<setprecision(4)<<(float)gv.raw1_stat.gs.q20_num/gv.raw1_stat.gs.base_number<<"\t"<<clean1_q20_ratio<<"\t"<<(float)gv.raw2_stat.gs.q20_num/gv.raw2_stat.gs.base_number<<"\t"<<clean2_q20_ratio<<endl;
	*/
	of_general_stat<<"Q30 number\t"<<setprecision(15)<<gv.raw1_stat.gs.q30_num<<" ("<<raw_r1[6]<<"%)\t"<<gv.clean1_stat.gs.q30_num<<" ("<<clean_r1[6]<<"%)\t"<<gv.raw2_stat.gs.q30_num<<" ("<<raw_r2[6]<<"%)\t"<<gv.clean2_stat.gs.q30_num<<" ("<<clean_r2[6]<<"%)"<<endl;
	/*
	float clean1_q30_ratio(0),clean2_q30_ratio(0);
	if(gv.clean1_stat.gs.base_number!=0)
		clean1_q30_ratio=(float)gv.clean1_stat.gs.q30_num/gv.clean1_stat.gs.base_number;
	if(gv.clean2_stat.gs.base_number!=0)
		clean2_q30_ratio=(float)gv.clean2_stat.gs.q30_num/gv.clean2_stat.gs.base_number;
	//of_general_stat<<"Q30 ratio\t"<<setprecision(4)<<(float)gv.raw1_stat.gs.q30_num/gv.raw1_stat.gs.base_number<<"\t"<<clean1_q30_ratio<<"\t"<<(float)gv.raw2_stat.gs.q30_num/gv.raw2_stat.gs.base_number<<"\t"<<clean2_q30_ratio<<endl;
	*/
	of_general_stat.close();
	of_readPos_base_stat1<<"Pos\tA\tC\tG\tT\tN\tclean A\tclean C\tclean G\tclean T\tclean N"<<endl;
	of_readPos_base_stat2<<"Pos\tA\tC\tG\tT\tN\tclean A\tclean C\tclean G\tclean T\tclean N"<<endl;
	for(int i=0;i<gv.raw1_stat.gs.read_length;i++){
		of_readPos_base_stat1<<i+1<<"\t";
		of_readPos_base_stat2<<i+1<<"\t";
		string base_set="ACGTN";
		float raw1_cur_pos_total_base=0;
		float clean1_cur_pos_total_base=0;
		float raw2_cur_pos_total_base=0;
		float clean2_cur_pos_total_base=0;
		for(int j=0;j!=base_set.size();j++){
			raw1_cur_pos_total_base+=gv.raw1_stat.bs.position_acgt_content[i][j];
			raw2_cur_pos_total_base+=gv.raw2_stat.bs.position_acgt_content[i][j];
			clean1_cur_pos_total_base+=gv.clean1_stat.bs.position_acgt_content[i][j];
			clean2_cur_pos_total_base+=gv.clean2_stat.bs.position_acgt_content[i][j];
		}
		for(int j=0;j!=base_set.size();j++){
			of_readPos_base_stat1<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.raw1_stat.bs.position_acgt_content[i][j]/raw1_cur_pos_total_base<<"%\t";
		}
		for(int j=0;j!=base_set.size();j++){
			of_readPos_base_stat1<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.clean1_stat.bs.position_acgt_content[i][j]/clean1_cur_pos_total_base<<"%";
			if(base_set[j]!='N'){
				of_readPos_base_stat1<<"\t";
			}else{
				of_readPos_base_stat1<<endl;
			}
		}
		for(int j=0;j!=base_set.size();j++){
			of_readPos_base_stat2<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.raw2_stat.bs.position_acgt_content[i][j]/raw2_cur_pos_total_base<<"%\t";
		}
		for(int j=0;j!=base_set.size();j++){
			of_readPos_base_stat2<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.clean2_stat.bs.position_acgt_content[i][j]/clean2_cur_pos_total_base<<"%";
			if(base_set[j]!='N'){
				of_readPos_base_stat2<<"\t";
			}else{
				of_readPos_base_stat2<<endl;
			}
		}
	}
	of_readPos_base_stat1.close();
	of_readPos_base_stat2.close();

	of_readPos_qual_stat1<<"#raw fastq1 quality distribution"<<endl;
	of_readPos_qual_stat2<<"#raw fastq2 quality distribution"<<endl;
	of_readPos_qual_stat1<<"Pos\t";
	of_readPos_qual_stat2<<"Pos\t";
	int max_qual=0;
	for(int i=0;i<gv.raw1_stat.gs.read_length;i++){
		for(int j=1;j<=MAX_QUAL;j++){
			if(gv.raw1_stat.qs.position_qual[i][j]>0){
				max_qual=max_qual>j?max_qual:j;
			}
		}
	}
	for(int i=0;i<=max_qual;i++){
		of_readPos_qual_stat1<<"Q"<<i<<"\t";
		of_readPos_qual_stat2<<"Q"<<i<<"\t";
	}
	of_readPos_qual_stat1<<"Mean\tMedian\tLower quartile\tUpper quartile\t10th percentile\t90th percentile"<<endl;
	of_readPos_qual_stat2<<"Mean\tMedian\tLower quartile\tUpper quartile\t10th percentile\t90th percentile"<<endl;
	float raw1_q20[gv.raw1_stat.gs.read_max_length],raw1_q30[gv.raw1_stat.gs.read_max_length];
	float raw2_q20[gv.raw2_stat.gs.read_max_length],raw2_q30[gv.raw2_stat.gs.read_max_length];
	float clean1_q20[gv.raw1_stat.gs.read_max_length],clean1_q30[gv.raw1_stat.gs.read_max_length];
	float clean2_q20[gv.raw2_stat.gs.read_max_length],clean2_q30[gv.raw2_stat.gs.read_max_length];
	for(int i=0;i!=gv.raw1_stat.gs.read_length;i++){
		of_readPos_qual_stat1<<i+1<<"\t";
		of_readPos_qual_stat2<<i+1<<"\t";
		long long raw1_q20_num(0),raw1_q30_num(0),raw1_total(0);
		long long raw2_q20_num(0),raw2_q30_num(0),raw2_total(0);
		//int pos_max_qual1(0),pos_max_qual2(0);
		for(int j=0;j<=max_qual;j++){
			if(j>=20){
				raw1_q20_num+=gv.raw1_stat.qs.position_qual[i][j];
				raw2_q20_num+=gv.raw2_stat.qs.position_qual[i][j];
			}
			if(j>=30){
				raw1_q30_num+=gv.raw1_stat.qs.position_qual[i][j];
				raw2_q30_num+=gv.raw2_stat.qs.position_qual[i][j];
			}
			raw1_total+=gv.raw1_stat.qs.position_qual[i][j];
			of_readPos_qual_stat1<<setiosflags(ios::fixed);
			of_readPos_qual_stat1<<setprecision(0)<<gv.raw1_stat.qs.position_qual[i][j]<<"\t";
			raw2_total+=gv.raw2_stat.qs.position_qual[i][j];
			of_readPos_qual_stat2<<setiosflags(ios::fixed);
			of_readPos_qual_stat2<<setprecision(0)<<gv.raw2_stat.qs.position_qual[i][j]<<"\t";
		}
		//cout<<max_qual<<"\t"<<pos_max_qual1<<"\t"<<pos_max_qual2<<endl;
		//exit(1);
		raw1_q20[i]=(float)raw1_q20_num/raw1_total;
		raw1_q30[i]=(float)raw1_q30_num/raw1_total;
		raw2_q20[i]=(float)raw2_q20_num/raw2_total;
		raw2_q30[i]=(float)raw2_q30_num/raw2_total;
		quartile_result raw1_quar=cal_quar_from_array(gv.raw1_stat.qs.position_qual[i],max_qual);
		quartile_result raw2_quar=cal_quar_from_array(gv.raw2_stat.qs.position_qual[i],max_qual);//lower_quar,upper_quar
		of_readPos_qual_stat1<<setiosflags(ios::fixed)<<setprecision(2)<<raw1_quar.mean<<"\t";
		of_readPos_qual_stat1<<setprecision(0)<<raw1_quar.median<<"\t"<<raw1_quar.lower_quar<<"\t"<<raw1_quar.upper_quar<<"\t"<<raw1_quar.first10_quar<<"\t"<<raw1_quar.last10_quar<<endl;
		of_readPos_qual_stat2<<setiosflags(ios::fixed)<<setprecision(2)<<raw2_quar.mean<<"\t";
		of_readPos_qual_stat2<<setprecision(0)<<raw2_quar.median<<"\t"<<raw2_quar.lower_quar<<"\t"<<raw2_quar.upper_quar<<"\t"<<raw2_quar.first10_quar<<"\t"<<raw2_quar.last10_quar<<endl;
	}
	of_readPos_qual_stat1<<"#clean fastq1 quality distribution"<<endl;
	of_readPos_qual_stat2<<"#clean fastq2 quality distribution"<<endl;
	of_readPos_qual_stat1<<"Pos\t";
	of_readPos_qual_stat2<<"Pos\t";
	for(int i=0;i<=max_qual;i++){
		of_readPos_qual_stat1<<"Q"<<i<<"\t";
		of_readPos_qual_stat2<<"Q"<<i<<"\t";
	}
	of_readPos_qual_stat1<<"Mean\tMedian\tLower quartile\tUpper quartile\t10th percentile\t90th percentile"<<endl;
	of_readPos_qual_stat2<<"Mean\tMedian\tLower quartile\tUpper quartile\t10th percentile\t90th percentile"<<endl;

	of_q2030_stat1<<"Position in reads\tPercentage of Q20+ bases\tPercentage of Q30+ bases\tPercentage of Clean Q20+\tPercentage of Clean Q30+"<<endl;
	of_q2030_stat2<<"Position in reads\tPercentage of Q20+ bases\tPercentage of Q30+ bases\tPercentage of Clean Q20+\tPercentage of Clean Q30+"<<endl;
	for(int i=0;i!=gv.clean1_stat.gs.read_max_length;i++){
		of_readPos_qual_stat1<<i+1<<"\t";
		of_readPos_qual_stat2<<i+1<<"\t";
		long long clean1_q20_num(0),clean1_q30_num(0),clean1_total(0);
		long long clean2_q20_num(0),clean2_q30_num(0),clean2_total(0);
		for(int j=0;j<=max_qual;j++){
			if(j>=20){
				clean1_q20_num+=gv.clean1_stat.qs.position_qual[i][j];
				clean2_q20_num+=gv.clean2_stat.qs.position_qual[i][j];
			}
			if(j>=30){
				clean1_q30_num+=gv.clean1_stat.qs.position_qual[i][j];
				clean2_q30_num+=gv.clean2_stat.qs.position_qual[i][j];
			}
			of_readPos_qual_stat1<<setiosflags(ios::fixed);
			clean1_total+=gv.clean1_stat.qs.position_qual[i][j];
			of_readPos_qual_stat1<<setprecision(0)<<gv.clean1_stat.qs.position_qual[i][j]<<"\t";
			clean2_total+=gv.clean2_stat.qs.position_qual[i][j];
			of_readPos_qual_stat2<<setiosflags(ios::fixed);
			of_readPos_qual_stat2<<setprecision(0)<<gv.clean2_stat.qs.position_qual[i][j]<<"\t";
		}
		clean1_q20[i]=(float)clean1_q20_num/clean1_total;
		clean1_q30[i]=(float)clean1_q30_num/clean1_total;
		clean2_q20[i]=(float)clean2_q20_num/clean2_total;
		clean2_q30[i]=(float)clean2_q30_num/clean2_total;
		//cout<<i<<"\t"<<max_qual<<"\tmax qual3\t"<<max_qual-1<<"\t"<<max_qual+1<<"\t"<<max_qual+2<<endl;
		quartile_result clean1_quar=cal_quar_from_array(gv.clean1_stat.qs.position_qual[i],max_qual);
		quartile_result clean2_quar=cal_quar_from_array(gv.clean2_stat.qs.position_qual[i],max_qual);//lower_quar,upper_quar
		of_readPos_qual_stat1<<setiosflags(ios::fixed)<<setprecision(2)<<clean1_quar.mean<<"\t";
		of_readPos_qual_stat1<<setprecision(0)<<clean1_quar.median<<"\t"<<clean1_quar.lower_quar<<"\t"<<clean1_quar.upper_quar<<"\t"<<clean1_quar.first10_quar<<"\t"<<clean1_quar.last10_quar<<endl;
		of_readPos_qual_stat2<<setiosflags(ios::fixed)<<setprecision(2)<<clean2_quar.mean<<"\t";
		of_readPos_qual_stat2<<setprecision(0)<<clean2_quar.median<<"\t"<<clean2_quar.lower_quar<<"\t"<<clean2_quar.upper_quar<<"\t"<<clean2_quar.first10_quar<<"\t"<<clean2_quar.last10_quar<<endl;
		of_q2030_stat1<<i+1<<setiosflags(ios::fixed)<<setprecision(2)<<"\t"<<100*raw1_q20[i]<<"%\t"<<100*raw1_q30[i]<<"%\t"<<100*clean1_q20[i]<<"%\t"<<100*clean1_q30[i]<<"%"<<endl;
		of_q2030_stat2<<i+1<<setiosflags(ios::fixed)<<setprecision(2)<<"\t"<<100*raw2_q20[i]<<"%\t"<<100*raw2_q30[i]<<"%\t"<<100*clean2_q20[i]<<"%\t"<<100*clean2_q30[i]<<"%"<<endl;
	}
	of_readPos_qual_stat1.close();
	of_readPos_qual_stat2.close();
	of_q2030_stat1.close();
	of_q2030_stat2.close();
	of_trim_stat1<<"Pos\tHeadLowQual\tHeadFixLen\tTailAdapter\tTailLowQual\tTailFixLen\tCleanHeadLowQual\tCleanHeadFixLen\tCleanTailAdapter\tCleanTailLowQual\tCleanTailFixLen"<<endl;
	of_trim_stat2<<"Pos\tHeadLowQual\tHeadFixLen\tTailAdapter\tTailLowQual\tTailFixLen\tCleanHeadLowQual\tCleanHeadFixLen\tCleanTailAdapter\tCleanTailLowQual\tCleanTailFixLen"<<endl;
	long long head_total1(0),tail_total1(0),head_total2(0),tail_total2(0);
	long long head_total_clean1(0),tail_total_clean1(0),head_total_clean2(0),tail_total_clean2(0);
	for(int i=0;i<gv.raw1_stat.gs.read_length;i++){
		head_total1+=gv.raw1_stat.ts.ht[i]+gv.raw1_stat.ts.hlq[i];
		tail_total1+=gv.raw1_stat.ts.ta[i]+gv.raw1_stat.ts.tlq[i]+gv.raw1_stat.ts.tt[i];
		head_total2+=gv.raw2_stat.ts.ht[i]+gv.raw2_stat.ts.hlq[i];
		tail_total2+=gv.raw2_stat.ts.ta[i]+gv.raw2_stat.ts.tlq[i]+gv.raw2_stat.ts.tt[i];
		head_total_clean1+=gv.clean1_stat.ts.ht[i]+gv.clean1_stat.ts.hlq[i];
		tail_total_clean1+=gv.clean1_stat.ts.ta[i]+gv.clean1_stat.ts.tlq[i]+gv.clean1_stat.ts.tt[i];
		head_total_clean2+=gv.clean2_stat.ts.ht[i]+gv.clean2_stat.ts.hlq[i];
		tail_total_clean2+=gv.clean2_stat.ts.ta[i]+gv.clean2_stat.ts.tlq[i]+gv.clean2_stat.ts.tt[i];
		//of_trim_stat1<<i+1<<"\t"<<gv.trim1_stat.ts.hlq[i]<<"\t"<<gv.trim1_stat.ts.ht[i]<<"\t"<<gv.trim1_stat.ts.ta[i]
	}
	//cout<<head_total_clean1<<"\t"<<tail_total_clean1<<"\t"<<head_total_clean2<<"\t"<<tail_total_clean2<<endl;
	for(int i=1;i<=gv.raw1_stat.gs.read_length;i++){
		of_trim_stat1<<i<<"\t";
		if(head_total1>0){
			of_trim_stat1<<gv.raw1_stat.ts.hlq[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.raw1_stat.ts.hlq[i]/head_total1<<"%\t";
			of_trim_stat1<<gv.raw1_stat.ts.ht[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.raw1_stat.ts.ht[i]/head_total1<<"%\t";
		}else{
			of_trim_stat1<<gv.raw1_stat.ts.hlq[i]<<"\t0.00%\t";
			of_trim_stat1<<gv.raw1_stat.ts.ht[i]<<"\t0.00%\t";
		}
		if(tail_total1>0){
			of_trim_stat1<<gv.raw1_stat.ts.ta[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.raw1_stat.ts.ta[i]/tail_total1<<"%\t";
			of_trim_stat1<<gv.raw1_stat.ts.tlq[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.raw1_stat.ts.tlq[i]/tail_total1<<"%\t";
			of_trim_stat1<<gv.raw1_stat.ts.tt[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.raw1_stat.ts.tt[i]/tail_total1<<"%\t";
		}else{
			of_trim_stat1<<gv.raw1_stat.ts.ta[i]<<"\t0.00%\t";
			of_trim_stat1<<gv.raw1_stat.ts.tlq[i]<<"\t0.00%\t";
			of_trim_stat1<<gv.raw1_stat.ts.tlq[i]<<"\t0.00%\t";
		}
		if(head_total_clean1>0){
			of_trim_stat1<<gv.clean1_stat.ts.hlq[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.clean1_stat.ts.hlq[i]/head_total_clean1<<"%\t";
			of_trim_stat1<<gv.clean1_stat.ts.ht[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.clean1_stat.ts.ht[i]/head_total_clean1<<"%\t";
		}else{
			of_trim_stat1<<gv.clean1_stat.ts.hlq[i]<<"\t0.00%\t";
			of_trim_stat1<<gv.clean1_stat.ts.ht[i]<<"\t0.00%\t";
		}
		if(tail_total_clean1>0){
			of_trim_stat1<<gv.clean1_stat.ts.ta[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.clean1_stat.ts.ta[i]/tail_total_clean1<<"%\t";
			of_trim_stat1<<gv.clean1_stat.ts.tlq[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.clean1_stat.ts.tlq[i]/tail_total_clean1<<"%\t";
			of_trim_stat1<<gv.clean1_stat.ts.tt[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.clean1_stat.ts.tt[i]/tail_total_clean1<<"%"<<endl;
		}else{
			of_trim_stat1<<gv.clean1_stat.ts.ta[i]<<"\t0.00%\t";
			of_trim_stat1<<gv.clean1_stat.ts.tlq[i]<<"\t0.00%\t";
			of_trim_stat1<<gv.clean1_stat.ts.tlq[i]<<"\t0.00%"<<endl;
		}
		
		of_trim_stat2<<i<<"\t";
		if(head_total2>0){
			of_trim_stat2<<gv.raw2_stat.ts.hlq[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.raw2_stat.ts.hlq[i]/head_total2<<"%\t";
			of_trim_stat2<<gv.raw2_stat.ts.ht[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.raw2_stat.ts.ht[i]/head_total2<<"%\t";
		}else{
			of_trim_stat2<<gv.raw2_stat.ts.hlq[i]<<"\t0.00%\t";
			of_trim_stat2<<gv.raw2_stat.ts.ht[i]<<"\t0.00%\t";
		}
		if(tail_total2>0){
			of_trim_stat2<<gv.raw2_stat.ts.ta[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.raw2_stat.ts.ta[i]/tail_total2<<"%\t";
			of_trim_stat2<<gv.raw2_stat.ts.tlq[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.raw2_stat.ts.tlq[i]/tail_total2<<"%\t";
			of_trim_stat2<<gv.raw2_stat.ts.tt[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.raw2_stat.ts.tt[i]/tail_total2<<"%\t";
		}else{
			of_trim_stat2<<gv.raw2_stat.ts.ta[i]<<"\t0.00%\t";
			of_trim_stat2<<gv.raw2_stat.ts.tlq[i]<<"\t0.00%\t";
			of_trim_stat2<<gv.raw2_stat.ts.tlq[i]<<"\t0.00%\t";
		}
		if(head_total_clean2>0){
			of_trim_stat2<<gv.clean2_stat.ts.hlq[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.clean2_stat.ts.hlq[i]/head_total_clean2<<"%\t";
			of_trim_stat2<<gv.clean2_stat.ts.ht[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.clean2_stat.ts.ht[i]/head_total_clean2<<"%\t";
		}else{
			of_trim_stat2<<gv.clean2_stat.ts.hlq[i]<<"\t0.00%\t";
			of_trim_stat2<<gv.clean2_stat.ts.ht[i]<<"\t0.00%\t";
		}
		if(tail_total_clean2>0){
			of_trim_stat2<<gv.clean2_stat.ts.ta[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.clean2_stat.ts.ta[i]/tail_total_clean2<<"%\t";
			of_trim_stat2<<gv.clean2_stat.ts.tlq[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.clean2_stat.ts.tlq[i]/tail_total_clean2<<"%\t";
			of_trim_stat2<<gv.clean2_stat.ts.tt[i]<<"\t"<<setiosflags(ios::fixed)<<setprecision(2)<<100*(float)gv.clean2_stat.ts.tt[i]/tail_total_clean2<<"%"<<endl;
		}else{
			of_trim_stat2<<gv.clean2_stat.ts.ta[i]<<"\t0.00%\t";
			of_trim_stat2<<gv.clean2_stat.ts.tlq[i]<<"\t0.00%\t";
			of_trim_stat2<<gv.clean2_stat.ts.tlq[i]<<"\t0.00%"<<endl;
		}
	}
	of_trim_stat1.close();
	of_trim_stat2.close();
}
void peProcess::update_stat(C_fastq_file_stat& fq1s_stat,C_fastq_file_stat& fq2s_stat,C_filter_stat& fs_stat,string type){	//update statistic information from each thread
	if(type=="raw"){
		if(gv.raw1_stat.gs.read_length==0)
			gv.raw1_stat.gs.read_length=fq1s_stat.gs.read_length;	//generate stat
		if(gv.raw1_stat.gs.read_max_length<fq1s_stat.gs.read_length){
			gv.raw1_stat.gs.read_max_length=fq1s_stat.gs.read_length;
		}
		
		gv.raw1_stat.gs.reads_number+=fq1s_stat.gs.reads_number;
		gv.raw1_stat.gs.base_number+=fq1s_stat.gs.base_number;
		gv.raw1_stat.gs.a_number+=fq1s_stat.gs.a_number;
		gv.raw1_stat.gs.c_number+=fq1s_stat.gs.c_number;
		gv.raw1_stat.gs.g_number+=fq1s_stat.gs.g_number;
		gv.raw1_stat.gs.t_number+=fq1s_stat.gs.t_number;
		gv.raw1_stat.gs.n_number+=fq1s_stat.gs.n_number;
		gv.raw1_stat.gs.q20_num+=fq1s_stat.gs.q20_num;
		gv.raw1_stat.gs.q30_num+=fq1s_stat.gs.q30_num;
		
		if(gv.raw2_stat.gs.read_length==0)
			gv.raw2_stat.gs.read_length=fq2s_stat.gs.read_length;
		if(gv.raw2_stat.gs.read_max_length<fq2s_stat.gs.read_length){
			gv.raw2_stat.gs.read_max_length=fq2s_stat.gs.read_length;
		}
		gv.raw2_stat.gs.reads_number+=fq2s_stat.gs.reads_number;
		gv.raw2_stat.gs.base_number+=fq2s_stat.gs.base_number;
		gv.raw2_stat.gs.a_number+=fq2s_stat.gs.a_number;
		gv.raw2_stat.gs.c_number+=fq2s_stat.gs.c_number;
		gv.raw2_stat.gs.g_number+=fq2s_stat.gs.g_number;
		gv.raw2_stat.gs.t_number+=fq2s_stat.gs.t_number;
		gv.raw2_stat.gs.n_number+=fq2s_stat.gs.n_number;
		gv.raw2_stat.gs.q20_num+=fq2s_stat.gs.q20_num;
		gv.raw2_stat.gs.q30_num+=fq2s_stat.gs.q30_num;

		string base_set="ACGTN";	//base content and quality along read position stat
		int max_qual=0;
		for(int i=0;i!=gv.raw1_stat.gs.read_max_length;i++){
			for(int j=0;j!=base_set.size();j++){
				gv.raw1_stat.bs.position_acgt_content[i][j]+=fq1s_stat.bs.position_acgt_content[i][j];
				gv.raw2_stat.bs.position_acgt_content[i][j]+=fq2s_stat.bs.position_acgt_content[i][j];
			}
		}
		for(int i=0;i!=gv.raw1_stat.gs.read_max_length;i++){
			gv.raw1_stat.ts.ht[i]+=fq1s_stat.ts.ht[i];
			gv.raw1_stat.ts.hlq[i]+=fq1s_stat.ts.hlq[i];
			gv.raw1_stat.ts.tt[i]+=fq1s_stat.ts.tt[i];
			gv.raw1_stat.ts.tlq[i]+=fq1s_stat.ts.tlq[i];
			gv.raw1_stat.ts.ta[i]+=fq1s_stat.ts.ta[i];
			gv.raw2_stat.ts.ht[i]+=fq2s_stat.ts.ht[i];
			gv.raw2_stat.ts.hlq[i]+=fq2s_stat.ts.hlq[i];
			gv.raw2_stat.ts.tt[i]+=fq2s_stat.ts.tt[i];
			gv.raw2_stat.ts.tlq[i]+=fq2s_stat.ts.tlq[i];
			gv.raw2_stat.ts.ta[i]+=fq2s_stat.ts.ta[i];
		}
		for(int i=0;i!=gv.raw1_stat.gs.read_max_length;i++){
			for(int j=1;j<=MAX_QUAL;j++){
				if(fq1s_stat.qs.position_qual[i][j]>0){
					max_qual=max_qual>j?max_qual:j;
				}
			}
		}
		//cout<<max_qual<<endl;
		for(int i=0;i!=gv.raw1_stat.gs.read_max_length;i++){
			for(int j=0;j<=max_qual;j++){
				gv.raw1_stat.qs.position_qual[i][j]+=fq1s_stat.qs.position_qual[i][j];
				gv.raw2_stat.qs.position_qual[i][j]+=fq2s_stat.qs.position_qual[i][j];
			}
		}
		//gv.fs.output_reads_num+=fs_stat.output_reads_num;	//filter stat
		gv.fs.in_adapter_list_num+=fs_stat.in_adapter_list_num;
		gv.fs.include_adapter_seq_num+=fs_stat.include_adapter_seq_num;
		gv.fs.include_adapter_seq_num1+=fs_stat.include_adapter_seq_num1;
		gv.fs.include_adapter_seq_num2+=fs_stat.include_adapter_seq_num2;
		gv.fs.include_adapter_seq_num_overlap+=fs_stat.include_adapter_seq_num_overlap;

		gv.fs.n_ratio_num+=fs_stat.n_ratio_num;
		gv.fs.n_ratio_num1+=fs_stat.n_ratio_num1;
		gv.fs.n_ratio_num2+=fs_stat.n_ratio_num2;
		gv.fs.n_ratio_num_overlap+=fs_stat.n_ratio_num_overlap;

		gv.fs.highA_num+=fs_stat.highA_num;
		gv.fs.highA_num1+=fs_stat.highA_num1;
		gv.fs.highA_num2+=fs_stat.highA_num2;
		gv.fs.highA_num_overlap+=fs_stat.highA_num_overlap;

		gv.fs.polyX_num+=fs_stat.polyX_num;
		gv.fs.polyX_num1+=fs_stat.polyX_num1;
		gv.fs.polyX_num2+=fs_stat.polyX_num2;
		gv.fs.polyX_num_overlap+=fs_stat.polyX_num_overlap;

		gv.fs.tile_num+=fs_stat.tile_num;
		gv.fs.fov_num+=fs_stat.fov_num;
		gv.fs.over_lapped_num+=fs_stat.over_lapped_num;

		gv.fs.low_qual_base_ratio_num+=fs_stat.low_qual_base_ratio_num;
		gv.fs.low_qual_base_ratio_num1+=fs_stat.low_qual_base_ratio_num1;
		gv.fs.low_qual_base_ratio_num2+=fs_stat.low_qual_base_ratio_num2;
		gv.fs.low_qual_base_ratio_num_overlap+=fs_stat.low_qual_base_ratio_num_overlap;

		gv.fs.mean_quality_num+=fs_stat.mean_quality_num;
		gv.fs.mean_quality_num1+=fs_stat.mean_quality_num1;
		gv.fs.mean_quality_num2+=fs_stat.mean_quality_num2;
		gv.fs.mean_quality_num_overlap+=fs_stat.mean_quality_num_overlap;

		gv.fs.short_len_num+=fs_stat.short_len_num;
		gv.fs.short_len_num1+=fs_stat.short_len_num1;
		gv.fs.short_len_num2+=fs_stat.short_len_num2;
		gv.fs.short_len_num_overlap+=fs_stat.short_len_num_overlap;

		gv.fs.long_len_num+=fs_stat.long_len_num;
		gv.fs.long_len_num1+=fs_stat.long_len_num1;
		gv.fs.long_len_num2+=fs_stat.long_len_num2;
		gv.fs.long_len_num_overlap+=fs_stat.long_len_num_overlap;

		gv.fs.include_contam_seq_num+=fs_stat.include_contam_seq_num;
		gv.fs.include_contam_seq_num1+=fs_stat.include_contam_seq_num1;
		gv.fs.include_contam_seq_num2+=fs_stat.include_contam_seq_num2;
		gv.fs.include_contam_seq_num_overlap+=fs_stat.include_contam_seq_num_overlap;

		gv.fs.include_global_contam_seq_num+=fs_stat.include_global_contam_seq_num;
		gv.fs.include_global_contam_seq_num1+=fs_stat.include_global_contam_seq_num1;
		gv.fs.include_global_contam_seq_num2+=fs_stat.include_global_contam_seq_num2;
		gv.fs.include_global_contam_seq_num_overlap+=fs_stat.include_global_contam_seq_num_overlap;

		
	}else if(type=="trim"){
			//generate stat
		gv.trim1_stat.gs.reads_number+=fq1s_stat.gs.reads_number;
		gv.trim1_stat.gs.base_number+=fq1s_stat.gs.base_number;
		if(gv.trim1_stat.gs.reads_number==0){
			gv.trim1_stat.gs.read_length=fq1s_stat.gs.read_length;
		}else{
			gv.trim1_stat.gs.read_length=gv.trim1_stat.gs.base_number/gv.trim1_stat.gs.reads_number;
		}
		if(gv.trim1_stat.gs.read_max_length<fq1s_stat.gs.read_length){
			gv.trim1_stat.gs.read_max_length=fq1s_stat.gs.read_length;
		}
		gv.trim1_stat.gs.a_number+=fq1s_stat.gs.a_number;
		gv.trim1_stat.gs.c_number+=fq1s_stat.gs.c_number;
		gv.trim1_stat.gs.g_number+=fq1s_stat.gs.g_number;
		gv.trim1_stat.gs.t_number+=fq1s_stat.gs.t_number;
		gv.trim1_stat.gs.n_number+=fq1s_stat.gs.n_number;
		gv.trim1_stat.gs.q20_num+=fq1s_stat.gs.q20_num;
		gv.trim1_stat.gs.q30_num+=fq1s_stat.gs.q30_num;
		
		
		//gv.trim2_stat.gs.read_length=fq2s_stat.gs.read_length;
		gv.trim2_stat.gs.reads_number+=fq2s_stat.gs.reads_number;
		gv.trim2_stat.gs.base_number+=fq2s_stat.gs.base_number;
		if(gv.trim2_stat.gs.reads_number==0){
			gv.trim2_stat.gs.read_length=fq2s_stat.gs.read_length;
		}else{
			gv.trim2_stat.gs.read_length=gv.trim2_stat.gs.base_number/gv.trim2_stat.gs.reads_number;
		}
		if(gv.trim2_stat.gs.read_max_length<fq2s_stat.gs.read_length){
			gv.trim2_stat.gs.read_max_length=fq2s_stat.gs.read_length;
		}
		gv.trim2_stat.gs.a_number+=fq2s_stat.gs.a_number;
		gv.trim2_stat.gs.c_number+=fq2s_stat.gs.c_number;
		gv.trim2_stat.gs.g_number+=fq2s_stat.gs.g_number;
		gv.trim2_stat.gs.t_number+=fq2s_stat.gs.t_number;
		gv.trim2_stat.gs.n_number+=fq2s_stat.gs.n_number;
		gv.trim2_stat.gs.q20_num+=fq2s_stat.gs.q20_num;
		gv.trim2_stat.gs.q30_num+=fq2s_stat.gs.q30_num;
		int max_qual(0);
		string base_set="ACGTN";	//base content and quality along read position stat
		for(int i=0;i!=gv.trim1_stat.gs.read_max_length;i++){
			for(int j=0;j!=base_set.size();j++){
				gv.trim1_stat.bs.position_acgt_content[i][j]+=fq1s_stat.bs.position_acgt_content[i][j];
				gv.trim2_stat.bs.position_acgt_content[i][j]+=fq2s_stat.bs.position_acgt_content[i][j];
			}
		}
		for(int i=0;i!=gv.trim1_stat.gs.read_max_length;i++){
			for(int j=1;j<=MAX_QUAL;j++){
				if(fq1s_stat.qs.position_qual[i][j]>0){
					max_qual=max_qual>j?max_qual:j;
				}
			}
		}
		for(int i=0;i!=gv.trim1_stat.gs.read_max_length;i++){
			for(int j=0;j<=max_qual;j++){
				gv.trim1_stat.qs.position_qual[i][j]+=fq1s_stat.qs.position_qual[i][j];
				gv.trim2_stat.qs.position_qual[i][j]+=fq2s_stat.qs.position_qual[i][j];
			}
		}
	}else if(type=="clean"){
		//gp.output_reads_num+=fq1s_stat.gs.reads_number;
		//cout<<gp.output_reads_num<<"\t"<<gp.total_reads_num<<endl;
		//if(gv.clean1_stat.gs.read_length==0)
		//	gv.clean1_stat.gs.read_length=fq1s_stat.gs.read_length;	//generate stat
		gv.clean1_stat.gs.reads_number+=fq1s_stat.gs.reads_number;
		gv.clean1_stat.gs.base_number+=fq1s_stat.gs.base_number;
		if(gv.clean1_stat.gs.reads_number==0){
			gv.clean1_stat.gs.read_length=fq1s_stat.gs.read_length;
		}else{
			gv.clean1_stat.gs.read_length=gv.clean1_stat.gs.base_number/gv.clean1_stat.gs.reads_number;
		}
		if(gv.clean1_stat.gs.read_max_length<fq1s_stat.gs.read_length){
			gv.clean1_stat.gs.read_max_length=fq1s_stat.gs.read_length;
		}
		gv.clean1_stat.gs.a_number+=fq1s_stat.gs.a_number;
		gv.clean1_stat.gs.c_number+=fq1s_stat.gs.c_number;
		gv.clean1_stat.gs.g_number+=fq1s_stat.gs.g_number;
		gv.clean1_stat.gs.t_number+=fq1s_stat.gs.t_number;
		gv.clean1_stat.gs.n_number+=fq1s_stat.gs.n_number;
		gv.clean1_stat.gs.q20_num+=fq1s_stat.gs.q20_num;
		gv.clean1_stat.gs.q30_num+=fq1s_stat.gs.q30_num;
		
		//if(gv.clean2_stat.gs.read_length==0)
		//	gv.clean2_stat.gs.read_length=fq2s_stat.gs.read_length;
		gv.clean2_stat.gs.reads_number+=fq2s_stat.gs.reads_number;
		gv.clean2_stat.gs.base_number+=fq2s_stat.gs.base_number;
		if(gv.clean2_stat.gs.reads_number==0){
			gv.clean2_stat.gs.read_length=fq2s_stat.gs.read_length;
		}else{
			gv.clean2_stat.gs.read_length=gv.clean2_stat.gs.base_number/gv.clean2_stat.gs.reads_number;
		}
		if(gv.clean2_stat.gs.read_max_length<gv.clean2_stat.gs.read_length){
			gv.clean2_stat.gs.read_max_length=gv.clean2_stat.gs.read_length;
		}
		gv.clean2_stat.gs.a_number+=fq2s_stat.gs.a_number;
		gv.clean2_stat.gs.c_number+=fq2s_stat.gs.c_number;
		gv.clean2_stat.gs.g_number+=fq2s_stat.gs.g_number;
		gv.clean2_stat.gs.t_number+=fq2s_stat.gs.t_number;
		gv.clean2_stat.gs.n_number+=fq2s_stat.gs.n_number;
		gv.clean2_stat.gs.q20_num+=fq2s_stat.gs.q20_num;
		gv.clean2_stat.gs.q30_num+=fq2s_stat.gs.q30_num;
		string base_set="ACGTN";	//base content and quality along read position stat
		int max_qual1(0),max_qual2(0);
		for(int i=0;i!=gv.clean1_stat.gs.read_max_length;i++){
			for(int j=0;j!=base_set.size();j++){
				gv.clean1_stat.bs.position_acgt_content[i][j]+=fq1s_stat.bs.position_acgt_content[i][j];
			}
		}
		for(int i=0;i!=gv.clean2_stat.gs.read_max_length;i++){
			for(int j=0;j!=base_set.size();j++){
				gv.clean2_stat.bs.position_acgt_content[i][j]+=fq2s_stat.bs.position_acgt_content[i][j];
			}
		}
		for(int i=0;i!=gv.clean1_stat.gs.read_max_length;i++){
			gv.clean1_stat.ts.ht[i]+=fq1s_stat.ts.ht[i];
			gv.clean1_stat.ts.hlq[i]+=fq1s_stat.ts.hlq[i];
			gv.clean1_stat.ts.tt[i]+=fq1s_stat.ts.tt[i];
			gv.clean1_stat.ts.tlq[i]+=fq1s_stat.ts.tlq[i];
			gv.clean1_stat.ts.ta[i]+=fq1s_stat.ts.ta[i];
			
		}
		for(int i=0;i!=gv.clean2_stat.gs.read_max_length;i++){
			gv.clean2_stat.ts.ht[i]+=fq2s_stat.ts.ht[i];
			gv.clean2_stat.ts.hlq[i]+=fq2s_stat.ts.hlq[i];
			gv.clean2_stat.ts.tt[i]+=fq2s_stat.ts.tt[i];
			gv.clean2_stat.ts.tlq[i]+=fq2s_stat.ts.tlq[i];
			gv.clean2_stat.ts.ta[i]+=fq2s_stat.ts.ta[i];
		}
		for(int i=0;i!=gv.clean1_stat.gs.read_max_length;i++){
			for(int j=1;j<=MAX_QUAL;j++){
				if(fq1s_stat.qs.position_qual[i][j]>0){
					max_qual1=max_qual1>j?max_qual1:j;
				}
			}
		}
		for(int i=0;i!=gv.clean1_stat.gs.read_max_length;i++){
			for(int j=0;j<=max_qual1;j++){
				gv.clean1_stat.qs.position_qual[i][j]+=fq1s_stat.qs.position_qual[i][j];
			}
		}
		for(int i=0;i!=gv.clean2_stat.gs.read_max_length;i++){
			for(int j=1;j<=MAX_QUAL;j++){
				if(fq2s_stat.qs.position_qual[i][j]>0){
					max_qual2=max_qual2>j?max_qual2:j;
				}
			}
		}
		for(int i=0;i!=gv.clean2_stat.gs.read_max_length;i++){
			for(int j=0;j<=max_qual2;j++){
				gv.clean2_stat.qs.position_qual[i][j]+=fq2s_stat.qs.position_qual[i][j];
			}
		}
	}else{
		cerr<<"Error:code error"<<endl;
		exit(1);
	}
	

}
void* peProcess::stat_pe_fqs(PEstatOption opt,string dataType){	//statistic the pair-ends fastq
	opt.stat1->gs.reads_number+=opt.fq1s->size();
	opt.stat2->gs.reads_number+=opt.fq2s->size();
	vector<C_fastq>::iterator ix_end=opt.fq1s->end();
	int qualityBase=0;
	if(dataType=="clean"){
		qualityBase=gp.outputQualityPhred;
	}else{
		qualityBase=gp.qualityPhred;
	}
	for(vector<C_fastq>::iterator ix=opt.fq1s->begin();ix!=ix_end;ix++){
		if((*ix).head_hdcut>0 || (*ix).head_lqcut>0){
			if((*ix).head_hdcut>=(*ix).head_lqcut){
				opt.stat1->ts.ht[(*ix).head_hdcut]++;
			}else{
				opt.stat1->ts.hlq[(*ix).head_lqcut]++;
			}
		}
		if((*ix).tail_hdcut>0 || (*ix).tail_lqcut>0 || (*ix).adacut_pos>0){
			if((*ix).tail_hdcut>=(*ix).tail_lqcut){
				if((*ix).tail_hdcut>=(*ix).adacut_pos){
					opt.stat1->ts.tt[(*ix).raw_length-(*ix).tail_hdcut+1]++;
				}else{

					opt.stat1->ts.ta[(*ix).raw_length-(*ix).adacut_pos+1]++;
				}
			}else{
				if((*ix).tail_lqcut>=(*ix).adacut_pos){
					opt.stat1->ts.tlq[(*ix).raw_length-(*ix).tail_lqcut+1]++;
				}else{
					opt.stat1->ts.ta[(*ix).raw_length-(*ix).adacut_pos+1]++;
				}
			}
		}
		int seq_len=(*ix).sequence.size();
		for(string::size_type i=0;i!=seq_len;i++){
			switch(((*ix).sequence)[i]){
				case 'a':
				case 'A':opt.stat1->bs.position_acgt_content[i][0]++;opt.stat1->gs.a_number++;break;
				case 'c':
				case 'C':opt.stat1->bs.position_acgt_content[i][1]++;opt.stat1->gs.c_number++;break;
				case 'g':
				case 'G':opt.stat1->bs.position_acgt_content[i][2]++;opt.stat1->gs.g_number++;break;
				case 't':
				case 'T':opt.stat1->bs.position_acgt_content[i][3]++;opt.stat1->gs.t_number++;break;
				case 'n':
				case 'N':opt.stat1->bs.position_acgt_content[i][4]++;opt.stat1->gs.n_number++;break;
				default:{
					cerr<<"Error:unrecognized sequence,"<<((*ix).sequence)<<endl;
					exit(1);
				}
			}
		}
		int qual_len=(*ix).qual_seq.size();
		for(string::size_type i=0;i!=qual_len;i++){	//process quality sequence
			int base_quality=((*ix).qual_seq)[i]-qualityBase;
			/*
			if(base_quality>MAX_QUAL){
				//cout<<i<<"\t"<<base_quality<<endl;
				cerr<<"Error:quality is too high,please check the quality system parameter or fastq file"<<endl;
				exit(1);
			}
			if(base_quality<MIN_QUAL){
				cerr<<"Error:quality is too low,please check the quality system parameter or fastq file"<<endl;
				exit(1);
			}
			*/
			opt.stat1->qs.position_qual[i][base_quality]++;
			if(base_quality>=20)
				opt.stat1->gs.q20_num++;
			if(base_quality>=30)
				opt.stat1->gs.q30_num++;
		}
		opt.stat1->gs.read_length=(*ix).sequence.size();
		opt.stat1->gs.base_number+=opt.stat1->gs.read_length;
	}
	//check base quality setting whether ok
	//run one time
	int q1_exceed(0),q1_normal_bq(0),q1_mean_bq(0);
	int q2_exceed(0),q2_normal_bq(0),q2_mean_bq(0);
	float q1_normal_ratio(0.0),q2_normal_ratio(0.0);
	float q1_mean(0.0),q2_mean(0.0);
	if(bq_check==0){
		pe_cal_m.lock();
		for(vector<C_fastq>::iterator ix=opt.fq1s->begin();ix!=ix_end;ix++){
			int qual_len=(*ix).qual_seq.size();
			for(string::size_type i=0;i!=qual_len;i++){	//process quality sequence
				int base_quality=((*ix).qual_seq)[i]-gp.qualityPhred;
				q1_mean_bq+=base_quality;
				if(base_quality>=MIN_QUAL && base_quality<=MAX_QUAL){
					q1_normal_bq++;
				}else{
					if(base_quality<MIN_QUAL-10 || base_quality>MAX_QUAL+10)
						q1_exceed++;
				}
				int another_q=gp.qualityPhred==64?33:64;
				int base_quality2=((*ix).qual_seq)[i]-another_q;
				q2_mean_bq+=base_quality2;
				if(base_quality2>=MIN_QUAL && base_quality2<=MAX_QUAL){
					q2_normal_bq++;
				}else{
					if(base_quality2<MIN_QUAL-10 || base_quality2>MAX_QUAL+10)
						q2_exceed++;
				}
			}
		}
		
		if(opt.stat1->gs.base_number==0){
			cerr<<"Error:no data"<<endl;
			exit(1);
		}
		q1_normal_ratio=(float)q1_normal_bq/opt.stat1->gs.base_number;
		q2_normal_ratio=(float)q2_normal_bq/opt.stat1->gs.base_number;
		q1_mean=(float)q1_mean_bq/opt.stat1->gs.base_number;
		q2_mean=(float)q2_mean_bq/opt.stat1->gs.base_number;
		//cout<<q1_exceed<<"\t"<<q1_normal_bq<<"\t"<<opt.stat1->gs.base_number<<"\t"<<q1_normal_ratio<<"\t"<<q1_mean<<endl;
		//cout<<q2_exceed<<"\t"<<q2_normal_bq<<"\t"<<opt.stat1->gs.base_number<<"\t"<<q2_normal_ratio<<"\t"<<q2_mean<<endl;
		int q1_score(0),q2_score(0);
		if(q1_exceed){
			q1_score+=0;
		}else{
			q1_score+=1;
		}
		if(q2_exceed){
			q2_score+=0;
		}else{
			q2_score+=1;
		}
		if(q1_normal_ratio>q2_normal_ratio){
			q1_score+=3;
			q2_score+=0;
		}else if(q1_normal_ratio<q2_normal_ratio){
			q1_score+=0;
			q2_score+=3;
		}else{
			q1_score+=3;
			q2_score+=3;
		}
		if(q1_mean<10 || q1_mean>MAX_QUAL){
			q1_score+=0;
		}else{
			q1_score+=2;
		}
		if(q2_mean<10 || q2_mean>MAX_QUAL){
			q2_score+=0;
		}else{
			q2_score+=2;
		}
		//cout<<q1_score<<"\t"<<q2_score<<endl;
		if(bq_check==0){
			if(q1_score-q2_score<-3){
				cerr<<"Error:base quality seems abnormal,please check the quality system parameter or fastq file"<<endl;
				exit(1);
			}else if(q1_score-q2_score<0){
				cerr<<"Warning:base quality seems abnormal,please check the quality system parameter or fastq file"<<endl;
			}
		}
		bq_check++;
		pe_cal_m.unlock();
	}

	//stat read sequence related information
	vector<C_fastq>::iterator ix2_end=opt.fq2s->end();
	for(vector<C_fastq>::iterator ix=opt.fq2s->begin();ix!=ix2_end;ix++){
		if((*ix).head_hdcut>0 || (*ix).head_lqcut>0){
			if((*ix).head_hdcut>=(*ix).head_lqcut){
				opt.stat2->ts.ht[(*ix).head_hdcut]++;
			}else{
				opt.stat2->ts.hlq[(*ix).head_lqcut]++;
			}
		}
		if((*ix).tail_hdcut>0 || (*ix).tail_lqcut>0 || (*ix).adacut_pos>0){
			if((*ix).tail_hdcut>=(*ix).tail_lqcut){
				if((*ix).tail_hdcut>=(*ix).adacut_pos){
					opt.stat2->ts.tt[(*ix).sequence.size()-(*ix).tail_hdcut+1]++;
				}else{
					opt.stat2->ts.ta[(*ix).sequence.size()-(*ix).adacut_pos+1]++;
				}
			}else{
				if((*ix).tail_lqcut>=(*ix).adacut_pos){
					opt.stat2->ts.tlq[(*ix).sequence.size()-(*ix).tail_lqcut+1]++;
				}else{
					opt.stat2->ts.ta[(*ix).sequence.size()-(*ix).adacut_pos+1]++;
				}
			}
		}
		int seq_len=(*ix).sequence.size();
		for(string::size_type i=0;i!=seq_len;i++){
			switch(((*ix).sequence)[i]){
				case 'a':
				case 'A':opt.stat2->bs.position_acgt_content[i][0]++;opt.stat2->gs.a_number++;break;
				case 'c':
				case 'C':opt.stat2->bs.position_acgt_content[i][1]++;opt.stat2->gs.c_number++;break;
				case 'g':
				case 'G':opt.stat2->bs.position_acgt_content[i][2]++;opt.stat2->gs.g_number++;break;
				case 't':
				case 'T':opt.stat2->bs.position_acgt_content[i][3]++;opt.stat2->gs.t_number++;break;
				case 'n':
				case 'N':opt.stat2->bs.position_acgt_content[i][4]++;opt.stat2->gs.n_number++;break;
				default:{
					cerr<<"Error:unrecognized sequence,"<<((*ix).sequence)<<endl;
					exit(1);
				}
			}
		}
		//stat base quality related information
		int qual_len=(*ix).qual_seq.size();
		for(string::size_type i=0;i!=qual_len;i++){	//process quality sequence
			int base_quality=((*ix).qual_seq)[i]-qualityBase;
			/*
			if(base_quality>MAX_QUAL){
				cerr<<"Error:quality is too high,please check the quality system parameter or fastq file"<<endl;
				exit(1);
			}
			if(base_quality<MIN_QUAL){
				cerr<<"Error:quality is too low,please check the quality system parameter or fastq file"<<endl;
				exit(1);
			}
			*/
			opt.stat2->qs.position_qual[i][base_quality]++;
			if(base_quality>=20)
				opt.stat2->gs.q20_num++;
			if(base_quality>=30)
				opt.stat2->gs.q30_num++;
		}
		opt.stat2->gs.read_length=(*ix).sequence.size();
		opt.stat2->gs.base_number+=opt.stat2->gs.read_length;
	}
    return &bq_check;
}
void peProcess::filter_pe_fqs(PEcalOption* opt){
	//C_reads_trim_stat_2 cut_pos;
	vector<C_fastq>::iterator i2=opt->fq2s->begin();
	vector<C_fastq>::iterator i_end=opt->fq1s->end();
	for(vector<C_fastq>::iterator i=opt->fq1s->begin();i!=i_end;i++){
		C_pe_fastq_filter pe_fastq_filter=C_pe_fastq_filter(*i,*i2,gp);
		/*int head_hdcut,head_lqcut,tail_hdcut,tail_lqcut,adacut_pos;
	int contam_pos;
	int global_contam_pos;
	int raw_length;*/
		pe_fastq_filter.pe_trim(gp);
		if(gp.adapter_discard_or_trim=="trim" || gp.contam_discard_or_trim=="trim" || !gp.trim.empty() || !gp.trimBadHead.empty() || !gp.trimBadTail.empty()){
			(*i).head_hdcut=pe_fastq_filter.fq1.head_hdcut;
			(*i).head_lqcut=pe_fastq_filter.fq1.head_lqcut;
			(*i).tail_hdcut=pe_fastq_filter.fq1.tail_hdcut;
			(*i).tail_lqcut=pe_fastq_filter.fq1.tail_lqcut;
			(*i).adacut_pos=pe_fastq_filter.fq1.adacut_pos;
			//(*i).contam_pos=pe_fastq_filter.fq1.contam_pos;
			//(*i).global_contam_pos=pe_fastq_filter.fq1.global_contam_pos;
			//(*i).raw_length=pe_fastq_filter.fq1.raw_length;
			(*i2).head_hdcut=pe_fastq_filter.fq2.head_hdcut;
			(*i2).head_lqcut=pe_fastq_filter.fq2.head_lqcut;
			(*i2).tail_hdcut=pe_fastq_filter.fq2.tail_hdcut;
			(*i2).tail_lqcut=pe_fastq_filter.fq2.tail_lqcut;
			(*i2).adacut_pos=pe_fastq_filter.fq2.adacut_pos;
			//(*i2).contam_pos=pe_fastq_filter.fq2.contam_pos;
			//(*i2).global_contam_pos=pe_fastq_filter.fq2.global_contam_pos;
			//(*i2).raw_length=pe_fastq_filter.fq2.raw_length;
		}
		if(!gp.trim_fq1.empty()){
			preOutput(1,pe_fastq_filter.fq1);
			preOutput(2,pe_fastq_filter.fq2);
			opt->trim_result1->emplace_back(pe_fastq_filter.fq1);
			opt->trim_result2->emplace_back(pe_fastq_filter.fq2);
		}
		if(pe_fastq_filter.pe_discard(opt->local_fs,gp)!=1){
			if(!gp.clean_fq1.empty()){
				preOutput(1,pe_fastq_filter.fq1);
				preOutput(2,pe_fastq_filter.fq2);
				opt->clean_result1->emplace_back(pe_fastq_filter.fq1);
				opt->clean_result2->emplace_back(pe_fastq_filter.fq2);
			}
		}
		i2++;
		if(i2==opt->fq2s->end()){
			break;
		}
	}
	//return cut_pos;
}

void  peProcess::preOutput(int type,C_fastq& a){	//modify the sequences before output if necessary
	if(gp.whether_add_pe_info){
		if(type==1){
			a.seq_id=a.seq_id+"/1";
		}else{
			a.seq_id=a.seq_id+"/2";
		}
	}
	if(!gp.base_convert.empty()){
	    if(gp.base_convert.find("TO")!=string::npos)
		    gp.base_convert=gp.base_convert.replace(gp.base_convert.find("TO"),2,"");
	    if(gp.base_convert.find("2")!=string::npos)
		    gp.base_convert=gp.base_convert.replace(gp.base_convert.find("2"),1,"");
		if(gp.base_convert.size()!=2){
			cerr<<"Error:base_conver value format error"<<endl;
			exit(1);
		}
		for(string::size_type ix=0;ix!=a.sequence.size();ix++){
			if(toupper(a.sequence[ix])==toupper(gp.base_convert[0]))
				a.sequence[ix]=gp.base_convert[1];
		}
	}
}
void peProcess::peWrite(vector<C_fastq>& pe1,vector<C_fastq>& pe2,gzFile out1,gzFile out2){	//output the sequences to  files
	output_fastqs("1",pe1,out1);
	output_fastqs("2",pe2,out2);
}
void peProcess::peWrite(vector<C_fastq>& pe1,vector<C_fastq>& pe2,FILE* out1,FILE* out2){
	output_fastqs("1",pe1,out1);
	output_fastqs("2",pe2,out2);
}
void peProcess::C_fastq_init(C_fastq& a,C_fastq& b){
	a.seq_id="";
	a.sequence="";
	a.qual_seq="";
	b.seq_id="";
	b.sequence="";
	b.qual_seq="";
	a.adapter_seq=gp.adapter1_seq;
	b.adapter_seq=gp.adapter2_seq;
	a.contam_seq=gp.contam1_seq;
	b.contam_seq=gp.contam2_seq;
	//a.global_contams=gp.global_contams;
	//b.global_contams=gp.global_contams;
	a.head_hdcut=-1;
	a.head_lqcut=-1;
	a.tail_hdcut=-1;
	a.tail_lqcut=-1;
	a.adacut_pos=-1;
	a.contam_pos=-1;
	a.global_contam_5pos=-1;
	a.global_contam_3pos=-1;
	b.head_hdcut=-1;
	b.head_lqcut=-1;
	b.tail_hdcut=-1;
	b.tail_lqcut=-1;
	b.adacut_pos=-1;
	b.contam_pos=-1;
	b.global_contam_5pos=-1;
	b.global_contam_3pos=-1;
	a.raw_length=0;
	b.raw_length=0;
	if(!gp.trim.empty()){
		vector<string> tmp_eles=get_pe_hard_trim(gp.trim);
		a.head_trim_len=tmp_eles[0];
		a.tail_trim_len=tmp_eles[1];
		b.head_trim_len=tmp_eles[2];
		b.tail_trim_len=tmp_eles[3];
	}

}
int peProcess::read(vector<C_fastq>& pe1,vector<C_fastq>& pe2,ifstream& infile1,ifstream& infile2){	//read file from ASCII text,used in ssd mode
	
	string buf1,buf2;
	int file1_line_num(0),file2_line_num(0);
	C_fastq fastq1,fastq2;
	C_fastq_init(fastq1,fastq2);
	for(int i=0;i<gp.patchSize*4;i++){
		if(getline(infile1,buf1)){
			file1_line_num++;
			//string s_line(buf1);
			//s_line.erase(s_line.size()-1);
			if(file1_line_num%4==1){
				fastq1.seq_id=buf1;
			}
			if(file1_line_num%4==2){
				fastq1.sequence=buf1;
			}
			if(file1_line_num%4==0){
				fastq1.qual_seq=buf1;
				//fq1s.emplace_back(fastq1);
			}
		}
		
		if(getline(infile2,buf2)){
			file2_line_num++;
			//string s_line(buf2);
			if(file2_line_num%4==1){
				fastq2.seq_id=buf2;
			}
			if(file2_line_num%4==2){
				fastq2.sequence=buf2;
			}
			if(file2_line_num%4==0){
				fastq2.qual_seq=buf2;
				pe1.emplace_back(fastq1);
				pe2.emplace_back(fastq2);
			}
		}else{
			return -1;
		}
	}
	return 0;
}
void peProcess::create_thread_smalltrimoutputFile(int index,int cycle){
    ostringstream trim_outfile1,trim_outfile2;
    if(gp.trim_fq1.rfind(".gz")==gp.trim_fq1.size()-3){	//create output trim files handle
        trim_outfile1<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<cycle<<".trim.r1.fq.gz";
        trim_outfile2<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<cycle<<".trim.r2.fq.gz";
        gz_trim_out1[index]=gzopen(trim_outfile1.str().c_str(),"wb");
        if(!gz_trim_out1[index]){
            cerr<<"Error:cannot write to the file,"<<trim_outfile1.str()<<endl;
            exit(1);
        }
        gzsetparams(gz_trim_out1[index], 2, Z_DEFAULT_STRATEGY);
        gzbuffer(gz_trim_out1[index],1024*1024*8);
        gz_trim_out2[index]=gzopen(trim_outfile2.str().c_str(),"wb");
        if(!gz_trim_out2[index]){
            cerr<<"Error:cannot write to the file,"<<trim_outfile2.str()<<endl;
            exit(1);
        }
        gzsetparams(gz_trim_out2[index], 2, Z_DEFAULT_STRATEGY);
        gzbuffer(gz_trim_out2[index],1024*1024*8);
    }else{
        trim_outfile1<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<cycle<<".trim.r1.fq";
        trim_outfile2<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<cycle<<".trim.r2.fq";
        if((nongz_clean_out1[index]=fopen(trim_outfile1.str().c_str(),"w"))==NULL){
            cerr<<"Error:cannot write to the file,"<<trim_outfile1.str()<<endl;
            exit(1);
        }
        if((nongz_clean_out2[index]=fopen(trim_outfile2.str().c_str(),"w"))==NULL){
            cerr<<"Error:cannot write to the file,"<<trim_outfile2.str()<<endl;
            exit(1);
        }
    }
}
void peProcess::create_thread_smallcleanoutputFile(int index,int cycle){
    ostringstream outfile1,outfile2;
    if(gp.clean_fq1.rfind(".gz")==gp.clean_fq1.size()-3){
        outfile1<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<cycle<<".clean.r1.fq.gz";
        outfile2<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<cycle<<".clean.r2.fq.gz";
        gz_clean_out1[index]=gzopen(outfile1.str().c_str(),"wb");
        if(!gz_clean_out1[index]){
            cerr<<"Error:cannot write to the file,"<<outfile1.str()<<endl;
            exit(1);
        }
        gzsetparams(gz_clean_out1[index], 2, Z_DEFAULT_STRATEGY);
        gzbuffer(gz_clean_out1[index],1024*1024*8);
        gz_clean_out2[index]=gzopen(outfile2.str().c_str(),"wb");
        if(!gz_clean_out2[index]){
            cerr<<"Error:cannot write to the file,"<<outfile2.str()<<endl;
            exit(1);
        }
        gzsetparams(gz_clean_out2[index], 2, Z_DEFAULT_STRATEGY);
        gzbuffer(gz_clean_out2[index],1024*1024*8);
    }else{
        outfile1<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<cycle<<".clean.r1.fq";
        outfile2<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<cycle<<".clean.r2.fq";
        if((nongz_clean_out1[index]=fopen(outfile1.str().c_str(),"w"))==NULL){
            cerr<<"Error:cannot write to the file,"<<outfile1.str()<<endl;
            exit(1);
        }
        if((nongz_clean_out2[index]=fopen(outfile2.str().c_str(),"w"))==NULL){
            cerr<<"Error:cannot write to the file,"<<outfile2.str()<<endl;
            exit(1);
        }
    }
}
void peProcess::closeSmallTrimFileHandle(int index){
    if(gp.trim_fq1.rfind(".gz")==gp.trim_fq1.size()-3){
        gzclose(gz_trim_out1[index]);
        gzclose(gz_trim_out2[index]);
    }else{
        fclose(nongz_trim_out1[index]);
        fclose(nongz_trim_out2[index]);
    }
}
void peProcess::closeSmallCleanFileHandle(int index){
    if(gp.clean_fq1.rfind(".gz")==gp.clean_fq1.size()-3){
        gzclose(gz_clean_out1[index]);
        gzclose(gz_clean_out2[index]);
    }else{
        fclose(nongz_clean_out1[index]);
        fclose(nongz_clean_out2[index]);
    }
}
void peProcess::thread_process_reads(int index,int cycle,vector<C_fastq> &fq1s,vector<C_fastq> &fq2s){
	check_disk_available();
    ostringstream outfile1;
    if(gp.clean_fq1.rfind(".gz")==gp.clean_fq1.size()-3){
        outfile1<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<cycle<<".clean.r1.fq.gz";
    }else{
        outfile1<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<cycle<<".clean.r1.fq";
    }
    if(access(outfile1.str().c_str(),0)==-1){
        if(cycle>0){
            closeSmallCleanFileHandle(index);
            create_thread_smallcleanoutputFile(index, cycle);
        }else{
            create_thread_smallcleanoutputFile(index, cycle);
        }
    }
    outfile1.str("");
    if(!gp.trim_fq1.empty()){
        if(gp.trim_fq1.rfind(".gz")==gp.trim_fq1.size()-3){
            outfile1<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<cycle<<".trim.r1.fq.gz";
        }else{
            outfile1<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<cycle<<".trim.r1.fq";
        }
        if(access(outfile1.str().c_str(),0)==-1){
            if(cycle>0){
                closeSmallTrimFileHandle(index);
                create_thread_smalltrimoutputFile(index, cycle);
            }else{
                create_thread_smalltrimoutputFile(index, cycle);
            }
        }
    }
    outfile1.str("");
	vector<C_fastq>  trim_result1;
	vector<C_fastq>  trim_result2;
	vector<C_fastq>  clean_result1;
	vector<C_fastq>  clean_result2;
	PEcalOption* opt2=new PEcalOption();
	opt2->local_fs=&local_fs[index];
	opt2->fq1s=&fq1s;
	opt2->fq2s=&fq2s;
	opt2->trim_result1=&trim_result1;
	opt2->trim_result2=&trim_result2;
	opt2->clean_result1=&clean_result1;
	opt2->clean_result2=&clean_result2;

	if(pair_check==0){
		string readid1=opt2->fq1s->front().seq_id;
		string readid2=opt2->fq2s->front().seq_id;
		if(readid1.size()!=readid2.size()){
			cerr<<"Warning:read ID in fq1 and fq2 seems not in pair, please check the input files if you are not sure"<<endl;
		}else{
			int diff_num(0);
			for(int i=0;i!=readid1.size();i++){
				if(readid1[i]!=readid2[i]){
					diff_num++;
				}
			}
			if(diff_num>1){
				cerr<<"Warning:read ID in fq1 and fq2 seems not in pair, please check the input files if you are not sure"<<endl;
			}
		}
		pair_check++;
	}
	filter_pe_fqs(opt2);		//filter raw fastqs by the given parameters
	delete opt2;
	PEstatOption opt_raw;
	opt_raw.fq1s=&fq1s;
	opt_raw.stat1=&local_raw_stat1[index];
	opt_raw.fq2s=&fq2s;
	opt_raw.stat2=&local_raw_stat2[index];
	stat_pe_fqs(opt_raw,"raw");		//statistic raw fastqs
	//add_raw_trim(local_raw_stat1[index],local_raw_stat2[index],raw_cut.stat1,raw_cut.stat2);
	fq1s.clear();
	fq2s.clear();
	PEstatOption opt_trim,opt_clean;
	if(!gp.trim_fq1.empty()){	//trim means only trim but not discard.
		opt_trim.fq1s=&trim_result1;
		opt_trim.stat1=&local_trim_stat1[index];
		opt_trim.fq2s=&trim_result2;
		opt_trim.stat2=&local_trim_stat2[index];
		stat_pe_fqs(opt_trim,"trim");	//statistic trim fastqs
	}

	//write_m.lock();
	if(!gp.trim_fq1.empty()){
	    if(gp.trimOutGzformat) {
            peWrite(trim_result1, trim_result2, gz_trim_out1[index], gz_trim_out2[index]);    //output trim files
        }else{
            peWrite(trim_result1, trim_result2, nongz_trim_out1[index], nongz_trim_out2[index]);
	    }

		trim_result1.clear();
		trim_result2.clear();
//        closeSmallTrimFileHandle(index);
	}

	if(!gp.clean_fq1.empty()){
		opt_clean.stat1=&local_clean_stat1[index];
		opt_clean.stat2=&local_clean_stat2[index];
        if(gp.cleanOutGzFormat){
            peWrite(clean_result1,clean_result2,gz_clean_out1[index],gz_clean_out2[index]);//output clean files
        }else{
            peWrite(clean_result1,clean_result2,nongz_clean_out1[index],nongz_clean_out2[index]);
        }
//        closeSmallCleanFileHandle(index);
		opt_clean.fq1s=&clean_result1;
		opt_clean.fq2s=&clean_result2;
		stat_pe_fqs(opt_clean,"clean");

		if(gp.is_streaming){
			write_m.lock();
            C_global_variable *tmp_gv=new C_global_variable();
			tmp_gv->fs=*(opt2->local_fs);
			tmp_gv->raw1_stat=*(opt_raw.stat1);
			tmp_gv->raw2_stat=*(opt_raw.stat2);
			//tmp_gv->trim1_stat=local_trim_stat1[index];
			//tmp_gv->trim2_stat=local_trim_stat2[index];
			tmp_gv->clean1_stat=*(opt_clean.stat1);
			tmp_gv->clean2_stat=*(opt_clean.stat2);
			peStreaming_stat(*tmp_gv);
            delete tmp_gv;
			write_m.unlock();
		}

	}
    if(clean_file_readsNum[index].size()<cycle+1){
        clean_file_readsNum[index].emplace_back(opt_clean.fq1s->size());
    }else{
        clean_file_readsNum[index][cycle]+=opt_clean.fq1s->size();
    }
    clean_result1.clear();
    clean_result2.clear();
	check_disk_available();
    //cycle++;
}


void peProcess::merge_stat(){
	for(int i=0;i!=gp.threads_num;i++){
		update_stat(local_raw_stat1[i],local_raw_stat2[i],local_fs[i],"raw");
		if(!gp.trim_fq1.empty()){
			update_stat(local_trim_stat1[i],local_trim_stat2[i],local_fs[i],"trim");
		}
		if(!gp.clean_fq1.empty()){
			update_stat(local_clean_stat1[i],local_clean_stat2[i],local_fs[i],"clean");
		}
	}
}
void peProcess::run_cmd(string cmd){
	if(system(cmd.c_str())==-1){
		cerr<<"Error:when running "<<cmd<<endl;
		exit(1);
	}
}
void peProcess::create_thread_read(int index){
    if(gp.inputGzformat) {
        multi_gzfq1[index] = gzopen((gp.fq1_path).c_str(), "rb");
        if (!multi_gzfq1[index]) {
            cerr << "Error:cannot open the file," << gp.fq1_path << endl;
            exit(1);
        }
        gzsetparams(multi_gzfq1[index], 2, Z_DEFAULT_STRATEGY);
        gzbuffer(multi_gzfq1[index], 2048 * 2048);
        multi_gzfq2[index] = gzopen((gp.fq2_path).c_str(), "rb");
        if (!multi_gzfq2[index]) {
            cerr << "Error:cannot open the file," << gp.fq2_path << endl;
            exit(1);
        }
        gzsetparams(multi_gzfq2[index], 2, Z_DEFAULT_STRATEGY);
        gzbuffer(multi_gzfq2[index], 2048 * 2048);
    }else{
        multi_Nongzfq1[index]=fopen((gp.fq1_path).c_str(), "r");
        if (!multi_Nongzfq1[index]) {
            cerr << "Error:cannot open the file," << gp.fq1_path << endl;
            exit(1);
        }
        multi_Nongzfq2[index]=fopen((gp.fq2_path).c_str(), "r");
        if (!multi_Nongzfq2[index]) {
            cerr << "Error:cannot open the file," << gp.fq2_path << endl;
            exit(1);
        }
    }
}
void* peProcess::sub_thread(int index){
	of_log<<get_local_time()<<"\tthread "<<index<<" start"<<endl;
	create_thread_read(index);
	int thread_cycle=-1;
	char buf1[READBUF],buf2[READBUF];
	C_fastq fastq1,fastq2;
	C_fastq_init(fastq1,fastq2);
	long long file1_line_num(0),file2_line_num(0);
	long long block_line_num1(0),block_line_num2(0);
	int thread_read_block=4*gp.patchSize*patch;
	vector<C_fastq> fq1s,fq2s;
	bool inputGzformat=true;
	gzFile tmpRead=gzopen((gp.fq1_path).c_str(), "rb");
	int spaceNum=0;
    if (gzgets(tmpRead, buf1, READBUF) != NULL){
        string tmpLine(buf1);
        while(isspace(tmpLine[tmpLine.size()-1])){
            spaceNum++;
            tmpLine.erase(tmpLine.size()-1);
        }
    }
    gzclose(tmpRead);
	if(gp.fq1_path.rfind(".gz")==gp.fq1_path.size()-3){
	    inputGzformat=true;
	}else{
	    inputGzformat=false;
	}
	if(inputGzformat) {
        while (1) {

            if (gzgets(multi_gzfq1[index], buf1, READBUF) != NULL) {
                if ((file1_line_num / thread_read_block) % gp.threads_num == index) {
                    block_line_num1++;
                    if (block_line_num1 % 4 == 1) {
                        fastq1.seq_id.assign(buf1);
                        fastq1.seq_id.erase(fastq1.seq_id.size() - spaceNum,spaceNum);
                    }
                    if (block_line_num1 % 4 == 2) {
                        fastq1.sequence.assign(buf1);
                        fastq1.sequence.erase(fastq1.sequence.size() - spaceNum,spaceNum);
                    }
                    if (block_line_num1 % 4 == 0) {
                        fastq1.qual_seq.assign(buf1);
                        fastq1.qual_seq.erase(fastq1.qual_seq.size() - spaceNum,spaceNum);
                    }
                }
                file1_line_num++;
            }
            if (gzgets(multi_gzfq2[index], buf2, READBUF) != NULL) {
                if ((file2_line_num / thread_read_block) % gp.threads_num == index) {
                    block_line_num2++;
                    if (block_line_num2 % 4 == 1) {
                        fastq2.seq_id.assign(buf2);
                        fastq2.seq_id.erase(fastq2.seq_id.size() - spaceNum,spaceNum);
                    } else if (block_line_num2 % 4 == 2) {
                        fastq2.sequence.assign(buf2);
                        fastq2.sequence.erase(fastq2.sequence.size() - spaceNum,spaceNum);
                    } else if (block_line_num2 % 4 == 0) {
                        fastq2.qual_seq.assign(buf2);
                        fastq2.qual_seq.erase(fastq2.qual_seq.size() - spaceNum,spaceNum);
                        fq1s.emplace_back(fastq1);
                        fq2s.emplace_back(fastq2);

                        if (fq1s.size() == gp.patchSize) {
                            if (end_sub_thread == 1) {
                                break;
                            }
                            int tmp_cycle=file2_line_num/(thread_read_block*gp.threads_num);
                            if(tmp_cycle!=thread_cycle && tmp_cycle>0) {
                                addCleanList(thread_cycle, index);
                            }
                            thread_cycle = tmp_cycle;
                            thread_process_reads(index, thread_cycle, fq1s, fq2s);
                            if (index == 0) {
                                of_log << get_local_time() << " processed_reads:\t" << file1_line_num / 4 << endl;
                            }
                        }
                    }
                }
                file2_line_num++;
            } else {
                if (!fq1s.empty()) {
                    if (end_sub_thread == 1) {
                        break;
                    }
                    int tmp_cycle=file1_line_num/(thread_read_block*gp.threads_num);
                    if(tmp_cycle!=thread_cycle && tmp_cycle>0) {
                        addCleanList(thread_cycle, index);
                    }
                    thread_cycle = tmp_cycle;
                    thread_process_reads(index, thread_cycle, fq1s, fq2s);
                    if (limit_end > 0) {
                        break;
                    }
                }
                gzclose(multi_gzfq1[index]);
                gzclose(multi_gzfq2[index]);
                break;
            }
        }
        if(thread_cycle>=0)
            addCleanList(thread_cycle, index);
    }else{
        while (1) {
            if (fgets(buf1, READBUF,multi_Nongzfq1[index]) != NULL) {
                if ((file1_line_num / thread_read_block) % gp.threads_num == index) {
                    block_line_num1++;
                    if (block_line_num1 % 4 == 1) {
                        fastq1.seq_id.assign(buf1);
                        fastq1.seq_id.erase(fastq1.seq_id.size() - 1);
                    }
                    if (block_line_num1 % 4 == 2) {
                        fastq1.sequence.assign(buf1);
                        fastq1.sequence.erase(fastq1.sequence.size() - 1);
                    }
                    if (block_line_num1 % 4 == 0) {
                        fastq1.qual_seq.assign(buf1);
                        fastq1.qual_seq.erase(fastq1.qual_seq.size() - 1);
                    }
                }
                file1_line_num++;
            }
            if (fgets(buf2, READBUF,multi_Nongzfq2[index]) != NULL) {
                if ((file2_line_num / thread_read_block) % gp.threads_num == index) {
                    block_line_num2++;
                    if (block_line_num2 % 4 == 1) {
                        fastq2.seq_id.assign(buf2);
                        fastq2.seq_id.erase(fastq2.seq_id.size() - 1, 1);
                    } else if (block_line_num2 % 4 == 2) {
                        fastq2.sequence.assign(buf2);
                        fastq2.sequence.erase(fastq2.sequence.size() - 1, 1);
                    } else if (block_line_num2 % 4 == 0) {
                        fastq2.qual_seq.assign(buf2);
                        fastq2.qual_seq.erase(fastq2.qual_seq.size() - 1, 1);
                        fq1s.emplace_back(fastq1);
                        fq2s.emplace_back(fastq2);
                        if (fq1s.size() == gp.patchSize) {
                            if (end_sub_thread == 1) {
                                break;
                            }
                            int tmp_cycle=file1_line_num/(thread_read_block*gp.threads_num);
                            if(tmp_cycle!=thread_cycle && tmp_cycle>0) {
                                addCleanList(thread_cycle, index);
                            }
                            thread_cycle = tmp_cycle;
                            thread_process_reads(index, thread_cycle, fq1s, fq2s);
                            if (index == 0) {
                                of_log << get_local_time() << " processed_reads:\t" << file1_line_num / 4 << endl;
                            }
                            if (limit_end > 0) {
                                break;
                            }
                        }
                    }
                }
                file2_line_num++;
            } else {
                if (!fq1s.empty()) {
                    if (end_sub_thread == 1) {
                        break;
                    }
                    int tmp_cycle=file1_line_num/(thread_read_block*gp.threads_num);
                    if(tmp_cycle!=thread_cycle && tmp_cycle>0) {
                        addCleanList(thread_cycle, index);
                    }
                    thread_cycle = tmp_cycle;
                    thread_process_reads(index, thread_cycle, fq1s, fq2s);
                    if (limit_end > 0) {
                        break;
                    }
                }
                gzclose(multi_gzfq1[index]);
                gzclose(multi_gzfq2[index]);
                break;
            }
        }
        if(thread_cycle>=0)
            addCleanList(thread_cycle, index);
	}
	closeSmallCleanFileHandle(index);
	if(!gp.trim_fq1.empty()){
	    closeSmallTrimFileHandle(index);
	}
	check_disk_available();
    sub_thread_done[index]=1;
	of_log<<get_local_time()<<"\tthread "<<index<<" done\t"<<endl;
    return &bq_check;
}
void peProcess::addCleanList(int tmp_cycle,int index){
    ostringstream checkClean1,checkClean2;
    if(gp.cleanOutGzFormat){
        checkClean1<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<tmp_cycle<<".clean.r1.fq.gz";
        checkClean2<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<tmp_cycle<<".clean.r2.fq.gz";
    }else{
        checkClean1<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<tmp_cycle<<".clean.r1.fq";
        checkClean2<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<tmp_cycle<<".clean.r2.fq";
    }
    if(find(readyCleanFiles1[index].begin(),readyCleanFiles1[index].end(),checkClean1.str())==readyCleanFiles1[index].end()) {
        readyCleanFiles1[index].emplace_back(checkClean1.str());
        readyCleanFiles2[index].emplace_back(checkClean2.str());
    }
    if(!gp.trim_fq1.empty()){
        ostringstream readyTrimR1,readyTrimR2;
        if(gp.cleanOutGzFormat){
            readyTrimR1<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<tmp_cycle<<".trim.r1.fq.gz";
            readyTrimR2<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<tmp_cycle<<".trim.r2.fq.gz";
        }else{
            readyTrimR1<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<tmp_cycle<<".trim.r1.fq";
            readyTrimR2<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<index<<"."<<tmp_cycle<<".trim.r2.fq";
        }
        if(find(readyTrimFiles1[index].begin(),readyTrimFiles1[index].end(),readyTrimR1.str())==readyTrimFiles1[index].end()) {
            readyTrimFiles1[index].emplace_back(readyTrimR1.str());
            readyTrimFiles2[index].emplace_back(readyTrimR2.str());
        }
    }
}
void peProcess::catRmFile(int index,int cycle,string type,bool gzFormat){
    ostringstream out_fq1_tmp, out_fq2_tmp;
    if (gzFormat) {
        out_fq1_tmp << gp.output_dir << "/" << tmp_dir << "/thread." << index << "." << cycle
                         << "."<<type<<".r1.fq.gz";
        out_fq2_tmp << gp.output_dir << "/" << tmp_dir << "/thread." << index << "." << cycle
                         << "."<<type<<".r2.fq.gz";
    } else {
        out_fq1_tmp << gp.output_dir << "/" << tmp_dir << "/thread." << index << "." << cycle
                         << "."<<type<<".r1.fq";
        out_fq2_tmp << gp.output_dir << "/" << tmp_dir << "/thread." << index << "." << cycle
                         << "."<<type<<".r2.fq";
    }
    if (access(out_fq1_tmp.str().c_str(), 0) != -1 &&
        access(out_fq2_tmp.str().c_str(), 0) != -1) {
        ostringstream cat_cmd1, cat_cmd2;
        string outFile1,outFile2;
        if(type=="trim"){
            outFile1=gp.trim_fq1;
            outFile2=gp.trim_fq2;
        }else if(type=="clean"){
            outFile1=gp.clean_fq1;
            outFile2=gp.clean_fq2;
        }else{
            cerr<<"Error: code error!"<<endl;
        }
        cat_cmd1 << "cat " << out_fq1_tmp.str() << " >>" << gp.output_dir << "/" << outFile1
                 << ";rm " << out_fq1_tmp.str();
        cat_cmd2 << "cat " << out_fq2_tmp.str() << " >>" << gp.output_dir << "/" << outFile2
                 << ";rm " << out_fq2_tmp.str();
        run_cmd(cat_cmd1.str());
        run_cmd(cat_cmd2.str());
    }
}
void peProcess::catRmFile(vector<int> indexes,int cycle,string type,bool gzFormat){
    ostringstream cmd1,cmd2;
    cmd1<<"cat ";
    cmd2<<"cat ";
    ostringstream rmCmd1,rmCmd2;
    rmCmd1<<"rm ";
    rmCmd2<<"rm ";
    for(vector<int>::iterator ix=indexes.begin();ix!=indexes.end();ix++) {
        ostringstream out_fq1_tmp, out_fq2_tmp;
        if (gzFormat) {
            out_fq1_tmp << gp.output_dir << "/" << tmp_dir << "/thread." << *ix << "." << cycle
                        << "." << type << ".r1.fq.gz";
            out_fq2_tmp << gp.output_dir << "/" << tmp_dir << "/thread." << *ix << "." << cycle
                        << "." << type << ".r2.fq.gz";
        } else {
            out_fq1_tmp << gp.output_dir << "/" << tmp_dir << "/thread." << *ix << "." << cycle
                        << "." << type << ".r1.fq";
            out_fq2_tmp << gp.output_dir << "/" << tmp_dir << "/thread." << *ix << "." << cycle
                        << "." << type << ".r2.fq";
        }
        if (access(out_fq1_tmp.str().c_str(), 0) != -1 &&
            access(out_fq2_tmp.str().c_str(), 0) != -1) {
            ostringstream cat_cmd1, cat_cmd2;
            cmd1 << out_fq1_tmp.str() << " ";
            cmd2 << out_fq2_tmp.str() << " ";
            rmCmd1<<out_fq1_tmp.str() << " ";
            rmCmd2<<out_fq2_tmp.str() << " ";
        }
    }
    string outFile1,outFile2;
    if(type=="trim"){
        outFile1=gp.trim_fq1;
        outFile2=gp.trim_fq2;
    }else if(type=="clean"){
        outFile1=gp.clean_fq1;
        outFile2=gp.clean_fq2;
    }else{
        cerr<<"Error: code error!"<<endl;
    }
    cmd1<<" >>"<<gp.output_dir<<"/"<<outFile1;
    cmd2<<" >>"<<gp.output_dir<<"/"<<outFile2;
    if(indexes.size()>0) {
        bool subThreadAllDone = true;
        for (int i = 0; i < gp.threads_num; i++) {
            if (sub_thread_done[i]!=1) {
                subThreadAllDone = false;
                break;
            }
        }
//        if(subThreadAllDone){
            thread t1=thread(bind(&peProcess::run_cmd,this,cmd1.str()));
            thread t2=thread(bind(&peProcess::run_cmd,this,cmd2.str()));
            t1.join();
            t2.join();
            thread t3=thread(bind(&peProcess::run_cmd,this,rmCmd1.str()));
            thread t4=thread(bind(&peProcess::run_cmd,this,rmCmd2.str()));
            t3.join();
            t4.join();
//        }else {
//            run_cmd(cmd1.str());
//            run_cmd(cmd2.str());
//            run_cmd(rmCmd1.str());
//            run_cmd(rmCmd2.str());
//        }
    }
}
void peProcess::extractReadsToFile(int cycle,int thread_index,int fileReadsNum,int& reads_number,string position,int& output_index,bool gzFormat){
    ostringstream outFile1,outFile2;
    outFile1<<gp.output_dir<<"/split."<<output_index<<"."<<gp.clean_fq1;
    outFile2<<gp.output_dir<<"/split."<<output_index<<"."<<gp.clean_fq2;
    ostringstream cleanSmallFile1,cleanSmallFile2;
    bool exists=0;
    if(access(outFile1.str().c_str(),0)!=-1){
        exists=1;
    }
    if(gzFormat){
        cleanSmallFile1<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<thread_index<<"."<<cycle<<".clean.r1.fq.gz";
        cleanSmallFile2<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<thread_index<<"."<<cycle<<".clean.r2.fq.gz";
        ostringstream tmpOut1,tmpOut2;
        if(fileReadsNum<=gp.cleanOutSplit-reads_number){
            if(!exists){
                string sh1="mv "+cleanSmallFile1.str()+" "+outFile1.str();
                string sh2="mv "+cleanSmallFile2.str()+" "+outFile2.str();
                run_cmd(sh1);
                run_cmd(sh2);
                reads_number+=fileReadsNum;
                return;
            }else{
                string backup=outFile1.str()+".backup";
                string backup2=outFile2.str()+".backup";
                tmpOut1<<gp.output_dir<<"/split.tmpR1."<<gp.clean_fq1;
                tmpOut2<<gp.output_dir<<"/split.tmpR2."<<gp.clean_fq2;
                string sh1="mv "+cleanSmallFile1.str()+" "+tmpOut1.str()+";mv "+outFile1.str()+" "+backup+";cat "+backup+" "+tmpOut1.str()+" >"+outFile1.str()+";rm "+backup+" "+tmpOut1.str();
                string sh2="mv "+cleanSmallFile2.str()+" "+tmpOut2.str()+";mv "+outFile2.str()+" "+backup+";cat "+backup+" "+tmpOut2.str()+" >"+outFile2.str()+";rm "+backup+" "+tmpOut2.str();
                run_cmd(sh1);
                run_cmd(sh2);
                reads_number+=fileReadsNum;
                return;
            }
        }
        gzFile gzCleanSmall1,gzCleanSmall2;
        gzCleanSmall1=gzopen(cleanSmallFile1.str().c_str(),"rb");
        gzCleanSmall2=gzopen(cleanSmallFile2.str().c_str(),"rb");
        gzFile splitGzFq1, splitGzFq2;

        if(!exists) {
            splitGzFq1 = gzopen(outFile1.str().c_str(), "wb");
            gzsetparams(splitGzFq1, 2, Z_DEFAULT_STRATEGY);
            gzbuffer(splitGzFq1, 1024 * 1024 * 10);
            splitGzFq2 = gzopen(outFile2.str().c_str(), "wb");
            gzsetparams(splitGzFq2, 2, Z_DEFAULT_STRATEGY);
            gzbuffer(splitGzFq2, 1024 * 1024 * 10);
        }else{

            tmpOut1<<gp.output_dir<<"/split.tmpR1."<<gp.clean_fq1;
            tmpOut2<<gp.output_dir<<"/split.tmpR2."<<gp.clean_fq2;
            splitGzFq1 = gzopen(tmpOut1.str().c_str(), "wb");
            splitGzFq2 = gzopen(tmpOut2.str().c_str(), "wb");
        }
        char buf1[READBUF],buf2[READBUF];
        int tmp_index=output_index;
        int lineNum=0;
        int readsNum=0;
        int tmpReadsNumber=reads_number;
        while(gzgets(gzCleanSmall1,buf1,READBUF)!=NULL){
            string line(buf1);
            lineNum++;
            gzwrite(splitGzFq1,line.c_str(),line.size());
            if(lineNum%4==0){
                readsNum++;
                reads_number++;
                if(reads_number==gp.cleanOutSplit){
                    reads_number = 0;
                    if(exists){
                        string backup=outFile1.str()+".backup";
                        outFile1.str("");
                        outFile1<<gp.output_dir<<"/split."<<output_index<<"."<<gp.clean_fq1;
                        string runSh="mv "+outFile1.str()+" "+backup+";cat "+backup+" "+tmpOut1.str()+" >"+outFile1.str()+";rm "+backup+" "+tmpOut1.str();
                        run_cmd(runSh);
                    }else {
                        gzclose(splitGzFq1);
                    }
                    output_index++;
                    outFile1.str("");
                    outFile1 << gp.output_dir << "/split." << output_index << "." << gp.clean_fq1;
                    splitGzFq1 = gzopen(outFile1.str().c_str(), "wb");
                    gzsetparams(splitGzFq1, 2, Z_DEFAULT_STRATEGY);
                    gzbuffer(splitGzFq1, 1024 * 1024 * 10);
                    readsNum++;
                }
            }

        }
        lineNum=0;
        readsNum=0;
        output_index=tmp_index;
        reads_number=tmpReadsNumber;
        while(gzgets(gzCleanSmall2,buf2,READBUF)!=NULL){
            string line(buf2);
            lineNum++;
            gzwrite(splitGzFq2,line.c_str(),line.size());
            if(lineNum%4==0){
                readsNum++;
                reads_number++;
                if(reads_number==gp.cleanOutSplit){
                    reads_number=0;
                    if(exists){
                        string backup2=outFile2.str()+".backup";
                        outFile2.str("");
                        outFile2<<gp.output_dir<<"/split."<<output_index<<"."<<gp.clean_fq2;
                        string runSh2="mv "+outFile2.str()+" "+backup2+";cat "+backup2+" "+tmpOut2.str()+" >"+outFile2.str()+";rm "+backup2+" "+tmpOut2.str();
                        run_cmd(runSh2);
                    }else {
                        gzclose(splitGzFq2);
                    }
                    output_index++;
                    outFile2.str("");
                    outFile2 << gp.output_dir << "/split." << output_index << "." << gp.clean_fq2;
                    splitGzFq2 = gzopen(outFile2.str().c_str(), "wb");
                    gzsetparams(splitGzFq2, 2, Z_DEFAULT_STRATEGY);
                    gzbuffer(splitGzFq2, 1024 * 1024 * 10);
                    readsNum++;
                }
            }
        }


        gzclose(splitGzFq1);
        gzclose(splitGzFq2);
        gzclose(gzCleanSmall1);
        gzclose(gzCleanSmall2);
    }else{
        cleanSmallFile1<<gp.output_dir<<"/thread."<<thread_index<<"."<<cycle<<".clean.r1.fq";
        cleanSmallFile1<<gp.output_dir<<"/thread."<<thread_index<<"."<<cycle<<".clean.r2.fq";
        ostringstream tmpOut1,tmpOut2;
        if(fileReadsNum<=gp.cleanOutSplit-reads_number){
            if(!exists){
                string sh1="mv "+cleanSmallFile1.str()+" "+outFile1.str();
                string sh2="mv "+cleanSmallFile2.str()+" "+outFile2.str();
                run_cmd(sh1);
                run_cmd(sh2);
                return;
            }else{
                string backup=outFile1.str()+".backup";
                string backup2=outFile2.str()+".backup";
                tmpOut1<<gp.output_dir<<"/split.tmpR1."<<gp.clean_fq1;
                tmpOut2<<gp.output_dir<<"/split.tmpR2."<<gp.clean_fq2;
                string sh1="mv "+cleanSmallFile1.str()+" "+tmpOut1.str()+";mv "+outFile1.str()+" "+backup+";cat "+backup+" "+tmpOut1.str()+" >"+outFile1.str()+";rm "+backup+" "+tmpOut1.str();
                string sh2="mv "+cleanSmallFile2.str()+" "+tmpOut2.str()+";mv "+outFile2.str()+" "+backup+";cat "+backup+" "+tmpOut2.str()+" >"+outFile2.str()+";rm "+backup+" "+tmpOut2.str();
                run_cmd(sh1);
                run_cmd(sh2);
                return;
            }
        }
        FILE* nongzCleanSmall1,*nongzCleanSmall2;
        nongzCleanSmall1=fopen(cleanSmallFile1.str().c_str(),"r");
        nongzCleanSmall2=fopen(cleanSmallFile2.str().c_str(),"r");
        FILE* splitNonGzFq1,*splitNonGzFq2;
        if(!exists) {
            splitNonGzFq1=fopen(outFile1.str().c_str(),"w");
            splitNonGzFq2=fopen(outFile2.str().c_str(),"w");
        }else{
            tmpOut1<<gp.output_dir<<"/split.tmpR1."<<gp.clean_fq1;
            tmpOut2<<gp.output_dir<<"/split.tmpR2."<<gp.clean_fq2;
            splitNonGzFq1=fopen(tmpOut1.str().c_str(),"w");
            splitNonGzFq2=fopen(tmpOut2.str().c_str(),"w");
        }


//        splitNonGzFq1=fopen(outFile1.str().c_str(),"w");
//        splitNonGzFq2=fopen(outFile2.str().c_str(),"w");
        char buf1[READBUF],buf2[READBUF];
        int tmp_index=output_index;
        int lineNum=0;
        int readsNum=0;
        int tmpReadsNumber=reads_number;
        while(fgets(buf1,READBUF,nongzCleanSmall1)!=NULL){
            lineNum++;
            reads_number++;
            string line(buf1);
            fputs(line.c_str(),splitNonGzFq1);
            if(lineNum%4==0) {
                readsNum++;
                reads_number++;
                if (readsNum == gp.cleanOutSplit) {
                    reads_number = 0;
                    if(exists){
                        string backup=outFile1.str()+".backup";
                        outFile1.str("");
                        outFile1<<gp.output_dir<<"/split."<<output_index<<"."<<gp.clean_fq1;
                        string runSh="mv "+outFile1.str()+" "+backup+";cat "+backup+" "+tmpOut1.str()+" >"+outFile1.str()+";rm "+backup+" "+tmpOut1.str();
                        run_cmd(runSh);
                    }else {
                        fclose(splitNonGzFq1);
                    }
                    output_index++;
                    outFile1.str("");
                    outFile1 << gp.output_dir << "/split." << output_index << "." << gp.clean_fq1;
                    splitNonGzFq1 = fopen(outFile1.str().c_str(), "w");
                    readsNum++;
                }
            }
        }
        lineNum=0;
        readsNum=0;
        output_index=tmp_index;
        reads_number=tmpReadsNumber;
        while(fgets(buf1,READBUF,nongzCleanSmall1)!=NULL){
            lineNum++;
            reads_number++;
            string line(buf1);
            fputs(line.c_str(),splitNonGzFq1);
            if(lineNum%4==0) {
                readsNum++;
                reads_number++;
                if (readsNum == gp.cleanOutSplit) {
                    reads_number = 0;
                    if(exists){
                        string backup=outFile2.str()+".backup";
                        outFile2.str("");
                        outFile2<<gp.output_dir<<"/split."<<output_index<<"."<<gp.clean_fq2;
                        string runSh="mv "+outFile2.str()+" "+backup+";cat "+backup+" "+tmpOut2.str()+" >"+outFile2.str()+";rm "+backup+" "+tmpOut2.str();
                        run_cmd(runSh);
                    }else {
                        fclose(splitNonGzFq2);
                    }
                    output_index++;
                    outFile2.str("");
                    outFile2 << gp.output_dir << "/split." << output_index << "." << gp.clean_fq2;
                    splitNonGzFq2 = fopen(outFile2.str().c_str(), "w");
                    readsNum++;
                }
            }
        }
        fclose(splitNonGzFq1);
        fclose(splitNonGzFq2);
        fclose(nongzCleanSmall1);
        fclose(nongzCleanSmall2);
    }
    string rmCmd1="rm "+cleanSmallFile1.str();
    string rmCmd2="rm "+cleanSmallFile2.str();
    run_cmd(rmCmd1);
    run_cmd(rmCmd2);
}
void peProcess::extractReadsToFile(int cycle,int thread_index,int reads_number,string position,bool gzFormat){
    ostringstream cleanSmallFile1,cleanSmallFile2;

    if(gzFormat){
        cleanSmallFile1<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<thread_index<<"."<<cycle<<".clean.r1.fq.gz";
        cleanSmallFile2<<gp.output_dir<<"/"<<tmp_dir<<"/thread."<<thread_index<<"."<<cycle<<".clean.r2.fq.gz";
        gzFile gzCleanSmall1,gzCleanSmall2;
        gzCleanSmall1=gzopen(cleanSmallFile1.str().c_str(),"rb");
        gzCleanSmall2=gzopen(cleanSmallFile2.str().c_str(),"rb");

        gzFile splitGzFq1,splitGzFq2;
        string out1=gp.output_dir+"/"+gp.clean_fq1;
        string out2=gp.output_dir+"/"+gp.clean_fq2;
        splitGzFq1=gzopen(out1.c_str(),"ab");
        gzsetparams(splitGzFq1, 2, Z_DEFAULT_STRATEGY);
        gzbuffer(splitGzFq1,1024*1024*10);
        splitGzFq2=gzopen(out2.c_str(),"ab");
        gzsetparams(splitGzFq2, 2, Z_DEFAULT_STRATEGY);
        gzbuffer(splitGzFq2,1024*1024*10);
        char buf1[READBUF],buf2[READBUF];

        if(position=="head"){
            for(int i=0;i<reads_number*4;i++){
                if(gzgets(gzCleanSmall1,buf1,READBUF)!=NULL){
                    string line(buf1);
                    gzwrite(splitGzFq1,line.c_str(),line.size());
                }
                if(gzgets(gzCleanSmall2,buf2,READBUF)!=NULL){
                    string line(buf2);
                    gzwrite(splitGzFq2,line.c_str(),line.size());
                }
            }
            gzclose(gzCleanSmall1);
            gzclose(gzCleanSmall2);
            gzclose(splitGzFq1);
            gzclose(splitGzFq2);
        }
    }else{
        cleanSmallFile1<<gp.output_dir<<"/thread."<<thread_index<<"."<<cycle<<".clean.r1.fq";
        cleanSmallFile2<<gp.output_dir<<"/thread."<<thread_index<<"."<<cycle<<".clean.r2.fq";
        FILE* nongzCleanSmall1,*nongzCleanSmall2;
        nongzCleanSmall1=fopen(cleanSmallFile1.str().c_str(),"r");
        nongzCleanSmall2=fopen(cleanSmallFile2.str().c_str(),"r");

        FILE* splitNonGzFq1,*splitNonGzFq2;
        splitNonGzFq1=fopen(gp.clean_fq1.c_str(),"a");
        splitNonGzFq2=fopen(gp.clean_fq2.c_str(),"a");
        char buf1[READBUF],buf2[READBUF];

        if(position=="head"){
            for(int i=0;i<reads_number*4;i++){
                if(fgets(buf1,READBUF,nongzCleanSmall1)!=NULL){
                    string line(buf1);
                    fputs(line.c_str(),splitNonGzFq1);
                }
                if(fgets(buf2,READBUF,nongzCleanSmall2)!=NULL){
                    string line(buf2);
                    fputs(line.c_str(),splitNonGzFq2);
                }
            }
            fclose(splitNonGzFq1);
            fclose(splitNonGzFq2);
            fclose(nongzCleanSmall1);
            fclose(nongzCleanSmall2);
        }
    }
    string rmCmd1="rm "+cleanSmallFile1.str();
    string rmCmd2="rm "+cleanSmallFile2.str();
    run_cmd(rmCmd1);
    run_cmd(rmCmd2);
}
void* peProcess::smallFilesProcess(){
    if(!gp.trim_fq1.empty()){
        string trim_file1,trim_file2;
        trim_file1=gp.output_dir+"/"+gp.trim_fq1;
        trim_file2=gp.output_dir+"/"+gp.trim_fq2;
        if(check_gz_empty(trim_file1)==1){
            string rm_cmd="rm "+trim_file1;
            system(rm_cmd.c_str());
        }
        if(check_gz_empty(trim_file2)==1){
            string rm_cmd="rm "+trim_file2;
            system(rm_cmd.c_str());
        }
    }
    if(!gp.clean_fq1.empty()){
        string clean_file1,clean_file2;
        clean_file1=gp.output_dir+"/"+gp.clean_fq1;
        clean_file2=gp.output_dir+"/"+gp.clean_fq2;
        if(check_gz_empty(clean_file1)==1){
            string rm_cmd="rm "+clean_file1;
            system(rm_cmd.c_str());
        }
        if(check_gz_empty(clean_file2)==1){
            string rm_cmd="rm "+clean_file2;
            system(rm_cmd.c_str());
        }
    }
    int sleepTime=5;
    int outputFileIndex=0;
    int cur_avaliable_total_reads_number = 0;
    if(gp.cleanOutSplit>0){

        int lastUncompleteFileReadsNumber=0;
        int sticky_tail_reads_number = 0;
        string tidyFile1=gp.output_dir+"/split.0."+gp.clean_fq1;
        string tidyFile2=gp.output_dir+"/split.0."+gp.clean_fq1;
        ostringstream rmCmd1,rmCmd2;
        rmCmd1<<"rm "<<gp.output_dir << "/split.*.fq*";
        rmCmd2<<"rm "<<gp.output_dir << "/split.*.fq*";

        if(access(tidyFile1.c_str(),0)!=-1){
            run_cmd(rmCmd1.str());
        }
        if(access(tidyFile2.c_str(),0)!=-1){
            run_cmd(rmCmd2.str());
        }

        while(1) {
            bool subThreadAllDone = true;
            for (int i = 0; i < gp.threads_num; i++) {
                if (sub_thread_done[i] !=1) {
                    subThreadAllDone = false;
                    break;
                }
            }
            if (subThreadAllDone) {
                int ready_cycles = 0;
                for (int i = 0; i < gp.threads_num; i++) {
                    if (readyCleanFiles1[i].size() > ready_cycles) {
                        ready_cycles = readyCleanFiles1[i].size();
                    }
                }
                string readyR1TrimSmallFiles,readyR2TrimSmallFiles;
                for (int cycle = cur_cat_cycle; cycle < ready_cycles; cycle++) {
                    for (int i = 0; i < gp.threads_num; i++) {
                        if (cycle==ready_cycles-1 && readyCleanFiles1[i].size() < ready_cycles) {
                            break;
                        }
                        if (!gp.trim_fq1.empty()) {
                            if(gp.trimOutGzformat){
                                readyR1TrimSmallFiles+=" "+gp.output_dir+"/"+tmp_dir+"/thread."+to_string(i)+"."+to_string(cycle)+".trim.r1.fq.gz";
                                readyR2TrimSmallFiles+=" "+gp.output_dir+"/"+tmp_dir+"/thread."+to_string(i)+"."+to_string(cycle)+".trim.r2.fq.gz";
                            }else{
                                readyR1TrimSmallFiles+=" "+gp.output_dir+"/"+tmp_dir+"/thread."+to_string(i)+"."+to_string(cycle)+".trim.r1.fq";
                                readyR2TrimSmallFiles+=" "+gp.output_dir+"/"+tmp_dir+"/thread."+to_string(i)+"."+to_string(cycle)+".trim.r2.fq";
                            }
                        }
                        int fileReadsNum=clean_file_readsNum[i][cycle];

                        extractReadsToFile(cycle, i, fileReadsNum,lastUncompleteFileReadsNumber, "head",
                                               outputFileIndex,
                                               gp.cleanOutGzFormat);
                    }
                }
                if (!gp.trim_fq1.empty()) {
                    string mergeShell1="cat "+readyR1TrimSmallFiles+" >>"+gp.output_dir+"/"+gp.trim_fq1;
                    string mergeShell2="cat "+readyR2TrimSmallFiles+" >>"+gp.output_dir+"/"+gp.trim_fq2;
                    thread t1=thread(bind(&peProcess::run_cmd,this,mergeShell1));
                    thread t2=thread(bind(&peProcess::run_cmd,this,mergeShell2));
                    t1.join();
                    t2.join();
                    mergeShell1="rm "+readyR1TrimSmallFiles;
                    mergeShell2="rm "+readyR2TrimSmallFiles;
                    t1=thread(bind(&peProcess::run_cmd,this,mergeShell1));
                    t2=thread(bind(&peProcess::run_cmd,this,mergeShell2));
                    t1.join();
                    t2.join();
                }
                break;
            }
            sleep(sleepTime);
        }
    }else {
        unsigned long long total_merged_reads_number=0;

        while (1) {  //merge small files by input order
            bool subThreadAllDone = true;
            while(1){
                subThreadAllDone = true;
                usleep(5000000);
                for (int i = 0; i < gp.threads_num; i++) {
                    if (sub_thread_done[i]!=1) {
                        subThreadAllDone = false;
                        break;
                    }
                }
                if(subThreadAllDone){
                    break;
                }
            }
            if (subThreadAllDone) {
                int ready_cycles = 0;
                for (int i = 0; i < gp.threads_num; i++) {
                    if (readyCleanFiles1[i].size() > ready_cycles) {
                        ready_cycles = readyCleanFiles1[i].size();
                    }
                }
                string readyR1SmallFiles,readyR2SmallFiles;
                string readyR1TrimSmallFiles,readyR2TrimSmallFiles;
                for (int cycle = cur_cat_cycle; cycle < ready_cycles; cycle++) {
//                    reArrangeReads(cycle,gp.cleanOutGzFormat,false,outputFileIndex,cur_avaliable_total_reads_number);
                    vector<int> readyCatFiles;
                    for (int i = 0; i < gp.threads_num; i++) {
                        if (cycle==ready_cycles-1 && readyCleanFiles1[i].size() < ready_cycles) {
                            break;
                        }
                        if (!gp.trim_fq1.empty()) {
                            if(gp.trimOutGzformat){
                                readyR1TrimSmallFiles+=" "+gp.output_dir+"/"+tmp_dir+"/thread."+to_string(i)+"."+to_string(cycle)+".trim.r1.fq.gz";
                                readyR2TrimSmallFiles+=" "+gp.output_dir+"/"+tmp_dir+"/thread."+to_string(i)+"."+to_string(cycle)+".trim.r2.fq.gz";
                            }else{
                                readyR1TrimSmallFiles+=" "+gp.output_dir+"/"+tmp_dir+"/thread."+to_string(i)+"."+to_string(cycle)+".trim.r1.fq";
                                readyR2TrimSmallFiles+=" "+gp.output_dir+"/"+tmp_dir+"/thread."+to_string(i)+"."+to_string(cycle)+".trim.r2.fq";
                            }
                        }
                        if (!gp.clean_fq1.empty()) {
                            if(gp.cleanOutGzFormat){
                                readyR1SmallFiles+=" "+gp.output_dir+"/"+tmp_dir+"/thread."+to_string(i)+"."+to_string(cycle)+".clean.r1.fq.gz";
                                readyR2SmallFiles+=" "+gp.output_dir+"/"+tmp_dir+"/thread."+to_string(i)+"."+to_string(cycle)+".clean.r2.fq.gz";
                            }else{
                                readyR1SmallFiles+=" "+gp.output_dir+"/"+tmp_dir+"/thread."+to_string(i)+"."+to_string(cycle)+".clean.r1.fq";
                                readyR2SmallFiles+=" "+gp.output_dir+"/"+tmp_dir+"/thread."+to_string(i)+"."+to_string(cycle)+".clean.r2.fq";
                            }
                        }
//                        total_merged_reads_number+=clean_file_readsNum[i][cycle];
//                        if(!gp.total_reads_num_random && gp.total_reads_num>0){
//                            if(total_merged_reads_number>gp.total_reads_num){
//                                end_sub_thread=1;
//                                catRmFile(readyCatFiles, cycle, "clean", gp.cleanOutGzFormat);
//                                //output some reads to the final file
//                                int toBeOutputReadsNumber=gp.total_reads_num-(total_merged_reads_number-clean_file_readsNum[i][cycle]);
//                                extractReadsToFile(cycle,i,toBeOutputReadsNumber,"head",gp.cleanOutGzFormat);
//                                rmTmpFiles();
//                                return &bq_check;
//                            }
//                        }
//                        readyCatFiles.emplace_back(i);
                    }
//                    if (!gp.trim_fq1.empty()) {
//                        catRmFile(readyCatFiles, cycle, "trim", gp.trimOutGzformat);
//                    }
//                    if (!gp.clean_fq1.empty()) {
//                        catRmFile(readyCatFiles, cycle, "clean", gp.cleanOutGzFormat);
//                    }
                }
                if (!gp.trim_fq1.empty()) {
                    string mergeShell1="cat "+readyR1TrimSmallFiles+" >"+gp.output_dir+"/"+gp.trim_fq1;
                    string mergeShell2="cat "+readyR2TrimSmallFiles+" >"+gp.output_dir+"/"+gp.trim_fq2;
                    thread t1=thread(bind(&peProcess::run_cmd,this,mergeShell1));
                    thread t2=thread(bind(&peProcess::run_cmd,this,mergeShell2));
                    t1.join();
                    t2.join();
                    mergeShell1="rm "+readyR1TrimSmallFiles;
                    mergeShell2="rm "+readyR2TrimSmallFiles;
                    t1=thread(bind(&peProcess::run_cmd,this,mergeShell1));
                    t2=thread(bind(&peProcess::run_cmd,this,mergeShell2));
                    t1.join();
                    t2.join();
                }
                if (!gp.clean_fq1.empty()) {
                    string mergeShell1="cat "+readyR1SmallFiles+" >"+gp.output_dir+"/"+gp.clean_fq1;
                    string mergeShell2="cat "+readyR2SmallFiles+" >"+gp.output_dir+"/"+gp.clean_fq2;
                    thread t1=thread(bind(&peProcess::run_cmd,this,mergeShell1));
                    thread t2=thread(bind(&peProcess::run_cmd,this,mergeShell2));
                    t1.join();
                    t2.join();
                    mergeShell1="rm "+readyR1SmallFiles;
                    mergeShell2="rm "+readyR2SmallFiles;
                    t1=thread(bind(&peProcess::run_cmd,this,mergeShell1));
                    t2=thread(bind(&peProcess::run_cmd,this,mergeShell2));
                    t1.join();
                    t2.join();
                }
                break;
            }
            int ready_cycles = readyCleanFiles1[0].size();
            for (int i = 1; i < gp.threads_num; i++) {
                if (readyCleanFiles1[i].size() < ready_cycles) {
                    ready_cycles = readyCleanFiles1[i].size();
                }
            }
            for (int cycle = cur_cat_cycle; cycle < ready_cycles; cycle++) {
                if (!gp.trim_fq1.empty()) {
                    for (int i = 0; i < gp.threads_num; i++) {
                        catRmFile(i, cycle, "trim", gp.trimOutGzformat);
                    }
                }
                if (!gp.clean_fq1.empty()) {
//                    reArrangeReads(cycle,gp.cleanOutGzFormat,false,outputFileIndex,cur_avaliable_total_reads_number);
                    vector<int> readyCatFiles;
                    for (int i = 0; i < gp.threads_num; i++) {
                        total_merged_reads_number+=clean_file_readsNum[i][cycle];
                        if(!gp.total_reads_num_random && gp.total_reads_num>0){
                            if(total_merged_reads_number>gp.total_reads_num){
                                end_sub_thread=1;
                                catRmFile(readyCatFiles, cycle, "clean", gp.cleanOutGzFormat);
                                //output some reads to the final file
                                int toBeOutputReadsNumber=gp.total_reads_num-(total_merged_reads_number-clean_file_readsNum[i][cycle]);
                                extractReadsToFile(cycle,i,toBeOutputReadsNumber,"head",gp.cleanOutGzFormat);

                                rmTmpFiles();
                                return &bq_check;
                            }
                        }
                        readyCatFiles.emplace_back(i);
                    }
                    catRmFile(readyCatFiles, cycle, "clean", gp.cleanOutGzFormat);
                }
            }
            if (ready_cycles > 0)
                cur_cat_cycle = ready_cycles;
            sleep(sleepTime);
//        if(sleepTime<60)
//            sleepTime+=10;
        }
    }
    return &bq_check;
}
void peProcess::rmTmpFiles(){
    string cmd="rm -f "+gp.output_dir+"/"+tmp_dir+"/*fq*";
    run_cmd(cmd);
}
void peProcess::process(){
	string mkdir_str="mkdir -p "+gp.output_dir;
	if(system(mkdir_str.c_str())==-1){
		cerr<<"Error:mkdir fail"<<endl;
		exit(1);
	}
	of_log.open(gp.log.c_str());
	if(!of_log){
		cerr<<"Error:cannot open such file,"<<gp.log<<endl;
		exit(1);
	}
	of_log<<get_local_time()<<"\tAnalysis start!"<<endl;
    make_tmpDir();
	thread t_array[gp.threads_num];
	//thread read_monitor(bind(&peProcess::monitor_read_thread,this));
	//sleep(10);
	for(int i=0;i<gp.threads_num;i++){
		//t_array[i]=thread(bind(&peProcess::sub_thread_nonssd_multiOut,this,i));
		t_array[i]=thread(bind(&peProcess::sub_thread,this,i));
	}
    thread catFiles = thread(bind(&peProcess::smallFilesProcess, this));


	for(int i=0;i<gp.threads_num;i++){
		t_array[i].join();
	}
    catFiles.join();
	check_disk_available();
    if(gp.total_reads_num_random==true && gp.total_reads_num>0){
        run_extract_random();
    }
	merge_stat();
	print_stat();
    remove_tmpDir();
	check_disk_available();
	of_log<<get_local_time()<<"\tAnalysis accomplished!"<<endl;
	of_log.close();
}
void peProcess::run_extract_random(){
    long long total_clean_reads(0);
    for(int i=0;i!=gp.threads_num;i++){
        total_clean_reads+=local_clean_stat1[i].gs.reads_number;
    }
    if(gp.f_total_reads_ratio>0){
        if(gp.f_total_reads_ratio>=1){
            cerr<<"Error:the ratio extract from clean fq file should not be more than 1"<<endl;
            exit(1);
        }
        if(gp.l_total_reads_num>0){
            cerr<<"Error:reads number and ratio should not be both assigned at the same time"<<endl;
            exit(1);
        }
        gp.l_total_reads_num=total_clean_reads*gp.f_total_reads_ratio;
    }
    if(total_clean_reads<gp.l_total_reads_num){
        cerr<<"Warning:the reads number in clean fastq file("<<total_clean_reads<<") is less than you assigned to output("<<gp.l_total_reads_num<<")"<<endl;
    }
    //cout<<gp.l_total_reads_num<<"\t"<<total_clean_reads<<endl;
    //vector<int> include_threads;
    int interval=total_clean_reads/gp.l_total_reads_num;
    string in1=gp.output_dir+"/"+gp.clean_fq1;
    string in2=gp.output_dir+"/"+gp.clean_fq2;
    string out1=gp.cleanOutGzFormat?gp.output_dir+"/cleanRandomExtractReads.r1.fq.gz":gp.output_dir+"/cleanRandomExtractReads.r1.fq";
    string out2=gp.cleanOutGzFormat?gp.output_dir+"/cleanRandomExtractReads.r2.fq.gz":gp.output_dir+"/cleanRandomExtractReads.r2.fq";
    if(gp.threads_num>1){
        thread extract1(bind(&peProcess::sub_extract,this,in1,interval,out1));
        thread extract2(bind(&peProcess::sub_extract,this,in2,interval,out2));
        extract1.join();
        extract2.join();
    }else{
        sub_extract(in1,interval,out1);
        sub_extract(in2,interval,out2);
    }
    string cmd1="mv "+gp.output_dir+"/"+gp.clean_fq1+" "+gp.output_dir+"/total."+gp.clean_fq1;
    cmd1+="; mv "+out1+" "+gp.output_dir+"/"+gp.clean_fq1;
    string cmd2="mv "+gp.output_dir+"/"+gp.clean_fq2+" "+gp.output_dir+"/total."+gp.clean_fq2;
    cmd2+="; mv "+out2+" "+gp.output_dir+"/"+gp.clean_fq2;
    run_cmd(cmd1);
    run_cmd(cmd2);
}
void* peProcess::sub_extract(string in,int mo,string out){
    int lineNum=0;
    int readsNum=0;
    if(gp.cleanOutGzFormat){
        gzFile subFile=gzopen(out.c_str(),"wb");
        gzsetparams(subFile, 2, Z_DEFAULT_STRATEGY);
        gzbuffer(subFile,2048*2048);
        gzFile cleanFile=gzopen(in.c_str(),"rb");
        char buf[READBUF];
        while(gzgets(cleanFile,buf,READBUF)){
            if(lineNum%(4*mo)>=0 && lineNum%(4*mo)<=3){
                string line(buf);
                gzwrite(subFile,line.c_str(),line.size());
                readsNum++;
                if(readsNum/4>=gp.l_total_reads_num && readsNum%4==0){
                    break;
                }
            }
            lineNum++;
        }
        gzclose(subFile);
        gzclose(cleanFile);
    }else{
        FILE* subFile=fopen(out.c_str(),"w");
        FILE* cleanFile=fopen(in.c_str(),"r");
        char buf[READBUF];
        while(fgets(buf,READBUF,cleanFile)){
            if(lineNum%(4*mo)>=0 && lineNum%(4*mo)<=3){
                string line(buf);
                fputs(line.c_str(),subFile);
                readsNum++;
                if(readsNum/4>=gp.l_total_reads_num && readsNum%4==0){
                    break;
                }
            }
            lineNum++;
        }
        fclose(subFile);
        fclose(cleanFile);
    }
    return &bq_check;
}
void peProcess::remove_tmpDir(){
	string rm_dir=gp.output_dir+"/"+tmp_dir;
	int iter(0);
	while(1){
		DIR* tmpdir;
		struct dirent* ptr;
		tmpdir=opendir(rm_dir.c_str());
		bool empty_flag=true;
		if(tmpdir==NULL){
			cerr<<"Error:open directory error,"<<rm_dir<<endl;
			exit(1);
		}
		while((ptr=readdir(tmpdir))!=NULL){
			if(strcmp(ptr->d_name,"..")!=0 && strcmp(ptr->d_name,".")!=0){
				empty_flag=false;
				break;
			}
		}
		closedir(tmpdir);
		if(empty_flag){
			string remove="rm -r "+rm_dir;
			if(system(remove.c_str())==-1){
				cerr<<"Error:rmdir error,"<<remove<<endl;
				exit(1);
			}
			break;
		}else{
			sleep(2);
			iter++;
		}
		if(iter>30){
			break;
		}
	}
}
void peProcess::make_tmpDir(){
	srand(time(0));
	ostringstream tmp_str;
	for(int i=0;i!=6;i++){
		int tmp_rand=random(26)+'A';
		tmp_str<<(char)tmp_rand;
	}
	tmp_dir="TMP"+tmp_str.str();
	string mkdir_str="mkdir -p "+gp.output_dir+"/"+tmp_dir;
	if(system(mkdir_str.c_str())==-1){
		cerr<<"Error:mkdir error,"<<mkdir_str<<endl;
		exit(1);
	}
}
void peProcess::output_fastqs(string type,vector<C_fastq> &fq1,gzFile outfile){
	//m.lock();
	string out_content,streaming_out;
	int fq1_size=fq1.size();
	for(int i=0;i!=fq1_size;i++){
	//for(vector<C_fastq>::iterator i=fq1->begin();i!=fq1->end();i++){
		if(gp.output_file_type=="fasta"){
			fq1[i].seq_id=fq1[i].seq_id.replace(fq1[i].seq_id.find("@"),1,">");
			out_content+=fq1[i].seq_id+"\n"+fq1[i].sequence+"\n";
		}else if(gp.output_file_type=="fastq"){
			if(gp.outputQualityPhred!=gp.qualityPhred){
				for(string::size_type ix=0;ix!=fq1[i].qual_seq.size();ix++){
					int b_q=fq1[i].qual_seq[ix]-gp.qualityPhred;
					fq1[i].qual_seq[ix]=(char)(b_q+gp.outputQualityPhred);
				}
			}
			if(gp.is_streaming){
				string modify_id=fq1[i].seq_id;
				modify_id.erase(0,1);
				streaming_out+=">+\t"+modify_id+"\t"+type+"\t"+fq1[i].sequence+"\t"+fq1[i].qual_seq+"\n";
			}else{
				out_content+=fq1[i].seq_id+"\n"+fq1[i].sequence+"\n+\n"+fq1[i].qual_seq+"\n";
			}
		}else{
			cerr<<"Error:output_file_type value error"<<endl;
			exit(1);
		}
	}
	if(gp.is_streaming){
		cout<<streaming_out;
	}else{
		gzwrite(outfile,out_content.c_str(),out_content.size());
		gzflush(outfile,1);
	}
	//m.unlock();
}
void peProcess::output_fastqs(string type,vector<C_fastq> &fq1,FILE* outfile){
	//m.lock();
	string out_content,streaming_out;
	int fq1_size=fq1.size();
	for(int i=0;i!=fq1_size;i++){
	//for(vector<C_fastq>::iterator i=fq1->begin();i!=fq1->end();i++){
		if(gp.output_file_type=="fasta"){
			fq1[i].seq_id=fq1[i].seq_id.replace(fq1[i].seq_id.find("@"),1,">");
			out_content+=fq1[i].seq_id+"\n"+fq1[i].sequence+"\n";
		}else if(gp.output_file_type=="fastq"){
			if(gp.outputQualityPhred!=gp.qualityPhred){
				for(string::size_type ix=0;ix!=fq1[i].qual_seq.size();ix++){
					int b_q=fq1[i].qual_seq[ix]-gp.qualityPhred;
					fq1[i].qual_seq[ix]=(char)(b_q+gp.outputQualityPhred);
				}
			}
			if(gp.is_streaming){
				string modify_id=fq1[i].seq_id;
				modify_id.erase(0,1);
				streaming_out+=">+\t"+modify_id+"\t"+type+"\t"+fq1[i].sequence+"\t"+fq1[i].qual_seq+"\n";
			}else{
				out_content+=fq1[i].seq_id+"\n"+fq1[i].sequence+"\n+\n"+fq1[i].qual_seq+"\n";
			}
		}else{
			cerr<<"Error:output_file_type value error"<<endl;
			exit(1);
		}
	}
	if(gp.is_streaming){
		cout<<streaming_out;
	}else{
		fputs(out_content.c_str(),outfile);
		//gzflush(outfile,1);
	}
	//m.unlock();
}
void peProcess::peStreaming_stat(C_global_variable& local_gv){
	cout<<"#Total_statistical_information"<<"\n";
	/*int output_reads_num;
	int in_adapter_list_num;
	int include_adapter_seq_num;
	int include_contam_seq_num;
	int n_ratio_num;
	int highA_num,polyX_num;
	int tile_num,fov_num;
	int low_qual_base_ratio_num;
	int mean_quality_num;
	int short_len_num,long_len_num;
	int over_lapped_num;
	int no_3_adapter_num,int_insertNull_num;
	*/
	int total=local_gv.fs.include_adapter_seq_num+local_gv.fs.include_contam_seq_num+local_gv.fs.low_qual_base_ratio_num+local_gv.fs.mean_quality_num+local_gv.fs.n_ratio_num+local_gv.fs.over_lapped_num+local_gv.fs.highA_num+local_gv.fs.polyX_num;
	cout<<total<<" "<<local_gv.fs.include_adapter_seq_num<<" "<<local_gv.fs.include_contam_seq_num<<" "<<local_gv.fs.low_qual_base_ratio_num<<" "<<local_gv.fs.mean_quality_num<<" "<<local_gv.fs.n_ratio_num<<" "<<local_gv.fs.over_lapped_num<<" "<<local_gv.fs.highA_num<<" "<<local_gv.fs.polyX_num<<"\n";
	/*int read_max_length;
	int read_length;
	int reads_number;
	long long base_number;
	long long a_number,c_number,g_number,t_number,n_number;
	//long long a_ratio,c_ratio,g_ratio,t_ratio,n_ratio;
	long long q20_num,q30_num;
	*/
	cout<<"#Fq1_statistical_information"<<"\n";
	cout<<local_gv.raw1_stat.gs.read_length<<" "<<local_gv.clean1_stat.gs.read_length<<" "<<local_gv.raw1_stat.gs.reads_number<<" "<<local_gv.clean1_stat.gs.reads_number<<" "<<local_gv.raw1_stat.gs.base_number<<" "<<local_gv.clean1_stat.gs.base_number<<" "<<local_gv.raw1_stat.gs.a_number<<" "<<local_gv.clean1_stat.gs.a_number<<" "<<local_gv.raw1_stat.gs.c_number<<" "<<local_gv.clean1_stat.gs.c_number<<" "<<local_gv.raw1_stat.gs.g_number<<" "<<local_gv.clean1_stat.gs.g_number<<" "<<local_gv.raw1_stat.gs.t_number<<" "<<local_gv.clean1_stat.gs.t_number<<" "<<local_gv.raw1_stat.gs.n_number<<" "<<local_gv.clean1_stat.gs.n_number<<" "<<local_gv.raw1_stat.gs.q20_num<<" "<<local_gv.clean1_stat.gs.q20_num<<" "<<local_gv.raw1_stat.gs.q30_num<<" "<<local_gv.clean1_stat.gs.q30_num<<"\n";
	cout<<"#Base_distributions_by_read_position"<<"\n";
	//long long position_acgt_content[READ_MAX_LEN][5];
	for(int i=0;i!=local_gv.raw1_stat.gs.read_length;i++){
		for(int j=0;j!=4;j++){
			cout<<local_gv.raw1_stat.bs.position_acgt_content[i][j]<<" ";
		}
		cout<<local_gv.raw1_stat.bs.position_acgt_content[i][4]<<"\n";
	}
	for(int i=0;i!=local_gv.clean1_stat.gs.read_length;i++){
		for(int j=0;j!=4;j++){
			cout<<local_gv.clean1_stat.bs.position_acgt_content[i][j]<<" ";
		}
		cout<<local_gv.clean1_stat.bs.position_acgt_content[i][4]<<"\n";
	}
	cout<<"#Raw_Base_quality_value_distribution_by_read_position"<<"\n";
	//position_qual[READ_MAX_LEN][MAX_QUAL]
	for(int i=0;i!=local_gv.raw1_stat.gs.read_length;i++){
		for(int j=0;j!=40;j++){
			cout<<local_gv.clean1_stat.qs.position_qual[i][j]<<" ";
		}
		cout<<"0\n";
	}
	for(int i=0;i!=local_gv.clean1_stat.gs.read_length;i++){
		for(int j=0;j!=40;j++){
			cout<<local_gv.clean1_stat.qs.position_qual[i][j]<<" ";
		}
		cout<<"0\n";
	}
	cout<<"#Fq2_statistical_information"<<"\n";
	cout<<local_gv.raw2_stat.gs.read_length<<" "<<local_gv.clean2_stat.gs.read_length<<" "<<local_gv.raw2_stat.gs.reads_number<<" "<<local_gv.clean2_stat.gs.reads_number<<" "<<local_gv.raw2_stat.gs.base_number<<" "<<local_gv.clean2_stat.gs.base_number<<" "<<local_gv.raw2_stat.gs.a_number<<" "<<local_gv.clean2_stat.gs.a_number<<" "<<local_gv.raw2_stat.gs.c_number<<" "<<local_gv.clean2_stat.gs.c_number<<" "<<local_gv.raw2_stat.gs.g_number<<" "<<local_gv.clean2_stat.gs.g_number<<" "<<local_gv.raw2_stat.gs.t_number<<" "<<local_gv.clean2_stat.gs.t_number<<" "<<local_gv.raw2_stat.gs.n_number<<" "<<local_gv.clean2_stat.gs.n_number<<" "<<local_gv.raw2_stat.gs.q20_num<<" "<<local_gv.clean2_stat.gs.q20_num<<" "<<local_gv.raw2_stat.gs.q30_num<<" "<<local_gv.clean2_stat.gs.q30_num<<"\n";
	cout<<"#Base_distributions_by_read_position"<<"\n";
	//long long position_acgt_content[READ_MAX_LEN][5];
	for(int i=0;i!=local_gv.raw2_stat.gs.read_length;i++){
		for(int j=0;j!=4;j++){
			cout<<local_gv.raw2_stat.bs.position_acgt_content[i][j]<<" ";
		}
		cout<<local_gv.raw2_stat.bs.position_acgt_content[i][4]<<"\n";
	}
	for(int i=0;i!=local_gv.clean2_stat.gs.read_length;i++){
		for(int j=0;j!=4;j++){
			cout<<local_gv.clean2_stat.bs.position_acgt_content[i][j]<<" ";
		}
		cout<<local_gv.clean2_stat.bs.position_acgt_content[i][4]<<"\n";
	}
	cout<<"#Raw_Base_quality_value_distribution_by_read_position"<<"\n";
	//position_qual[READ_MAX_LEN][MAX_QUAL]
	for(int i=0;i!=local_gv.raw2_stat.gs.read_length;i++){
		for(int j=0;j!=41;j++){
			cout<<local_gv.raw2_stat.qs.position_qual[i][j]<<" ";
		}
		cout<<"0\n";
	}
	for(int i=0;i!=local_gv.clean2_stat.gs.read_length;i++){
		for(int j=0;j!=41;j++){
			cout<<local_gv.clean2_stat.qs.position_qual[i][j]<<" ";
		}
		cout<<"0\n";
	}
}
void peProcess::check_disk_available(){
	if(access(gp.fq1_path.c_str(),0)==-1 || access(gp.fq2_path.c_str(),0)==-1){
		cerr<<"Error:input raw fastq not exists suddenly, please check the disk"<<endl;
		exit(1);
	}
	if(access(gp.output_dir.c_str(),0)==-1){
		cerr<<"Error:output directory cannot open suddenly, please check the disk"<<endl;
		exit(1);
	}
}

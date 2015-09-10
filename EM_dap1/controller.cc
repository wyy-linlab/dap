#include "classdef.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <math.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <omp.h>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include "logistic.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

void controller::load_data(char *filename){

  

  ifstream dfile(filename, ios_base::in | ios_base::binary);
  boost::iostreams::filtering_istream in;

  in.push(boost::iostreams::gzip_decompressor());
  in.push(dfile);
  
 
  string line;
  istringstream ins;

  string curr_loc_id = "";

  vector<SNP> snp_vec;
  string loc_id;
  string snp_id;
  double beta;
  double se_beta;
  double t_val;
  int index_count = 0;
  int loc_count = 0;
  while(getline(in,line)){

    ins.clear();
    ins.str(line);

    if(ins>>snp_id>>loc_id>>beta>>t_val){
      

      if(curr_loc_id != loc_id){
	if(curr_loc_id != ""){
	  
	  if(loc_hash.find(curr_loc_id)== loc_hash.end()){
	    Locus loc(curr_loc_id, snp_vec);
	    int pos = loc_hash.size();
	    loc_hash[curr_loc_id] = pos;
	    locVec.push_back(loc);	  
	    loc_count++;
	  }else{
	    // already defined, concate the existing snpVec
	    int pos = loc_hash[curr_loc_id];
	    for(int i=0;i<snp_vec.size();i++){
	      locVec[pos].snpVec.push_back(snp_vec[i]);
	    }	   
	  }
	  
	  snp_vec.clear();
	
	}
	curr_loc_id = loc_id;
   
      }
      
      se_beta = beta/t_val;
      double log10_BF = compute_log10_BF(beta, se_beta);
      SNP snp(snp_id, log10_BF, index_count);
      index_count++;
      snp_hash[snp_id] = 100;
      snp_vec.push_back(snp);
      
    }
  }
   
  dfile.close();

  if(loc_hash.find(curr_loc_id)== loc_hash.end()){
    Locus loc(curr_loc_id, snp_vec);
    int pos = loc_hash.size();
    loc_hash[curr_loc_id] = pos;
    locVec.push_back(loc);
    loc_count++;
  }else{
    // already defined, concate the existing snpVec                                               
    int pos = loc_hash[curr_loc_id];
    for(int i=0;i<snp_vec.size();i++){
      locVec[pos].snpVec.push_back(snp_vec[i]);
    }
  }
  
  
 
  p = index_count;

  fprintf(stderr, "Read in %d loci, %d locus-SNP pairs ... \n",loc_count, p);
  
  prior_vec = gsl_vector_calloc(p);

}


// this function directly loads pre-computed log10 Bayes factors

void controller::load_data_BF(char *filename){

  

  ifstream dfile(filename, ios_base::in | ios_base::binary);
  boost::iostreams::filtering_istream in;

  in.push(boost::iostreams::gzip_decompressor());
  in.push(dfile);
  
 
  string line;
  istringstream ins;

  string curr_loc_id = "";

  vector<SNP> snp_vec;
  string loc_id;
  string snp_id;
  double log10_BF;
  int index_count = 0;
  int loc_count = 0;
  while(getline(in,line)){

    ins.clear();
    ins.str(line);

    if(ins>>snp_id>>loc_id>>log10_BF){
      

      if(curr_loc_id != loc_id){
	if(curr_loc_id != ""){
	  
	  if(loc_hash.find(curr_loc_id)== loc_hash.end()){
	    Locus loc(curr_loc_id, snp_vec);
	    int pos = loc_hash.size();
	    loc_hash[curr_loc_id] = pos;
	    locVec.push_back(loc);	  
	    loc_count++;
	  }else{
	    // already defined, concate the existing snpVec
	    int pos = loc_hash[curr_loc_id];
	    for(int i=0;i<snp_vec.size();i++){
	      locVec[pos].snpVec.push_back(snp_vec[i]);
	    }	   
	  }
	  
	  snp_vec.clear();
	
	}
	curr_loc_id = loc_id;
   
      }
      
      SNP snp(snp_id, log10_BF, index_count);
      index_count++;
      snp_hash[snp_id] = 100;
      snp_vec.push_back(snp);
      
    }
  }
   
  dfile.close();

  if(loc_hash.find(curr_loc_id)== loc_hash.end()){
    Locus loc(curr_loc_id, snp_vec);
    int pos = loc_hash.size();
    loc_hash[curr_loc_id] = pos;
    locVec.push_back(loc);
    loc_count++;
  }else{
    // already defined, concate the existing snpVec                                               
    int pos = loc_hash[curr_loc_id];
    for(int i=0;i<snp_vec.size();i++){
      locVec[pos].snpVec.push_back(snp_vec[i]);
    }
  }
  
  
 
  p = index_count;

  fprintf(stderr, "Read in %d loci, %d locus-SNP pairs ... \n",loc_count, p);
  
  prior_vec = gsl_vector_calloc(p);

}








void controller::load_map(char* gene_file, char *snp_file){

  if(strlen(gene_file)==0 || strlen(snp_file)==0){
    return;
  }

  map<string, int> gene_map;
  map<string, int> snp_map;

  ifstream gfile(gene_file, ios_base::in | ios_base::binary);
  boost::iostreams::filtering_istream in;

  in.push(boost::iostreams::gzip_decompressor());
  in.push(gfile);
  

  string line;
  istringstream ins;

  
  string gene_id;
  int chr;
  double tss1;
  double tss2;
  while(getline(in,line)){

    ins.clear();
    ins.str(line);

    if(ins>>gene_id>>chr>>tss1>>tss2){
      if(loc_hash.find(gene_id) != loc_hash.end()){
	gene_map[gene_id] = tss1;
      }
    }
  }
  gfile.close();

  
  ifstream sfile(snp_file, ios_base::in | ios_base::binary);
  boost::iostreams::filtering_istream snp_in;
  
  snp_in.push(boost::iostreams::gzip_decompressor());
  snp_in.push(sfile);

  istringstream snp_ins;


  string snp_id;
  double pos;
  
  while(getline(snp_in,line)){

    snp_ins.clear();
    snp_ins.str(line);

    if(snp_ins>>snp_id>>chr>>pos){
      if(snp_hash.find(snp_id)!=snp_hash.end()){
	snp_map[snp_id] = pos;
      }
    }
  }
  sfile.close();

  dist_bin = gsl_vector_int_calloc(p);
  map<int, int> bin_hash;
 
  for (int i=0;i<locVec.size();i++){
    string loc_id = locVec[i].id;
    for(int j=0;j<locVec[i].snpVec.size();j++){
      string snp_id = locVec[i].snpVec[j].id;
      int bin = classify_dist_bin(snp_map[snp_id], gene_map[loc_id],dist_bin_size);
      if(bin_hash.find(bin)==bin_hash.end()){
         bin_hash[bin] = 0;
      }
      bin_hash[bin]++;
      gsl_vector_int_set(dist_bin,locVec[i].snpVec[j].index, bin);
    }
  }
  

  dtss_map[0] =0;
  dtss_rmap[0] = 0;
  int count = 1;
  for (map<int,int>::iterator it=bin_hash.begin(); it!=bin_hash.end(); ++it){
    //printf("bin %d    %d \n", it->first, it->second);
    if(it->first==0)
      continue;
    dtss_map[it->first] = count;
    dtss_rmap[count] = it->first;
    count++;
  }
  
  dist_bin_level = count;
  
  for(int i=0;i<p;i++){
    //printf("%d \n", gsl_vector_int_get(dist_bin,i));
    gsl_vector_int_set(dist_bin, i, dtss_map[gsl_vector_int_get(dist_bin,i)]);
  }
  //exit(1);
  
}





void controller::load_annotation(char* annot_file){


  map<string, vector<double> > annot_map;

  map<int, int> col2cat;
  map<int, int> col2cpos;
  map<int, int> col2dpos;
  int col_count = -1;


  if(strlen(annot_file)>0){
 
    ifstream afile(annot_file, ios_base::in | ios_base::binary);
    boost::iostreams::filtering_istream in;

    in.push(boost::iostreams::gzip_decompressor());
    in.push(afile);
    
    string line;
    istringstream ins;

    // parsing headers

    int count_d=0;
    int count_c=0;
    

       
    
    while(getline(in,line)){
      ins.clear();
      ins.str(line);
      string token;
      
      while(ins>>token){
	
	if(col_count==-1){
	  if(token == "SNP" || token == "snp"){
	    col_count = 0;
	    continue;
	  }else{
	    break;
	  }
	}
	
	string cat = token.substr(token.size()-2, 2);
	// continuous
	if(cat == "_c" || cat =="_C"){
	  col2cat[col_count] = 1;
	  col2cpos[col_count] = kc;
	  string name = token.substr(0,token.size()-2);
	  cvar_name_vec.push_back(name);
	  kc++;
	} // discrete/categorical
	else{
	  col2cat[col_count] = 2;
	  col2dpos[col_count] = kd;
	  string name = token.substr(0,token.size()-2);
	  dvar_name_vec.push_back(name);
	  kd++;
	  
	}
	
	col_count++;
      }
      

      if(col_count!=-1)
	break;
      
    }
    

    // read in data
   
    
    while(getline(in,line)){
      
      ins.clear();
      ins.str(line);
      string snp;
      
    
      ins>>snp;
      if(snp_hash[snp] != 100)
	continue;
      
      double val;
      vector<double> avec;
      while(ins>>val){
	avec.push_back(val);
      }
      annot_map[snp] = avec;
    }
  }
  

  // memory allocation

  if(kc>0){
    Xc = gsl_matrix_calloc(p,kc);
  }
  
  
  int ncol = kd;
  if(dist_bin_level>0)
    ncol++;
  if(ncol>0){
    Xd = gsl_matrix_int_calloc(p,ncol);
    dlevel = gsl_vector_int_calloc(ncol);
  }
  
  
  if(kc+kd>0){
    for (int i=0;i<locVec.size();i++){
      for(int j=0;j<locVec[i].snpVec.size();j++){
	string snp_id = locVec[i].snpVec[j].id;
	int index = locVec[i].snpVec[j].index;
	
	vector<double> avec((kd+kc),0.0);
	if(annot_map.find(snp_id)!=annot_map.end())
	  avec = annot_map[snp_id];
      
	for(int k=0;k<avec.size();k++){
	  if(col2cat[k] == 1){
	    gsl_matrix_set(Xc,index,col2cpos[k],avec[k]);
	  }else{
	    gsl_matrix_int_set(Xd, index, col2dpos[k],int(avec[k]));
	  }
	
	}
      }
    }
   
    for(int i=0;i<kd;i++){
      int nl = count_factor_level(i);
      gsl_vector_int_set(dlevel, i,nl);
    }
  }


  if(dist_bin_level>0){
    gsl_matrix_int_set_col(Xd, ncol-1, dist_bin);
    gsl_vector_int_set(dlevel, ncol-1, dist_bin_level);
    dvar_name_vec.push_back(string("dtss"));
    kd++;
  }
  
  gsl_vector_int_free(dist_bin);
}



int controller::count_factor_level(int col){
 
  map<int, int> rcd;
  for(int i=0;i<p;i++){
    int val = gsl_matrix_int_get(Xd,i,col);
    rcd[val] = 1;
  }
  
  return rcd.size();
}



void controller::init_params(){
  
  prior_vec = gsl_vector_calloc(p);
  pip_vec = gsl_vector_calloc(p);
  for(int i=0;i<p;i++){
    gsl_vector_set(prior_vec, i, 1e-3);
  }
  

  for(int i=0;i<locVec.size();i++){
    locVec[i].pip_vec = pip_vec;
    locVec[i].prior_vec = prior_vec;
  }




  ncoef = 0;
  for(int i=0;i<kd;i++){
    ncoef += gsl_vector_int_get(dlevel,i)-1;
  }

  ncoef += 1;
  ncoef += kc;
  

  beta_vec = gsl_vector_calloc(ncoef);
  
}


void controller::run_EM(){  
  
  // start iteration
  fprintf(stderr,"Starting EM ... \n");
  int count = 1;
  double last_log10_lik = -9999999;
  init_params();

  fprintf(stderr,"  Iter          loglik          Intercept    ");
  
  for(int i=0;i<dvar_name_vec.size();i++){
    string prefix = dvar_name_vec[i];
    int level = gsl_vector_int_get(dlevel,i);
    for(int j=1;j<level;j++){
      ostringstream stream;
      stream <<prefix<<"."<<j;
      string label = stream.str();
      fprintf(stderr, "%s\t", label.c_str());
    }
    
  }
    
  fprintf(stderr,"\n");
  while(1){
    
    
    double curr_log10_lik = 0;


    for(int i=0;i<locVec.size();i++){
   
      locVec[i].EM_update();
      curr_log10_lik += locVec[i].log10_lik;
      
      /*
      for(int j=0;j<locVec[i].snpVec.size();j++){
	int index = locVec[i].snpVec[j].index;
	printf("%s %f  ", locVec[i].snpVec[j].id.c_str(), gsl_vector_get(pip_vec,index));                                                          
	for(int k=0;k<kd;k++){                                                                              
	  printf("%d ", gsl_matrix_int_get(Xd,index,k));                                                        
	  }                                                                                                   
	printf("\n"); 
	}
      */
      
    }
    
    


    /*
    for(int i=0;i<p;i++){
      printf("%f  ", gsl_vector_get(pip_vec,i));
      for(int j=0;j<kd;j++){
	printf("%d ", gsl_matrix_int_get(Xd,i,j));
      }
      printf("\n");
      

    }
    */
    


    if(ncoef==1){
      simple_regression();
    }else if(kc==0 && kd == 1 && !force_logistic){
      single_ct_regression();
    }else{
      logistic_fit(beta_vec, Xd, dlevel, pip_vec, 0,0);
      logistic_pred(beta_vec,Xd, dlevel, prior_vec);
    }

    //printf("%f   %f\n%f   %f\n",ct_00, ct_01, ct_10, ct_11);
    fprintf(stderr,"%4d        %10.3f        ",count++,curr_log10_lik/log10(exp(1)));
    

    for(int i=0;i<ncoef;i++){
      fprintf(stderr, "%9.3f  ",gsl_vector_get(beta_vec,i));
    }
    fprintf(stderr,"\n");
   
    // output 
    
    
    
    if(fabs(curr_log10_lik-last_log10_lik)<EM_thresh){
      final_log10_lik = curr_log10_lik;
      break;
    }

    last_log10_lik = curr_log10_lik;
  }   
  
}



void controller::simple_regression(){
  
  double sum = 0;
  for(int i=0;i<p;i++){
    sum += gsl_vector_get(pip_vec,i);
  }
  double new_prior = sum/p;
  for(int i=0;i<p;i++){
    gsl_vector_set(prior_vec,i,new_prior);
  }

  gsl_vector_set(beta_vec,0, log(new_prior/(1-new_prior)));
}




void controller::single_ct_regression(){
 
  map<int,double> sum_pip;
  map<int,double> sum;
  
  int levels = gsl_vector_int_get(dlevel,0);

  for(int i=0;i<levels;i++){
    sum_pip[i] = sum[i] = 0;
  }

  for(int i=0;i<p;i++){
    int cat = gsl_matrix_int_get(Xd,i,0);
    sum_pip[cat] += gsl_vector_get(pip_vec,i);
    sum[cat] += 1;
  }
  
  
  for(int i=0;i<p;i++){
    int cat = gsl_matrix_int_get(Xd,i,0);
    gsl_vector_set(prior_vec,i,sum_pip[cat]/sum[cat]);
  }
  

  double baseline=0;
  for(int i=0;i<levels;i++){
    double new_prior = sum_pip[i]/sum[i];
    gsl_vector_set(beta_vec,i, log(new_prior/(1-new_prior))-baseline);
    if(i==0){
      baseline = log(new_prior/(1-new_prior));
    }
 
  }
  
  
}




// option 1: find egene


void controller::find_eGene(double fdr_thresh){

  if(!finish_em){
    run_EM();
    finish_em = 1;
  }
  
  for(int i=0;i<locVec.size();i++){
    locVec[i].compute_fdr();
  }
  
  std::sort(locVec.begin(), locVec.end(),rank_by_fdr);
  
  double rej = 1.0;
  double cpr = 0.0;
  int rej_decision = 1;
  for(int i=0;i<locVec.size();i++){
    cpr += locVec[i].fdr;
    if(cpr/rej > fdr_thresh){
      rej_decision = 0;
    }
    printf("%5d  %20s    %9.3e    %d\n",int(rej), locVec[i].id.c_str(), locVec[i].fdr,rej_decision);
    rej++;
  }
  
  

}


// option 2: parameter estimation

void controller::estimate(){
  
  if(!finish_em){
    run_EM();
    finish_em = 1;
  }

  gsl_vector *saved_beta_vec = gsl_vector_calloc(ncoef);
  gsl_vector *saved_prior_vec = gsl_vector_calloc(p);
  gsl_vector_memcpy(saved_beta_vec, beta_vec);  
  gsl_vector_memcpy(saved_prior_vec, prior_vec);
  

  
  int index = 1; //start with 1, 0 always intercept
  for(int i=0;i<dvar_name_vec.size();i++){
    if(dvar_name_vec[i]=="dtss"){
      index += gsl_vector_int_get(dlevel,i);
      break;
    }
    int level = gsl_vector_int_get(dlevel,i);
    string prefix = dvar_name_vec[i];
    for(int j=1;j<level;j++){
      ostringstream stream;
      stream <<prefix<<"."<<j;
      string label = stream.str();
      double est = gsl_vector_get(beta_vec, index);
      gsl_vector_set(beta_vec,index, 0.0);
      logistic_pred(beta_vec, Xd, dlevel, prior_vec);
      double null_log10_lik = 0;
      for(int k=0;k<locVec.size();k++){
	locVec[k].EM_update();
	null_log10_lik += locVec[k].log10_lik;
      }
      double diff = (final_log10_lik - null_log10_lik)/log10(exp(1));
      if(diff<1e-8){
	diff = 1e-8;
      }
      double sd = fabs(est)/sqrt(2*diff);
      printf("%15s  %9.3f     %9.3f  %9.3f\n", label.c_str(), est, est-1.96*sd, est+1.96*sd);
      index++;
      // restore
      gsl_vector_memcpy(beta_vec, saved_beta_vec);  
      gsl_vector_memcpy(prior_vec, saved_prior_vec);
    }
  }
  
  // restore all
  for(int k=0;k<locVec.size();k++){
    locVec[k].EM_update();
  }
  
  gsl_vector_free(saved_beta_vec);
  gsl_vector_free(saved_prior_vec);

}



void controller::dump_prior(char *prior_path){

  if(!finish_em){
    run_EM();
    finish_em = 1;
  }
 
  
  fprintf(stderr,"Output priors ...\n");
  if(mkdir(prior_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)!=0){
    fprintf(stderr, "Error: cannot create directory %s\n", prior_path);
    exit(1);
  }
  
  string dir = string(prior_path);
  for( int i=0; i<locVec.size();i++){
    string file_name = dir + "/" + locVec[i].id+".prior";
    FILE *fd = fopen(file_name.c_str(), "w");
    for(int j=0;j<locVec[i].snpVec.size();j++){
      int index = locVec[i].snpVec[j].index;
      string name = locVec[i].snpVec[j].id;
      fprintf(fd, "%s  %9.4e\n",name.c_str(), gsl_vector_get(prior_vec, index));
    }
    fclose(fd);
  }
}
						
    
  
















void Locus::EM_update(){

  
  // compute log10_lik
  vector<double> BF_vec;
  vector<double> p_vec;
  
  double locus_pi0 = 1;
  
  for(int i=0;i<snpVec.size();i++){
    double prior = gsl_vector_get(prior_vec, snpVec[i].index);
    BF_vec.push_back(snpVec[i].log10_BF);
    p_vec.push_back(prior/(1-prior));
    locus_pi0 *= (1-prior);
  }
  
  for(int i=0;i<snpVec.size();i++){
    p_vec[i] *= locus_pi0;
  }
  
  BF_vec.push_back(0);
  p_vec.push_back(locus_pi0);
  

  log10_lik = log10_weighted_sum(BF_vec, p_vec);
  
  //printf("%s  log10_lik = %f\n",id.c_str(), log10_lik);
  
  for(int i=0;i<snpVec.size();i++){
    
    double val = log10(p_vec[i]) + snpVec[i].log10_BF - log10_lik;
    double pip = pow(10, val);
    gsl_vector_set(pip_vec, snpVec[i].index, pip);
  }
  
 
}


void Locus::compute_fdr(){

  double locus_pi0 = 1;
 
  // compute log10_lik
  vector<double> BF_vec;
  vector<double> p_vec;
  for(int i=0;i<snpVec.size();i++){
    double prior = gsl_vector_get(prior_vec, snpVec[i].index);
    BF_vec.push_back(snpVec[i].log10_BF);
    p_vec.push_back(prior/(1-prior));
    locus_pi0 *= (1-prior);
  }
  
  for(int i=0;i<snpVec.size();i++){
    p_vec[i] *= locus_pi0;
  }


  BF_vec.push_back(0);
  p_vec.push_back(locus_pi0);
  
  log10_lik = log10_weighted_sum(BF_vec, p_vec);
  fdr =  pow(10,log10(locus_pi0)-log10_lik);

}












double compute_log10_BF(double beta, double se_beta){

  if(se_beta == 0)
    return 0;
  
  double phi[4] = {0.1,0.2,0.4,0.8};
  int size = 4;
  double z2 = pow((beta/se_beta), 2.0);
  double v2 = pow(se_beta, 2.0);
  vector<double> rstv;
  vector<double> wtv(size,1.0/double(size));
  for(int i=0;i<size;i++){
    //double w2 = pow(se_beta*phi[i],2.0);
    double w2 = pow(phi[i],2.0);
    double val = 0.5*log(v2/(v2+w2)) + 0.5*z2*(w2/(v2+w2));
    rstv.push_back(val/log(10));
  }

  return log10_weighted_sum(rstv,wtv);
}




double log10_weighted_sum(vector<double> &vec, vector<double> &wts){


  double max = vec[0];
  for(size_t i=0;i<vec.size();i++){
    if(vec[i]>max)
      max = vec[i];
  }
  double sum = 0;
  for(size_t i=0;i<vec.size();i++){
    sum += wts[i]*pow(10, (vec[i]-max));
  }

  return (max+log10(sum));
}

bool rank_by_fdr (const Locus & loc1 , const Locus & loc2){
  return (loc1.fdr < loc2.fdr);
}
 





// default binning scheme
int classify_dist_bin(int pos, int tss, double bin_size){

  if(bin_size>0){
    return int((pos-tss)/1000.0);
  }
  
  // else default scheme
  double dist = (pos-tss)/1000.0;
  int sign = 1;
  if(dist < 0){
    sign = -1;
  }
  dist = fabs(dist);
  int bin;
  
  if(dist<0.5){
    bin = 0;
  }else if(dist<1){
    bin = 1;
  }else if(dist<2.5){
    bin = 2;
  }else if(dist<5){
    bin = 3;
  }else if(dist<10){
    bin = 4;
  }else if(dist <25){
    bin = 5;
  }else if(dist <50){
    bin = 6;
  }else if(dist <100){
    bin = 7;
  }else if(dist <250){
    bin =8;
  }else if(dist < 500){
    bin = 9;
  }else{
    bin = 10;
  }

  return bin*sign;
}



  

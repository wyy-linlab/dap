using namespace std;

#include "parser.h"
#include "stdlib.h"
#include <string>
#include <sstream>
#include <fstream>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_blas.h>

void parser::process_data(char *filename){
  
  
  // reading data file
  ifstream infile(filename);
  string line;

  while(getline(infile,line))
    process_line(line);
   
  //output();
  for(int i=0;i<pheno_vec.size();i++){
       if(covar_vec[i].size()>0)	  
          regress_cov(pheno_vec[i], covar_vec[i], geno_vec[i]);
  } 

  for(int i=0;i<covar_vec.size();i++){
      if(covar_vec[i].size()>0)
         covar_vec[i].clear();
  }  
}

void parser::process_line(string line){
    
  // parsing

  istringstream ins(line);
  
  char *content = new char[strlen(line.c_str())+1];
  strcpy(content, line.c_str());


  string header;
  ins>>header;

  string name;
  string grp;
  ins>> name;
  ins >> grp;
  

  vector<double> vecv;
  
  int missing_flag = 0;  // missing data flag
  string val;
  while(ins>>val){
    
    if(val == "NA" || val == "na"){
      missing_flag = 1;
      vecv.push_back(-1);
    }else
      vecv.push_back(atof(val.c_str()));
  }
  
  // impute missing data with mean

  if(missing_flag == 1){
    
    double nm_sum = 0;
    int nm_count = 0;
    vector<int> iv;
    for (int i=0;i<vecv.size();i++){
      if(vecv[i]==-1){
	iv.push_back(i);
      }else{
	nm_sum += vecv[i];
	nm_count++;
      }
    }

    double imp_mean = 0;
    if(nm_count!=0){
      imp_mean = nm_sum/nm_count;
    }

    for(int i=0;i<iv.size();i++){
      vecv[iv[i]] = imp_mean;
    }
  }
      


  if( header == "pheno" || header == "response"){
    pheno_name = name;
    pheno_vec.push_back(vecv);  
    pheno_map[pheno_vec.size()-1] = grp;
    
    pheno_index[string(grp)] = pheno_vec.size()-1;
    
    vector<vector<double> > gvec;
    geno_vec.push_back(gvec);
    vector<vector<double> > cvec;
    covar_vec.push_back(cvec);
  }else if( header == "geno" || header == "covariate" ){
    int index = pheno_index[grp];
    geno_vec[index].push_back(vecv);
    if(geno_rmap.find(name)==geno_rmap.end()){
      geno_map[geno_vec[index].size()-1] = name;
      geno_rmap[name] = geno_vec[index].size()-1;
    }
  }else if(header == "controlled"){
    int index = pheno_index[grp];
    covar_vec[index].push_back(vecv);
    
  }
  else{
    fprintf(stderr, "unknown header %s ... skipping \n", header.c_str());
  }




}

  
void parser::output(){
  
  for(int i=0;i<pheno_vec.size();i++)
    printf("%10s  ", pheno_map[i].c_str());
    
  
  for(int i=0;i<geno_vec[0].size();i++)
    printf("%10s  ", geno_map[i].c_str());

  printf("\n");

  int n = pheno_vec[0].size();
  
  for(int i=0;i<n;i++){
    for(int j=0;j<pheno_vec.size();j++)
      printf("%9.3f  ",pheno_vec[j][i]);
    
    for(int j=0;j<geno_vec.size();j++){
      printf("  ");
      for(int k=0;k<geno_vec[j].size();k++){
	printf("%.1f ", geno_vec[j][k][i]);
      }
      printf("\n");
    }
  }
}








void parser::regress_cov(vector<double> &phenov, vector<vector<double> > &cov, vector<vector<double> > &genov ){

         int n = phenov.size();
         int p = cov.size();	 
         
         gsl_matrix *X = gsl_matrix_calloc(n, p+1);
	 for(int i=0;i<n;i++){
		gsl_matrix_set(X,i,0,1.0);
	 }	
         for(int i=0;i<p;i++){
		 vector<double> vec = cov[i];
		 for(int j=0;j<n;j++){
		    gsl_matrix_set(X,j,i+1,vec[j]);
	         }
          }		 


	  gsl_matrix *XtX = gsl_matrix_calloc(p+1,p+1);
	  gsl_blas_dgemm(CblasTrans,CblasNoTrans,1,X,X,0,XtX);


	  // compute inverse of XtX (generalized inverse version)
	  gsl_matrix *V = gsl_matrix_calloc(p+1,p+1);
	  gsl_vector *S = gsl_vector_calloc(p+1);
	  gsl_vector *work = gsl_vector_calloc(p+1);
	  gsl_linalg_SV_decomp (XtX, V, S,work);
	  
	  gsl_matrix *t1 = gsl_matrix_calloc(p+1,p+1);
	  for(int i=0;i<p+1;i++){
	       double v = gsl_vector_get(S,i);
	       if(v>1e-8){
	             gsl_matrix_set(t1,i,i,1/v);
	       }
	  }
	  gsl_matrix *t2 = gsl_matrix_calloc(p+1,p+1);
	  gsl_blas_dgemm(CblasNoTrans,CblasNoTrans,1,V,t1,0,t2);
	  
	  gsl_matrix *XtX_inv = gsl_matrix_calloc(p+1,p+1);
	  gsl_blas_dgemm(CblasNoTrans,CblasTrans,1,t2,V,0,XtX_inv);
	

          // (X'X)^{-1)X'
          gsl_matrix *t3 = gsl_matrix_calloc(p+1,n);
          gsl_blas_dgemm(CblasNoTrans,CblasTrans,1,XtX_inv,X,0,t3);

	  // X(X'X)^{-1}X'
          gsl_matrix *H = gsl_matrix_calloc(n,n);
          gsl_blas_dgemm(CblasNoTrans,CblasNoTrans,1,X,t3,0,H);

          gsl_matrix_free(t1);
	  gsl_matrix_free(t2);
	  gsl_matrix_free(t3);

          gsl_matrix_free(XtX);
          gsl_matrix_free(XtX_inv);
          gsl_matrix_free(X);
		 
	  gsl_matrix_free(V);
	  gsl_vector_free(S);
	  gsl_vector_free(work);

          
          // do this for all predictors and y        
          gsl_matrix *y = gsl_matrix_calloc(n,1);
	  for(int i=0;i<n;i++){
	       gsl_matrix_set(y,i,0, phenov[i]);
	  }

	  	
          gsl_matrix *fy = gsl_matrix_calloc(n,1);
          gsl_blas_dgemm(CblasNoTrans,CblasNoTrans,1,H,y,0,fy);

          gsl_matrix *res= gsl_matrix_calloc(n,1);
	  gsl_matrix_memcpy(res,y);
	  gsl_matrix_sub(res,fy);
          
	  for(int i=0;i<n;i++){
		phenov[i] = gsl_matrix_get(res,i,0);
          }

          gsl_matrix_free(y);
	  gsl_matrix_free(fy);
	  gsl_matrix_free(res);



         for(int i=0; i<genov.size();i++){
	       gsl_matrix *g = gsl_matrix_calloc(n,1);
	       for(int j=0;j<n;j++){
	            gsl_matrix_set(g,j,0, genov[i][j]);
	       }
	  
	      gsl_matrix *fg = gsl_matrix_calloc(n,1);
              gsl_blas_dgemm(CblasNoTrans,CblasNoTrans,1,H,g,0,fg);

              gsl_matrix *r= gsl_matrix_calloc(n,1);
	      gsl_matrix_memcpy(r,g);
	      gsl_matrix_sub(r,fg);
          
	      for(int j=0;j<n;j++){
	    	   genov[i][j] = gsl_matrix_get(r,j,0);
              } 

             gsl_matrix_free(g);
	     gsl_matrix_free(fg);
	     gsl_matrix_free(r);

       }

       gsl_matrix_free(H);
        

}



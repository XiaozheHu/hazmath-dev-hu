/*! \file examples/approximation/aaa.c
 *
 *  Created by James Adler, Xiaozhe Hu, and Ludmil Zikatanov 2019/01/09.
 *  Copyright 2015_HAZMATH__. All rights reserved.
 *
 * \brief Computes approximations with rational functions using AAA
 *        algorithm
 *
 * \note This example shows how to use AAA method to compute rational
 *       approximation of a function f_to_approx.
 *
 *\note The function would return long double and its arguments are
 *      long double
 */

#include "hazmath.h"
/*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
#include "aaa_example.h"
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
INT main(int argc,char *argv[])
{
  REAL xmin_in=0e0,xmax_in=1e0;
  ///////////////////////////////////////////////////////////////////////////
  //mbig is the total number of points; mmax_in is the max number of
  //nodes taking part in the interpolation, mmax is much smaller than
  //mbig;
  //
  // m is the number of nodes in the final interpolation after
  // tolerance tolaaa is achieved or mmax is reached.
  INT mbig=(1<<14)+1;
  INT mmax_in=(INT )(mbig/2),m=-22;
  REAL16 tolaaa=powl(2e0,-52e0);
  // parameters for the function we are approximating.
  // For example: s[2]*(x^s[0])+s[3]*(x**s[1])
  REAL16 s[4]={-0.5e0,0.5e0,1e0,0e0}; // s1, s2, alpha,beta
  //
  INT mem,mem16,i,j,k,m1=m+1,m01=m-1,mm=m*m,mm1=m1*m1;
  // s[0]=s,s[1]=t,s[2]=alpha,s[3]=beta;
  fprintf(stderr,"\nUSAGE: ./aaa.ex<<EOF_FRAC\n s,t,alpha,beta,xmin,xmax\nEOF_FRAC\n");
  fprintf(stderr,"\nEXAMPLE:\n./aaa.ex <<EOF_FRAC >some_file.m\n %.2Lf %.2Lf %.2Lf %.2Lf %.2f %.2f\nEOF_FRAC\n", \
	  s[0],s[1],s[2],s[3],xmin_in,xmax_in);
  k=fscanf(stdin,"%Lg %Lg %Lg %Lg %lg %lg",&s[0],&s[1],&s[2],&s[3],&xmin_in,&xmax_in);
  /////////////////////////////////////////////
  //  s[0]=0.5;  s[1]=-0.5;  s[2]=1e0;  s[3]=2e0;
  //for debug: s[0]=0.5;  s[1]=-0.5;  s[2]=1e0;  s[3]=2e0; xmin_in=0e0;xmax_in=1e0;
  // s[0]=-0.50; s[1]=0.50;   s[2]=6.1031431e-03;   s[3]=1e0;   xmin_in=0e0;   xmax_in=1e0;
  /////////////////////////////////////////////////////////////////////////////////
  REAL **cpzwf=malloc(5*sizeof(REAL *));
  // after a call to get_cpzwf, cpzwf[k] are pointers to the following
  // arrays (each of m+1 elements but at most m are used:
  //
  // cpzwf[0]-> residues (res[]); (also as last entry contains the free
  //                               term c[m])
  // cpzwf[1]-> poles (pol[]);
  // cpzwf[2]-> nodes (z[]);
  // cpzwf[3]-> weights (w[]);
  // cpzwf[4]-> function values (f[])
  //
  // the rational approximation is:
  // r(z)=res[m-1] + \sum_{i=0}^{m-2} res[i]/(z-pol[i]);
  //
  INT print_level=0;
  REAL rmax=get_cpzwf(f_to_approx_l,				\
		      (void *)s,				\
		      cpzwf,					\
		      &mbig,&mmax_in,				\
		      &m,xmin_in,xmax_in,tolaaa,print_level);
  //rmax is the max error on the rest of z
  fprintf(stdout,"\nfunction [res,pol,z,w,f,er]=frac()\n");
  fprintf(stdout,"\n%% AUTO GENERATED\n");
  fprintf(stdout,"\n%%%%EXAMPLE(fractional):\n");
  fprintf(stdout,"\n%%%%[res,pol,z,w,f,er]=frac();\n");
  //  fprintf(stdout,"\n%%f_in = @(x) ( %.2Lf * x.^(%.1Lf) + %.2Lf * x.^(%.1Lf) );\n",s[2],s[0],s[3],s[1]);
  fprintf(stdout,"\n%%f_in = %.2Lf * x.^(%.1Lf) + %.2Lf * x.^(%.1Lf) ;\n",s[2],s[0],s[3],s[1]);
  fprintf(stdout,"\n%%===============================================%%\n");
  fprintf(stdout,"\nbounds=[%.16e,%.16e];\n",xmin_in,xmax_in);
  fprintf(stdout,"\n\nm=%d; max_err=%.12e;\n",m,rmax);
  fprintf(stdout,"\npol=zeros(%d,1);res=zeros(%d,1);\nz=zeros(%d,1);w=zeros(%d,1);f=zeros(%d,1);\n", \
	  m-1,m,m,m,m);
  fprintf(stdout,"\n%%===============================================%%\n");

  for(i=0;i<m;i++)fprintf(stdout,"\nres(%d)=%.16e;",i+1,*(cpzwf[0]+i));
  fprintf(stdout,"\n");
  for(i=0;i<m-1;i++) fprintf(stdout,"\npol(%d)=%.16e;",i+1,*(cpzwf[1]+i));
  fprintf(stdout,"\n");
  for(i=0;i<m;i++) fprintf(stdout,"\nz(%d)=%.16e;",i+1,*(cpzwf[2]+i));
  fprintf(stdout,"\n");
  for(i=0;i<m;i++)fprintf(stdout,"\nw(%d)=%.16e;",i+1,*(cpzwf[3]+i));
  fprintf(stdout,"\n");
  for(i=0;i<m;i++)fprintf(stdout,"\nf(%d)=%.16e;",i+1,*(cpzwf[4]+i));
  fprintf(stdout,"\n");
  fprintf(stdout,"\nfz = %.2Lf * z.^(%.1Lf) + %.2Lf * z.^(%.1Lf);\n",s[2],s[0],s[3],s[1]);
  fprintf(stdout,"\ner=norm(f-fz);");
  fprintf(stdout,"\nreturn;\nend\n");
  fprintf(stdout,"\n%%===============================================%%\n\n");
  free(cpzwf[0]);// that is enough. the rest 1-4 are just shifts of cpzwf[0] with m+1;
  free(cpzwf);
  return 0;
}

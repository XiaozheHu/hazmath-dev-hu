/*
 *  interp.c
 *  
 *  Created by James Adler and Xiaozhe Hu on 2/1/15.
 *  Copyright 2015_JXLcode__. All rights reserved.
 *
 */

#include "hazmat.h"

/****************************************************************************************************************************/
// INTERPOLATION AND EVALUATION ROUTINES for Finite Element Basis Functions
/****************************************************************************************************************************/

/****************************************************************************************************************************/
void FE_Interpolation(REAL* val,REAL *u,REAL x,REAL y,REAL z,INT *dof_on_elm,INT *v_on_elm,fespace *FE,trimesh *mesh,INT ndof,INT nun)
{

/* Interpolate a finite-element approximation to any other point in the given element using the given type of elements 
   *    INPUT:
   *		       u       Approximation to interpolate
   *               x,y,z       Coordinates where to compute value
   *          dof_on_elm       DOF belonging to particular element
   *          v_on_elm         Vertices belonging to particular element
   *                  FE       FE Space struct
   *                mesh       Mesh struct
   *		    ndof       Total DOF for each unknown
   *		     nun       Number of unknowns in u (u1,u2,etc?) 1 is a scalar...
   *    OUTPUT:
   *          val	       Value of approximation at given values
   *
   */
	
	
  INT i,nd,j;

  // Get FE and Mesh data
  INT dof_per_elm = FE->dof_per_elm;
  INT FEtype = FE->FEtype;
  INT dim = mesh->dim;

  // Basis Functions and its derivatives if necessary
  REAL* phi=NULL;
  REAL* dphix=NULL;
  REAL* dphiy=NULL;
  REAL* dphiz=NULL;

  if(FEtype>0) { // Lagrange Elements
    phi = (REAL *) calloc(dof_per_elm,sizeof(REAL));
    dphix = (REAL *) calloc(dof_per_elm,sizeof(REAL));
    dphiy = (REAL *) calloc(dof_per_elm,sizeof(REAL));
    if(dim==3) dphiz = (REAL *) calloc(dof_per_elm,sizeof(REAL));
    PX_H1_basis(phi,dphix,dphiy,dphiz,x,y,z,dof_on_elm,FEtype,mesh);
    REAL coef;

    for (i=0; i<nun; i++) {
      coef = 0.0;
      
      for (j=0; j<dof_per_elm; j++) {
	nd = i*ndof + dof_on_elm[j] - 1;
	coef = coef + u[nd]*phi[j];
      }
      val[i] = coef;
    }
  } else if (FEtype==-1) { // Nedelec
    REAL coef1,coef2,coef3;
    INT edge;
    phi = (REAL *) calloc(dof_per_elm*dim,sizeof(REAL));
    if(dim==2) {
      dphix = (REAL *) calloc(dof_per_elm,sizeof(REAL)); // Curl of basis function
    } else if (dim==3) {
      dphix = (REAL *) calloc(dof_per_elm*dim,sizeof(REAL)); // Curl of basis function
    } else {
      baddimension();
    }
    
    ned_basis(phi,dphix,x,y,z,v_on_elm,dof_on_elm,mesh);
    	
    coef1 = 0.0;
    coef2 = 0.0;
    coef3 = 0.0;
		
    if (dim==2) {
      for (j=0; j<dof_per_elm; j++) {
	edge = dof_on_elm[j]-1;
	coef1 = coef1 + u[edge]*phi[j*dim+0];
	coef2 = coef2 + u[edge]*phi[j*dim+1];
      }
      val[0] = coef1;
      val[1] = coef2;
    } else if (dim==3) {
      for (j=0; j<dof_per_elm; j++) {
	edge = dof_on_elm[j]-1;
	coef1 = coef1 + u[edge]*phi[j*dim+0];
	coef2 = coef2 + u[edge]*phi[j*dim+1];
	coef3 = coef3 + u[edge]*phi[j*dim+2];
      }
      val[0] = coef1;
      val[1] = coef2;
      val[2] = coef3;
    }
  } else if (FEtype==-2) { // Raviart-Thomas
    REAL coef1,coef2,coef3;
    INT face;
    phi = (REAL *) calloc(dof_per_elm*dim,sizeof(REAL));
    dphix = (REAL *) calloc(dof_per_elm,sizeof(REAL)); // Divergence of element
    
    rt_basis(phi,dphix,x,y,z,v_on_elm,dof_on_elm,mesh);
    	
    coef1 = 0.0;
    coef2 = 0.0;
    coef3 = 0.0;
		
    if (dim==2) {
      for (j=0; j<dof_per_elm; j++) {
	face = dof_on_elm[j]-1;
	coef1 = coef1 + u[face]*phi[j*dim+0];
	coef2 = coef2 + u[face]*phi[j*dim+1];
      }
      val[0] = coef1;
      val[1] = coef2;
    } else if (dim==3) {
      for (j=0; j<dof_per_elm; j++) {
	face = dof_on_elm[j]-1;
	coef1 = coef1 + u[face]*phi[j*dim+0];
	coef2 = coef2 + u[face]*phi[j*dim+1];
	coef3 = coef3 + u[face]*phi[j*dim+2];
      }
      val[0] = coef1;
      val[1] = coef2;
      val[2] = coef3;
    }
  }
    
    if (phi) free(phi);
    if(dphix) free(dphix);
    if(dphiy) free(dphiy);
    if(dphiz) free(dphiz);
    return;
}
/****************************************************************************************************************************/

/****************************************************************************************************************************/
void FE_DerivativeInterpolation(REAL* val,REAL *u,REAL x,REAL y,REAL z,INT *dof_on_elm,INT *v_on_elm,fespace *FE,trimesh *mesh,INT ndof,INT nun)
{

/* Interpolate the "derivative" of a finite-element approximation to any other point in the given element using the given type of elements 
 * Note that for Lagrange Elements this means the Gradient, grad u, for Nedelec it means the Curl, curl u, and for RT it is the Divergence, div u.
   *    INPUT:
   *		       u       Approximation to interpolate
   *               x,y,z       Coordinates where to compute value
   *          dof_on_elm       DOF belonging to particular element
   *          v_on_elm         Vertices belonging to particular element
   *                  FE       FE Space struct
   *                mesh       Mesh struct
   *		    ndof       Total DOF for each unknown
   *		     nun       Number of unknowns in u (u1,u2,etc?) 1 is a scalar...
   *    OUTPUT:
   *          val	       Value of derivative of approximation at given values
   *
   */
	
	
  INT i,nd,j;

  // Get FE and Mesh data
  INT dof_per_elm = FE->dof_per_elm;
  INT FEtype = FE->FEtype;
  INT dim = mesh->dim;

  // Basis Functions and its derivatives if necessary
  REAL* phi=NULL;
  REAL* dphix=NULL;
  REAL* dphiy=NULL;
  REAL* dphiz=NULL;
  REAL* coef=NULL;

  if(FEtype>0) { // Lagrange Elements
    phi = (REAL *) calloc(dof_per_elm,sizeof(REAL));
    dphix = (REAL *) calloc(dof_per_elm,sizeof(REAL));
    dphiy = (REAL *) calloc(dof_per_elm,sizeof(REAL));
    if(dim==3) dphiz = (REAL *) calloc(dof_per_elm,sizeof(REAL));
    PX_H1_basis(phi,dphix,dphiy,dphiz,x,y,z,dof_on_elm,FEtype,mesh);
    coef = (REAL *) calloc(dim,sizeof(REAL));

    for (i=0; i<nun; i++) {
      for(j=0;j<dim;j++) coef[0] = 0.0;
      
      for (j=0; j<dof_per_elm; j++) {
	nd = i*ndof + dof_on_elm[j] - 1;
	coef[0] = coef[0] + u[nd]*dphix[j];
	coef[1] = coef[1] + u[nd]*dphiy[j];
	if(dim==3) coef[2] = coef[2] + u[nd]*dphiz[j];
      }
      val[i] = coef[0];
      val[nun+i] = coef[1];
      if(dim==3) val[2*nun+i] = coef[2];
    }
  } else if (FEtype==-1) { // Nedelec
    INT edge;
    phi = (REAL *) calloc(dof_per_elm*dim,sizeof(REAL));
    if(dim==2) {
      dphix = (REAL *) calloc(dof_per_elm,sizeof(REAL)); // Curl of basis function
    } else if (dim==3) {
      dphix = (REAL *) calloc(dof_per_elm*dim,sizeof(REAL)); // Curl of basis function
    } else {
      baddimension();
    }
    ned_basis(phi,dphix,x,y,z,v_on_elm,dof_on_elm,mesh);
    	
    coef = (REAL *) calloc(dim,sizeof(REAL));
    for(j=0;j<dim;j++) coef[0] = 0.0;
		
    if (dim==2) {
      for (j=0; j<dof_per_elm; j++) {
	edge = dof_on_elm[j]-1;
	coef[0] = coef[0] + u[edge]*dphix[j];
      }
      val[0] = coef[0];
    } else if (dim==3) {
      for (j=0; j<dof_per_elm; j++) {
	edge = dof_on_elm[j]-1;
	coef[0] = coef[0] + u[edge]*dphix[j*dim+0];
	coef[1] = coef[1] + u[edge]*dphix[j*dim+1];
	coef[2] = coef[2] + u[edge]*dphix[j*dim+2];
      }
      val[0] = coef[0];
      val[1] = coef[1];
      val[2] = coef[2];
    }
  } else if (FEtype==-2) { // Raviart-Thomas
    INT face;
    phi = (REAL *) calloc(dof_per_elm*dim,sizeof(REAL));
    dphix = (REAL *) calloc(dof_per_elm,sizeof(REAL)); // Divergence of element
    
    rt_basis(phi,dphix,x,y,z,v_on_elm,dof_on_elm,mesh);
    	
    coef = (REAL *) calloc(1,sizeof(REAL));
    coef[0] = 0.0;
		
    for (j=0; j<dof_per_elm; j++) {
      face = dof_on_elm[j]-1;
      coef[0] = coef[0] + u[face]*dphix[j];
    }
    val[0] = coef[0];
  }
    
    if (phi) free(phi);
    if(dphix) free(dphix);
    if(dphiy) free(dphiy);
    if(dphiz) free(dphiz);
    if(coef) free(coef);
    return;
}
/****************************************************************************************************************************/

/****************************************************************************************************************************/
void FE_Evaluate(REAL* val,void (*expr)(REAL *,REAL *,REAL),fespace *FE,trimesh *mesh,REAL time)
{

/* Evaluate a given analytic function on the finite-element space given
   *    INPUT:
   *		    expr       Function call to analytic expression expr(FE approx, x values, time)
   *                  FE       FE Space struct
   *                mesh       Mesh struct
   *                time       Time to evaluate function if time-dependent
   *    OUTPUT:
   *          val	       FE approximation of function on fespace
   *
   */

  int i,j;
  REAL* x = (REAL *) calloc(mesh->dim,sizeof(REAL));
  REAL* valx = NULL;
  INT dim = mesh->dim;
  INT FEtype = FE->FEtype;
  
  if(FEtype>0) { // Lagrange Elements u[dof] = u[x_i}
    for(i=0;i<FE->ndof;i++) {
      valx = (REAL *) calloc(1,sizeof(REAL));
      x[0] = FE->cdof->x[i];
      x[1] = FE->cdof->y[i];
      if(dim==3) x[2] = FE->cdof->z[i];
      (*expr)(valx,x,time);
      val[i] = valx[0];
    }
  } else if (FEtype==-1) { // Nedelec u[dof] = (1/elen) \int_edge u*t_edge
    for(i=0;i<FE->ndof;i++) {
      valx = (REAL *) calloc(dim,sizeof(REAL));
      x[0] = mesh->ed_mid[i*dim];
      x[1] = mesh->ed_mid[i*dim+1];
      if(dim==3) x[2] = mesh->ed_mid[i*dim+2];
      (*expr)(valx,x,time);
      val[i] = 0.0;
      for(j=0;j<dim;j++) val[i]+=mesh->ed_tau[i*dim+j]*valx[j];
    }
  } else if (FEtype==-2) { // Raviart-Thomas u[dof] = 1/farea \int_face u*n_face
    for(i=0;i<FE->ndof;i++) {
      valx = (REAL *) calloc(dim,sizeof(REAL));
      x[0] = mesh->f_mid[i*dim];
      x[1] = mesh->f_mid[i*dim+1];
      if(dim==3) x[2] = mesh->f_mid[i*dim+2];
      (*expr)(valx,x,time);
      val[i] = 0.0;
      for(j=0;j<dim;j++) val[i]+=mesh->f_norm[i*dim+j]*valx[j];
    }
  }
  
  if (x) free(x);
  if(valx) free(valx);
  return;
}
/****************************************************************************************************************************/

/****************************************************************************************************************************/
REAL FE_Evaluate_DOF(void (*expr)(REAL *,REAL *,REAL),fespace *FE,trimesh *mesh,REAL time,INT DOF)
{

/* Evaluate a given analytic function on the specific degree of freedom of the finite-element space given
   *    INPUT:
   *		    expr       Function call to analytic expression expr(FE approx, x values, time)
   *                  FE       FE Space struct
   *                mesh       Mesh struct
   *                time       Time to evaluate function if time-dependent
   *                 DOF       DOF index to evaluate (start at 0)
   *    OUTPUT:
   *          val	       FE approximation of function on fespace at DOF
   *
   */

  INT j;
  REAL* x = (REAL *) calloc(mesh->dim,sizeof(REAL));
  REAL* valx = NULL;
  INT dim = mesh->dim;
  INT FEtype = FE->FEtype;
  REAL val=-666e+00;
  
  if(FEtype>0) { // Lagrange Elements u[dof] = u[x_i}
    valx = (REAL *) calloc(1,sizeof(REAL));
    x[0] = FE->cdof->x[DOF];
    x[1] = FE->cdof->y[DOF];
    if(dim==3) x[2] = FE->cdof->z[DOF];
    (*expr)(valx,x,time);
    val = valx[0];
  } else if (FEtype==-1) { // Nedelec u[dof] = (1/elen) \int_edge u*t_edge
    valx = (REAL *) calloc(dim,sizeof(REAL));
    x[0] = mesh->ed_mid[DOF*dim];
    x[1] = mesh->ed_mid[DOF*dim+1];
    if(dim==3) x[2] = mesh->ed_mid[DOF*dim+2];
    (*expr)(valx,x,time);
    val = 0.0;
    for(j=0;j<dim;j++) val+=mesh->ed_tau[DOF*dim+j]*valx[j];
  } else if (FEtype==-2) { // Raviart-Thomas u[dof] = 1/farea \int_face u*n_face
    valx = (REAL *) calloc(dim,sizeof(REAL));
    x[0] = mesh->f_mid[DOF*dim];
    x[1] = mesh->f_mid[DOF*dim+1];
    if(dim==3) x[2] = mesh->f_mid[DOF*dim+2];
    (*expr)(valx,x,time);
    val = 0.0;
    for(j=0;j<dim;j++) val+=mesh->f_norm[DOF*dim+j]*valx[j];
  }
  
  if (x) free(x);
  if(valx) free(valx);

  return val;
}
/****************************************************************************************************************************/

/****************************************************************************************************************************/
void blockFE_Evaluate(REAL* val,void (*expr)(REAL *,REAL *,REAL),block_fespace *FE,trimesh *mesh,REAL time)
{

/* Evaluate a given analytic function on the finite-element space given
   *    INPUT:
   *		    expr       Function call to analytic expression expr(FE approx, x values, time) assumes multiple variables
   *                  FE       block FE Space struct for multiple variables
   *                mesh       Mesh struct
   *                time       Time to evaluate function if time-dependent
   *    OUTPUT:
   *          val	       FE approximation of function on fespace
   *
   */

  int i,j,k;
  REAL* x = (REAL *) calloc(mesh->dim,sizeof(REAL));
  REAL* valx = NULL;
  INT dim = mesh->dim;
  INT entry = 0;

  for(k=0;k<FE->numspaces;i++) {
    if(FE->var_spaces[k]->FEtype>0) { // Lagrange Elements u[dof] = u[x_i}
      for(i=0;i<FE->var_spaces[k]->ndof;i++) {
	valx = (REAL *) calloc(1,sizeof(REAL));
	x[0] = FE->var_spaces[k]->cdof->x[i];
	x[1] = FE->var_spaces[k]->cdof->y[i];
	if(dim==3) x[2] = FE->var_spaces[k]->cdof->z[i];
	(*expr)(valx,x,time);
	val[entry + i] = valx[0];
      }
    } else if (FEtype==-1) { // Nedelec u[dof] = (1/elen) \int_edge u*t_edge
      for(i=0;i<FE->ndof;i++) {
	valx = (REAL *) calloc(dim,sizeof(REAL));
	x[0] = mesh->ed_mid[i*dim];
	x[1] = mesh->ed_mid[i*dim+1];
	if(dim==3) x[2] = mesh->ed_mid[i*dim+2];
	(*expr)(valx,x,time);
	val[i] = 0.0;
	for(j=0;j<dim;j++) val[i]+=mesh->ed_tau[i*dim+j]*valx[j];
      }
    } else if (FEtype==-2) { // Raviart-Thomas u[dof] = 1/farea \int_face u*n_face
      for(i=0;i<FE->ndof;i++) {
	valx = (REAL *) calloc(dim,sizeof(REAL));
	x[0] = mesh->f_mid[i*dim];
	x[1] = mesh->f_mid[i*dim+1];
	if(dim==3) x[2] = mesh->f_mid[i*dim+2];
	(*expr)(valx,x,time);
	val[i] = 0.0;
	for(j=0;j<dim;j++) val[i]+=mesh->f_norm[i*dim+j]*valx[j];
      }
    }
    entry += FE->var_spaces[k]->ndof
  }
  
  if (x) free(x);
  if(valx) free(valx);
  return;
}
/****************************************************************************************************************************/

/***********************************************************************************************/
void get_grad_H1toNed(dCSRmat* Grad,trimesh* mesh) 
{
  // TODO: This only makes sense for P1 elements

  /* Computes Gradient operator.  Applying the resulting matrix computes the gradient of an H1 approximation.  
   * Takes Nodal (H1) DOF vector to Edge (Nedelec) DOF vector
   * Ordering determined by edge to node map: bigger node is +1 smaller is -1
   *    
   *    INPUT:
   *                mesh       Mesh struct
   *    OUTPUT:
   *                Grad       Matrix that takes the gradient of an H1 approximation
   *
   */
  
  INT i,j,k,rowa;
  INT nedge = mesh->nedge;
  REAL oneoverlen;
  dCSRmat Gtmp;

  Gtmp.row = mesh->ed_v->row;
  Gtmp.col = mesh->ed_v->col;
  Gtmp.nnz = mesh->ed_v->nnz;
  Gtmp.IA = (INT *) calloc(Gtmp.row+1,sizeof(INT));
  Gtmp.JA = (INT *) calloc(Gtmp.nnz,sizeof(INT));
  Gtmp.val = (REAL *) calloc(Gtmp.nnz,sizeof(REAL));

  for(i=0;i<=nedge;i++) {
    Gtmp.IA[i] = mesh->ed_v->IA[i];
  }
  for(i=0;i<Gtmp.nnz;i++) {
    Gtmp.JA[i] = mesh->ed_v->JA[i];
  }

  for (i=0; i<nedge; i++) {
    oneoverlen = 1.0/(mesh->ed_len[i]);
    rowa = mesh->ed_v->IA[i]-1;
    j = mesh->ed_v->JA[rowa];
    k = mesh->ed_v->JA[rowa+1];
    if(j>k) {
      Gtmp.val[rowa] = oneoverlen;
      Gtmp.val[rowa+1] = -oneoverlen;
    } else { 
      Gtmp.val[rowa+1] = oneoverlen;
      Gtmp.val[rowa] = -oneoverlen;
    }
  }

  *Grad = Gtmp;
	
  return;
}
/***********************************************************************************************/

/***********************************************************************************************/
void get_curl_NedtoRT(dCSRmat* Curl,trimesh* mesh) 
{
	
  // TODO: Only makes sense in 3D and assuming lowest order elements

  /* Computes Curl operator in 3D ONLY.  Applying the resulting matrix computes the Curl of a Nedelec approximation.  
   * Takes Edge (Nedelec) DOF vector to Face (RT) DOF vector
   *    
   *    INPUT:
   *                mesh       Mesh struct
   *    OUTPUT:
   *                Curl       Matrix that takes the curl of a Nedelec approximation
   *
   */
  
  INT i,j,k,s,col_a,nd1,nd2,nd3,rowa,rowb,jcntr;
  INT ndpf = mesh->dim;
  INT mydim = mesh->dim;
  INT nface = mesh->nface;
  INT* inf = (INT *) calloc(ndpf,sizeof(INT));
  REAL vec1[3],vec2[3],vec3[3];
  REAL mydet;

  dCSRmat Ktmp;

  Ktmp.row = mesh->f_ed->row;
  Ktmp.col = mesh->f_ed->col;
  Ktmp.nnz = mesh->f_ed->nnz;
  Ktmp.IA = (INT *) calloc(Ktmp.row+1,sizeof(INT));
  Ktmp.JA = (INT *) calloc(Ktmp.nnz,sizeof(INT));
  Ktmp.val = (REAL *) calloc(Ktmp.nnz,sizeof(REAL));

  for(i=0;i<=Ktmp.row;i++) {
    Ktmp.IA[i] = mesh->f_ed->IA[i];
  }
  for(i=0;i<Ktmp.nnz;i++) {
    Ktmp.JA[i] = mesh->f_ed->JA[i];
  }

  // Get Kcurl -> if_ed,jf_ed, sign = sign(det(xi-xk,xj-xk,n_fijk))
  for (i=0;i<nface;i++) {
    // Get Normal Vector
    vec3[0] = mesh->f_norm[i*mydim];
    vec3[1] = mesh->f_norm[i*mydim+1];
    vec3[2] = mesh->f_norm[i*mydim+2];
    // Get nodes of Face
    rowa = mesh->f_v->IA[i]-1;
    rowb = mesh->f_v->IA[i+1]-1;
    jcntr=0;
    for(j=rowa;j<rowb;j++) {
      inf[jcntr] = mesh->f_v->JA[j];
      jcntr++;
    }
    // Get edges of face
    rowa = mesh->f_ed->IA[i]-1;
    rowb = mesh->f_ed->IA[i+1]-1;
    for(j=rowa;j<rowb;j++) {
      k = mesh->f_ed->JA[j];
      // Get nodes of edge
      col_a = mesh->ed_v->IA[k-1]-1;
      nd1 = mesh->ed_v->JA[col_a];
      nd2 = mesh->ed_v->JA[col_a+1];
      // Determine what other node on face is
      for(s=0;s<ndpf;s++) {
	if(inf[s]!=nd1 && inf[s]!=nd2) {
	  nd3 = inf[s];
	}
      }
      vec1[0] = mesh->cv->x[nd1-1]-mesh->cv->x[nd3-1];
      vec2[0] = mesh->cv->x[nd2-1]-mesh->cv->x[nd3-1];
      vec1[1] = mesh->cv->y[nd1-1]-mesh->cv->y[nd3-1];
      vec2[1] = mesh->cv->y[nd2-1]-mesh->cv->y[nd3-1];
      vec1[2] = mesh->cv->z[nd1-1]-mesh->cv->z[nd3-1];
      vec2[2] = mesh->cv->z[nd2-1]-mesh->cv->z[nd3-1];
      if(nd1>nd2) {
      	det3D(&mydet,vec2,vec1,vec3);
      } else {
      	det3D(&mydet,vec1,vec2,vec3);
      }
      if(mydet>0) {
	Ktmp.val[j]=1;
      } else {
	Ktmp.val[j]=-1;
      }
    }
  }

  // K -> Df^(-1) K De
  for(i=0;i<nface;i++) {
    rowa = mesh->f_ed->IA[i]-1;
    rowb = mesh->f_ed->IA[i+1]-1;
    for(j=rowa;j<rowb;j++) {
      k = mesh->f_ed->JA[j]-1;
      Ktmp.val[j] = (1.0/(mesh->f_area[i]))*( (REAL) Ktmp.val[j])*(mesh->ed_len[k]);
    }
  }	

  *Curl = Ktmp;
	
  return;
}
/***********************************************************************************************/

/***********************************************************************************************/
void get_Pigrad_H1toNed(dCSRmat* Pgrad,trimesh* mesh) 
{
  // TODO: This only makes sense for P1 elements...I think
  // Also assumes shuffled ordering

  /* Computes Gradient operator into scalar components for HX preconditioner.  
   * Ordering determined by edge to node map: bigger node is +1 smaller is -1
   *    
   *    INPUT:
   *                mesh       Mesh struct
   *    OUTPUT:
   *                Pgrad      Matrix that takes the \Pi for HX preconditioner.
   *
   */
  
  INT i,j,k,rowa,cola;
  INT nedge = mesh->nedge;
  INT dim = mesh->dim;
  REAL oneoverlen,xL,yL,zL;
  dCSRmat Ptmp;

  Ptmp.row = mesh->ed_v->row;
  Ptmp.col = (mesh->ed_v->col)*dim;
  Ptmp.nnz = (mesh->ed_v->nnz)*dim;
  Ptmp.IA = (INT *) calloc(Ptmp.row+1,sizeof(INT));
  Ptmp.JA = (INT *) calloc(Ptmp.nnz,sizeof(INT));
  Ptmp.val = (REAL *) calloc(Ptmp.nnz,sizeof(REAL));    

  for (i=0; i<nedge; i++) {
    oneoverlen = 1.0/(mesh->ed_len[i]);
    rowa = mesh->ed_v->IA[i]-1;
    Ptmp.IA[i] = rowa+1 + i*(dim-1)*2;
    cola = Ptmp.IA[i]-1;
    j = mesh->ed_v->JA[rowa];
    k = mesh->ed_v->JA[rowa+1];
    if(j>k) {
      xL = 0.5*oneoverlen*((mesh->cv->x[j-1])-(mesh->cv->x[k-1]));
      yL = 0.5*oneoverlen*((mesh->cv->y[j-1])-(mesh->cv->y[k-1]));
      if(dim==3)
	zL = 0.5*oneoverlen*((mesh->cv->z[j-1])-(mesh->cv->z[k-1]));
    } else { 
      xL = 0.5*oneoverlen*((mesh->cv->x[k-1])-(mesh->cv->x[j-1]));
      yL = 0.5*oneoverlen*((mesh->cv->y[k-1])-(mesh->cv->y[j-1]));
      if(dim==3)
	zL = 0.5*oneoverlen*((mesh->cv->z[k-1])-(mesh->cv->z[j-1]));
    }
    Ptmp.JA[cola] = (j-1)*dim+1;
    Ptmp.val[cola] = xL;
    Ptmp.JA[cola+dim] = (k-1)*dim+1;
    Ptmp.val[cola+dim] = xL;
    Ptmp.JA[cola+1] = (j-1)*dim+2;
    Ptmp.val[cola+1] = yL;
    Ptmp.JA[cola+dim+1] = (k-1)*dim+2;
    Ptmp.val[cola+dim+1] = yL;
    if(dim==3) {
      Ptmp.JA[cola+2] = (j-1)*dim+3;
      Ptmp.val[cola+2] = zL;
      Ptmp.JA[cola+dim+2] = (k-1)*dim+3;
      Ptmp.val[cola+dim+2] = zL;
    }
  }

  *Pgrad = Ptmp;
	
  return;
}
/***********************************************************************************************/

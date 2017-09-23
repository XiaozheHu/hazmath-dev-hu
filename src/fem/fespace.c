/*! \file src/fem/fespace.c
 *
 * \brief Creates and destroys the structres for the finite-element spaces.
 *
 *  Created by James Adler, Xiaozhe Hu, and Ludmil Zikatanov on 2/17/15.
 *  Copyright 2015__HAZMATH__. All rights reserved.
 *
 * \note modified by James Adler 11/14/2016
 */

#include "hazmath.h"

/****************************************************************************************/
/*!
 * \fn void initialize_fespace(fespace *FE)
 *
 * \brief Initializes all components of the structure FE.
 *
 * \return FE     Struct for FE space
 *
 */
void initialize_fespace(fespace *FE)
{
  FE->FEtype = -666;
  FE->nelm = -666;
  FE->cdof = NULL;
  FE->ndof = -666;
  FE->nbdof = -666;
  FE->dof_per_elm = -666;
  FE->el_dof = NULL;
  FE->ed_dof = NULL;
  FE->f_dof = NULL;
  FE->dirichlet = NULL;
  FE->dof_flag = NULL;
  FE->phi = NULL;
  FE->dphi = NULL;

  return;
}
/****************************************************************************************/

/****************************************************************************************/
/*!
 * \fn void create_fespace(fespace *FE,trimesh* mesh,INT FEtype)
 *
 * \brief Allocates memory and properties of fespace struct.
 *
 * \param mesh      Mesh struc
 * \param FEtype    Element Type:
 *                  0-9 PX | 10-19 QX (not yet) | 20 Ned | 30 RT | -9 - -1 DGX (not yet)
 *
 * \return FE       Struct for FE space
 *
 */
void create_fespace(fespace *FE,trimesh* mesh,INT FEtype)
{
  // Flag for errors
  SHORT status;
  INT i;

  // Initialize First
  initialize_fespace(FE);

  // Set parameters
  FE->FEtype = FEtype;
  FE->nelm = mesh->nelm;
  INT dim = mesh->dim;

  INT* dirichlet;
  INT* dof_flag;
  REAL* phi;
  REAL* dphi;

  /**/
  iCSRmat* temp;
  temp = (iCSRmat *) malloc(sizeof(struct iCSRmat));
  iCSRmat ed_el;
  iCSRmat f_el;
  iCSRmat ed_f;
  switch (FEtype)
  {
  case 0: // Contants - P0
    FE->ndof = mesh->nelm;
    coordinates *barycenter = allocatecoords(mesh->nelm,dim);
    for (i=0; i<mesh->nelm; i++) {
      barycenter->x[i] = mesh->el_mid[i*dim];
      if(mesh->dim>1)
        barycenter->y[i] = mesh->el_mid[i*dim + 1];
      if (mesh->dim>2)
        barycenter->z[i] = mesh->el_mid[i*dim + 2];
    }
    FE->cdof = barycenter;
    FE->nbdof = 0;
    FE->dof_per_elm = 1;
    FE->el_dof = malloc(sizeof(struct iCSRmat));
    *(FE->el_dof) = icsr_create_identity(mesh->nelm, 1);
    if(mesh->dim>1) {
      icsr_trans_1(mesh->el_ed,&ed_el);
      FE->ed_dof = malloc(sizeof(struct iCSRmat));
      *(FE->ed_dof) = ed_el;
      icsr_trans_1(mesh->el_f,&f_el);
      FE->f_dof = malloc(sizeof(struct iCSRmat));
      *(FE->f_dof) = f_el;
    }
    dirichlet = (INT *) calloc(mesh->nelm,sizeof(INT));
    dof_flag = (INT *) calloc(mesh->nelm,sizeof(INT));
    for(i=0;i<mesh->nelm;i++) {
      dirichlet[i] = 0;
      dof_flag[i] = 0;
    }
    FE->dirichlet = dirichlet;
    FE->dof_flag = dof_flag;
    phi = (REAL *) calloc(FE->dof_per_elm,sizeof(REAL));
    FE->phi = phi;
    break;
  case 1: // Linears - P1
    FE->cdof = mesh->cv;
    FE->ndof = mesh->nv;
    FE->nbdof = mesh->nbv;
    FE->dof_per_elm = mesh->v_per_elm;
    FE->el_dof = mesh->el_v;
    if(mesh->dim>1) {
      FE->ed_dof = mesh->ed_v;
      FE->f_dof = mesh->f_v;
    }
    dirichlet = (INT *) calloc(FE->ndof,sizeof(INT));
    dof_flag = (INT *) calloc(FE->ndof,sizeof(INT));
    for(i=0;i<FE->ndof;i++) {
      dirichlet[i] = mesh->v_bdry[i];
      dof_flag[i] = mesh->v_bdry[i];
    }
    FE->dirichlet = dirichlet;
    FE->dof_flag = dof_flag;
    phi = (REAL *) calloc(FE->dof_per_elm,sizeof(REAL));
    FE->phi = phi;
    dphi = (REAL *) calloc(FE->dof_per_elm*mesh->dim,sizeof(REAL));
    FE->dphi = dphi;
    break;
  case 2: // Quadratics - P2
    FE->ndof = mesh->nv + mesh->nelm; // In 1D
    FE->nbdof = mesh->nbv; // In 1D
    FE->dof_per_elm = mesh->v_per_elm + 1; // In 1D
    FE->el_dof = malloc(sizeof(struct iCSRmat));
    if(mesh->dim>1) { // Not in 1D
      FE->ed_dof = malloc(sizeof(struct iCSRmat));
      FE->f_dof = malloc(sizeof(struct iCSRmat));
      FE->ndof = mesh->nv + mesh->nedge;
      FE->nbdof = mesh->nbv + mesh->nbedge;
      FE->dof_per_elm = mesh->v_per_elm + mesh->ed_per_elm;
    }
    get_P2(FE,mesh);
    phi = (REAL *) calloc(FE->dof_per_elm,sizeof(REAL));
    FE->phi = phi;
    dphi = (REAL *) calloc(FE->dof_per_elm*mesh->dim,sizeof(REAL));
    FE->dphi = dphi;
    break;
  case 20: // Nedelec Elements
    FE->cdof = NULL;
    FE->ndof = mesh->nedge;
    FE->nbdof = mesh->nbedge;
    FE->dof_per_elm = mesh->ed_per_elm;
    FE->el_dof = mesh->el_ed;
    FE->ed_dof = malloc(sizeof(struct iCSRmat));
    *(FE->ed_dof) = icsr_create_identity(mesh->nedge, 1);
    FE->f_dof = mesh->f_ed;
    dirichlet = (INT *) calloc(FE->ndof,sizeof(INT));
    dof_flag = (INT *) calloc(FE->ndof,sizeof(INT));
    for(i=0;i<FE->ndof;i++) {
      dirichlet[i] = mesh->ed_bdry[i];
      dof_flag[i] = mesh->ed_bdry[i];
    }
    FE->dirichlet = dirichlet;
    FE->dof_flag = dof_flag;
    phi = (REAL *) calloc(FE->dof_per_elm*mesh->dim,sizeof(REAL));
    FE->phi = phi;
    if(mesh->dim==2) // Scalar Curl
      dphi = (REAL *) calloc(FE->dof_per_elm,sizeof(REAL));
    else // Vector Curl
      dphi = (REAL *) calloc(FE->dof_per_elm*mesh->dim,sizeof(REAL));
    FE->dphi = dphi;
    break;
  case 30: // Raviart-Thomas Elements
    FE->cdof = NULL;
    FE->ndof = mesh->nface;
    FE->nbdof = mesh->nbface;
    FE->dof_per_elm = mesh->f_per_elm;
    FE->el_dof = mesh->el_f;
    icsr_trans_1(mesh->f_ed,&ed_f);
    FE->ed_dof = malloc(sizeof(struct iCSRmat));
    *(FE->ed_dof) = ed_f;
    FE->f_dof = malloc(sizeof(struct iCSRmat));
    *(FE->f_dof) = icsr_create_identity(mesh->nface, 1);
    dirichlet = (INT *) calloc(FE->ndof,sizeof(INT));
    dof_flag = (INT *) calloc(FE->ndof,sizeof(INT));
    for(i=0;i<FE->ndof;i++) {
      dirichlet[i] = mesh->f_bdry[i];
      dof_flag[i] = mesh->f_bdry[i];
    }
    FE->dirichlet = dirichlet;
    FE->dof_flag = dof_flag;
    phi = (REAL *) calloc(FE->dof_per_elm*mesh->dim,sizeof(REAL));
    FE->phi = phi;
    dphi = (REAL *) calloc(FE->dof_per_elm,sizeof(REAL));
    FE->dphi = dphi;
    break;
  case 60: // Vector Velocity
    FE->cdof = NULL;
    FE->ndof = mesh->nv * mesh->dim;
    FE->nbdof = mesh->nbv * mesh->dim;
    FE->dof_per_elm = mesh->v_per_elm * mesh->dim;
    // FE->el_dof = mesh->el_v; // iCSRmat*
    FE->el_dof = malloc(sizeof(struct iCSRmat));
    printf("Before Concat\n");
    if(mesh->dim==1){
      FE->el_dof = mesh->el_v;
    } else if(mesh->dim==2){
      icsr_shift(mesh->el_v, -1);
      icsr_concat(mesh->el_v, mesh->el_v, FE->el_dof);
      icsr_shift(mesh->el_v, 1);
      icsr_shift(FE->el_dof, 1);
    } else if(mesh->dim==3){
      icsr_shift(mesh->el_v, -1);
      icsr_concat(mesh->el_v,mesh->el_v,temp);
      icsr_concat(temp,mesh->el_v,FE->el_dof);
      icsr_shift(mesh->el_v, 1);
      icsr_shift(FE->el_dof, 1);
      icsr_free(temp);
    }
    printf("After Concat\n");
    if(mesh->dim>1) {
      //  FE->ed_dof = mesh->ed_v; // iCSRmat*
      FE->ed_dof = malloc(sizeof(struct iCSRmat));
      if(mesh->dim==2){
        icsr_shift(mesh->ed_v, -1);
        icsr_concat(mesh->ed_v, mesh->ed_v, FE->ed_dof);
        icsr_shift(mesh->ed_v, 1);
        icsr_shift(FE->ed_dof, 1);
      } else if(mesh->dim==3) {
        icsr_shift(mesh->ed_v, -1);
        icsr_concat(mesh->ed_v, mesh->ed_v, temp);
        icsr_concat(temp, mesh->ed_v, FE->ed_dof);
        icsr_shift(mesh->ed_v, 1);
        icsr_shift(FE->ed_dof, 1);
        icsr_free(temp);
      }
      //  FE->f_dof = mesh->f_v; // iCSRmat*
      FE->f_dof = malloc(sizeof(struct iCSRmat));
      if(mesh->dim==2){
        icsr_shift(mesh->f_v, -1);
        icsr_concat(mesh->f_v, mesh->f_v, FE->f_dof);
        icsr_shift(mesh->f_v, 1);
        icsr_shift(FE->f_dof, 1);
      } else if(mesh->dim==3){
        icsr_shift(mesh->f_v, -1);
        icsr_concat(mesh->f_v, mesh->f_v, temp);
        icsr_concat(temp, mesh->f_v, FE->f_dof);
        icsr_shift(mesh->f_v, 1);
        icsr_shift(FE->f_dof, 1);
        icsr_free(temp);
      }
    }
    dirichlet = (INT *) calloc(FE->ndof,sizeof(INT));
    dof_flag = (INT *) calloc(FE->ndof,sizeof(INT));
    for(i=0;i<FE->ndof;i++) {
      dirichlet[i] = mesh->v_bdry[i % mesh->nv];
      dof_flag[i] = mesh->v_bdry[i % mesh->nv];
    }
    FE->dirichlet = dirichlet;
    FE->dof_flag = dof_flag;
    phi = (REAL *) calloc(FE->dof_per_elm,sizeof(REAL)); // This is basis functions
    FE->phi = phi;
    dphi = (REAL *) calloc(FE->dof_per_elm*mesh->dim,sizeof(REAL));
    FE->dphi = dphi;
    break;
  case 61: // Bubbles
    FE->cdof = NULL;
    FE->ndof = mesh->nface;
    FE->nbdof = mesh->nbface;
    FE->dof_per_elm = mesh->f_per_elm;
    FE->el_dof = mesh->el_f;
    icsr_trans_1(mesh->f_ed,&ed_f);
    FE->ed_dof = malloc(sizeof(struct iCSRmat));
    *(FE->ed_dof) = ed_f;
    FE->f_dof = malloc(sizeof(struct iCSRmat));
    *(FE->f_dof) = icsr_create_identity(mesh->nface, 1);;
    dirichlet = (INT *) calloc(FE->ndof,sizeof(INT));
    dof_flag = (INT *) calloc(FE->ndof,sizeof(INT));
    for(i=0;i<FE->ndof;i++) {
      dirichlet[i] = mesh->f_bdry[i];
      dof_flag[i] = mesh->f_bdry[i];
    }
    FE->dirichlet = dirichlet;
    FE->dof_flag = dof_flag;
    phi = (REAL *) calloc(FE->dof_per_elm*mesh->dim,sizeof(REAL));
    FE->phi = phi;
    dphi = (REAL *) calloc(FE->dof_per_elm*mesh->dim*mesh->dim,sizeof(REAL));
    FE->dphi = dphi;
    break;
  default:
    status = ERROR_FE_TYPE;
    check_error(status, __FUNCTION__);
  }

  // Assume all DOF are not periodic to start
  FE->periodic = (INT *) calloc(FE->ndof,sizeof(INT));
  for(i=0;i<FE->ndof;i++) {
    FE->periodic[i] = -1;
  }
  
  return;
}
/****************************************************************************************/

/****************************************************************************************/
/*!
 * \fn void free_fespace(fespace* FE)
 *
 * \brief Frees memory of arrays of fespace struct
 *
 * \return FE       Struct for FE space to be freed
 *
 */
void free_fespace(fespace* FE)
{
  if(FE->cdof && (FE->FEtype!=1)) { // If Linears, free_mesh will destroy coordinate struct
    free_coords(FE->cdof);
    free(FE->cdof);
    FE->cdof = NULL;
  }

  if(FE->el_dof && (FE->FEtype==2 || FE->FEtype==0)) { // If not P2 or P0, free_mesh will destroy el_dof struct
    icsr_free(FE->el_dof);
    free(FE->el_dof);
    FE->el_dof = NULL;
  }

  if(FE->ed_dof && FE->FEtype!=1) { // If Linears, free_mesh will destroy ed_dof struct
    icsr_free(FE->ed_dof);
    free(FE->ed_dof);
    FE->ed_dof = NULL;
  }
  
  if(FE->f_dof && FE->FEtype!=1 && FE->FEtype!=20) { // If Linears or Nedelec, free_mesh will destroy f_dof
    icsr_free(FE->f_dof);
    free(FE->f_dof);
    FE->f_dof = NULL;
  }

  if(FE->dirichlet) {
    free(FE->dirichlet);
    FE->dirichlet = NULL;
  }

  if(FE->dof_flag) {
    free(FE->dof_flag);
    FE->dof_flag = NULL;
  }

  if(FE->phi) {
    free(FE->phi);
    FE->phi = NULL;
  }

  if(FE->dphi) {
    free(FE->dphi);
    FE->dphi = NULL;
  }
  
  return;
}
/****************************************************************************************/

/****************************************************************************************/
/*!
 * \fn void free_blockfespace(block_fespace* FE)
 *
 * \brief Frees memory of arrays of block_fespace struct
 *
 * \return FE       Struct for BLOCK FE space to be freed
 *
 */
void free_blockfespace(block_fespace* FE)
{
  if (FE == NULL) return; // Nothing need to be freed!

  INT i;
  INT num_spaces = (FE->nspaces);

  for ( i=0; i<num_spaces; i++ ) {
    free_fespace(FE->var_spaces[i]);
    FE->var_spaces[i] = NULL;
  }

  free(FE->var_spaces);
  FE->var_spaces = NULL;

  if(FE->dirichlet) {
    free(FE->dirichlet);
    FE->dirichlet = NULL;
  }

  if(FE->dof_flag) {
    free(FE->dof_flag);
    FE->dof_flag = NULL;
  }
  
  return;
}
/****************************************************************************************/


/***********************************************************************************************/
/*!
 * \fn void get_P2(fespace* FE,trimesh* mesh)
 *
 * \brief Converts mesh data to account for P2 elements
 *
 * \param mesh      Mesh struct
 *
 * \return FE       Struct for P2 FE space
 *
 */
void get_P2(fespace* FE,trimesh* mesh) 
{
  // Loop indices
  INT i,j,k,s,jcntr;
  INT n1,n2,n3,va,vb,ea,eb,v1,v2,mye,ed,na,ed1,ed2,face;

  INT ndof = FE->ndof;
  INT dof_per_elm = FE->dof_per_elm;
  INT dim = mesh->dim;
  INT nv = mesh->nv;
  INT nedge = mesh->nedge;
  INT nface = mesh->nface;
  INT nelm = mesh->nelm;
  INT v_per_elm = mesh->v_per_elm;
  iCSRmat *el_v = mesh->el_v;
  iCSRmat *el_ed = NULL;
  iCSRmat *ed_v = NULL;
  iCSRmat *f_v = NULL;
  iCSRmat *f_ed = NULL;

  if(dim>1) {
    el_ed = mesh->el_ed;
    ed_v = mesh->ed_v;
    f_v = mesh->f_v;
    f_ed = mesh->f_ed;
  }

  INT* ipv = (INT *) calloc(mesh->v_per_elm,sizeof(INT));

  // Get Coordinates
  coordinates *cdof = allocatecoords(ndof,dim);

  // First go through all vertices.
  for (i=0; i<nv; i++) {
    cdof->x[i] = mesh->cv->x[i];
    if(dim>1)
      cdof->y[i] = mesh->cv->y[i];
    if (dim>2)
      cdof->z[i] = mesh->cv->z[i];
  }
  // Now, go through and add extra nodes
  s = nv;

  // In 1D this is just the midpoint of the elements
  if(dim==1) {
    for(i=0;i<nelm;i++) {
      cdof->x[s] = mesh->el_mid[i];
      s++;
    }
  } else { // In 2D or 3D, these are simply the midpoints of the edges
    for (i=0; i<nedge; i++) {
      ed = ed_v->IA[i]-1;
      n1 = ed_v->JA[ed]-1;
      n2 = ed_v->JA[ed+1]-1;
      cdof->x[s] = 0.5*(mesh->cv->x[n1]+mesh->cv->x[n2]);
      cdof->y[s] = 0.5*(mesh->cv->y[n1]+mesh->cv->y[n2]);
      if (dim>2) { cdof->z[s] = 0.5*(mesh->cv->z[n1]+mesh->cv->z[n2]); }
      s++;
    }
  }
  // Get Element to Node map
  iCSRmat el_n = icsr_create(nelm,ndof,dof_per_elm*nelm);
  // Rows of Element to Node map
  for(i=0;i<nelm+1;i++) {
    el_n.IA[i] = dof_per_elm*i + 1;
  }
  // Columns
  // In 1D just add the midpoint of elements
  if(dim==1) {
    for(i=0;i<nelm;i++) {
      va = el_v->IA[i]-1;
      vb = el_v->IA[i+1]-1;
      na = el_n.IA[i]-1;
      jcntr = 0;
      for(j=va;j<vb;j++) {
        el_n.JA[na + jcntr] = el_v->JA[j];
        jcntr++;
      }
      el_n.JA[na+jcntr] = nv+i+1;
    }
  } else {
    // For 2D or 3D we add the midpoint of the edges
    for(i=0;i<nelm;i++) {
      va = el_v->IA[i]-1;
      vb = el_v->IA[i+1]-1;
      ea = el_ed->IA[i]-1;
      eb = el_ed->IA[i+1]-1;
      na = el_n.IA[i]-1;
      jcntr = 0;
      for(j=va;j<vb;j++) {
        ipv[jcntr] = el_v->JA[j];
        el_n.JA[na+jcntr] = ipv[jcntr];
        jcntr++;
      }
      for (j=0;j<v_per_elm;j++) {
        for (k=j+1;k<v_per_elm;k++) {
          v1 = ipv[j];
          v2 = ipv[k];
          for(s=ea;s<eb;s++) {
            mye = el_ed->JA[s];
            n1 = ed_v->JA[ed_v->IA[mye-1]-1];
            n2 = ed_v->JA[ed_v->IA[mye-1]];
            if ((n1==v1 && n2==v2) || (n2==v1 && n1==v2)) {
              el_n.JA[v_per_elm+na] = mye+nv;
              na++;
            }
          }
        }
      }
    }
  }

  // Fix Boundaries
  INT* dirichlet = (INT *) calloc(ndof,sizeof(INT));
  INT* dof_flag = (INT *) calloc(ndof,sizeof(INT));
  // First set of nodes are vertices
  for (i=0; i<nv; i++) {
    dirichlet[i] = mesh->v_bdry[i];
    dof_flag[i] = mesh->v_bdry[i];
  }
  // In 1D rest are interior
  if(dim==1) {
    for(i=0;i<nelm;i++) {
      dirichlet[nv+i] = 0;
      dof_flag[nv+i] = 0;
    }
  } else {
    // In 2D or 3D, rest are edges
    for(i=0;i<nedge;i++) {
      dirichlet[nv+i] = mesh->ed_bdry[i];
      dof_flag[nv+i] = mesh->ed_bdry[i];
    }
  }

  // Get edge to DOF map
  if(dim>1) {
    iCSRmat ed_n = icsr_create(nedge,ndof,3*nedge);
    s = nv+1;
    for (i=0; i<nedge; i++) {
      ed = ed_v->IA[i]-1;
      ed_n.IA[i] = ed+1+i;
      n1 = ed_v->JA[ed]-1;
      n2 = ed_v->JA[ed+1]-1;
      ed_n.JA[ed+i] = n1+1;
      ed_n.JA[ed+1+i] = n2+1;
      ed_n.JA[ed+2+i] = s;
      s++;
    }
    ed_n.IA[nedge] = 3*nedge+1;

    // Get face to DOF map
    INT n_per_f = 3*(dim-1);
    INT extra_n = 2*dim-3;
    iCSRmat f_n = icsr_create(nface,ndof,n_per_f*nface);

    for (i=0; i<nface; i++) {
      face = f_v->IA[i]-1;
      f_n.IA[i] = face+1+(2*dim-3)*i;
      n1 = f_v->JA[face]-1;
      n2 = f_v->JA[face+1]-1;
      if(dim==3) n3 = f_v->JA[face+2]-1;
      f_n.JA[face+extra_n*i] = n1+1;
      f_n.JA[face+extra_n*i+1] = n2+1;
      if(dim==3) f_n.JA[face+extra_n*i+2] = n3+1;
      // Fill in rest with edge numbers
      ed1 = f_ed->IA[i]-1;
      ed2 = f_ed->IA[i+1]-1;
      jcntr = 0;
      for(j=ed1;j<ed2;j++) {
        f_n.JA[face+extra_n*i+dim+jcntr] = f_ed->JA[j]+nv;
        jcntr++;
      }
    }
    f_n.IA[nface] = n_per_f*nedge+1;
    *(FE->ed_dof) = ed_n;
    *(FE->f_dof) = f_n;
  }

  FE->dirichlet = dirichlet;
  FE->dof_flag = dof_flag;
  *(FE->el_dof) = el_n;
  FE->cdof = cdof;
  
  if(ipv) free(ipv);

  return;
}
/******************************************************************************************/

/****************************************************************************************/
/*!
 * \fn void dump_el_dof(FILE* fid,iCSRmat *el_dof)
 *
 * \brief Dump the element to DOF map to file for plotting purposes.
 *
 * \param el_dof      Element to DOF map
 * \param fid         Output FILE ID
 *
 * \return el_dof.dat Output file with el_dof data
 *
 */
void dump_el_dof(FILE* fid,iCSRmat *el_dof) 
{
  // Loop indices
  INT i,j,acol,bcol;

  for (i=0; i<el_dof->row; i++) {
    acol = el_dof->IA[i]-1;
    bcol = el_dof->IA[i+1]-1;
    for (j=acol; j<bcol; j++) {
      fprintf(fid,"%d\t",el_dof->JA[j]);
    }
    fprintf(fid,"\n");
  }

  return;
}
/****************************************************************************************/

/****************************************************************************************/
/*!
 * \fn void dump_fespace(fespace *FE,char *varname,char *dir)
 *
 * \brief Dump the FE space data to file for plotting purposes
 *
 * \param FE               FE space to dump
 * \param varname          Output file name
 * \param dir              Directory to dump data
 *
 * \return dir/varname.dat Output file with FE data
 *
 */
void dump_fespace(fespace *FE,char *varname,char *dir) 
{
  INT i;
  INT totdof = FE->ndof;
  char eldofname[20];
  char bdryname[20];

  sprintf(eldofname,"%s/el_dof_%s.dat",dir,varname);
  sprintf(bdryname,"%s/bdry_%s.dat",dir,varname);
  FILE* fid1 = fopen(eldofname,"w");
  FILE* fid2 = fopen(bdryname,"w");

  if(fid1==NULL || fid2==NULL) {
    printf("\n\nFilenames: %s\t%s are incorrect or do not exist.  No files dumped.\n\n",eldofname,bdryname);
  } else {
    // Dump Element to DOF map
    dump_el_dof(fid1,FE->el_dof);

    // Dump boundary data
    for(i=0;i<totdof;i++) {
      fprintf(fid2,"%d\n",FE->dirichlet[i]);
    }

    fclose(fid1);
    fclose(fid2);
  }
  return;
}
/****************************************************************************************/

/****************************************************************************************/
/*!
 * \fn void set_dirichlet_bdry(fespace* FE,trimesh* mesh, const INT flag0, const INT flag1)
 *
 * \brief Determine which boundary DOF are Dirichlet.  Determined by the FE space type
 *        and by the given flag from the mesh file.
 *
 * \param FE               FE space struct
 * \param mesh             Mesh struct
 * \param flag0            min flag value for Dirichlet DOF 
 *                         (e.g. in fem.h: MARKER_DIRICHLET)
 * \param flag1            max flag value for Dirichlet DOF  
 *                         e.g. in fem.h (MARKER_NEUMANN - 1)
 *
 * \return FE.dirichlet    Binary boundary array for DOF
 *
 */
void set_dirichlet_bdry(fespace* FE,trimesh* mesh, const INT flag0, const INT flag1) 
{
  INT i;
  for(i=0;i<FE->ndof;i++) {
    if((FE->dof_flag[i])>=flag0 && (FE->dof_flag[i]<=flag1)) {
      FE->dirichlet[i] = 1;
    } else {
      FE->dirichlet[i] = 0;
    }
  }
  return;
}
/****************************************************************************************/

/****************************************************************************************/
/*!
 * \fn void set_dirichlet_bdry_block(fespace* FE,trimesh* mesh)
 *
 * \brief Determine which boundary DOF are Dirichlet.  Determined by the BLOCK FE space type
 *        and by the given flag from the mesh file.
 *
 * \param FE               BLOCK FE space struct
 * \param mesh             Mesh struct
 *
 * \return FE.dirichlet    Binary boundary array for DOF
 * \return FE.dof_flag     Also set DOF flags based on each FE space
 *
 */
void set_dirichlet_bdry_block(block_fespace* FE,trimesh* mesh)
{
  INT i,j,ndof,cnt;

  INT* isdirichlet = (INT *) calloc(FE->ndof,sizeof(INT));
  INT* dof_flags = (INT *) calloc(FE->ndof,sizeof(INT));

  cnt = 0;
  for(i=0;i<FE->nspaces;i++) {
    ndof = FE->var_spaces[i]->ndof;
    for(j=0;j<ndof;j++) {
      isdirichlet[cnt+j] = FE->var_spaces[i]->dirichlet[j];
      dof_flags[cnt+j] = FE->var_spaces[i]->dof_flag[j];
    }
    cnt += ndof;
  }

  FE->dirichlet = isdirichlet;
  FE->dof_flag = dof_flags;
  return;
}
/****************************************************************************************/

/******************************************************************************/
/*!
 * \fn void get_incidence_row(INT row,iCSRmat *fem_map,INT* thisrow)
 *
 * \brief Gets single row of an incidence map (i.e., gets vertices of given element from el_v)
 *
 * \param row       Row to grab (indexed at 0)
 * \param fem_map   Incidence matrix to grab
 *
 * \return thisrow  Given row of the incidence matrix
 *
 */
void get_incidence_row(INT row,iCSRmat *fem_map,INT* thisrow)
{
  INT j;
  INT rowa = fem_map->IA[row]-1;
  INT rowb = fem_map->IA[row+1]-1;
  INT jcntr = 0;
  for (j=rowa; j<rowb; j++) {
    thisrow[jcntr] = fem_map->JA[j];
    jcntr++;
  }

  return;
}
/******************************************************************************/

/******************************************************************************/
/*!
 * \fn void get_coords(REAL* x,INT dof,coordinates* cdof,INT dim)
 *
 * \brief Gets coordinates of a particular DOF
 *
 * \param dof       DOF to grab (indexed at 0)
 * \param cdof      Coordinate struct to grab from
 * \param dim
 *
 * \return x        (x,y,z) coordinates
 *
 */
void get_coords(REAL* x,INT dof,coordinates* cdof,INT dim)
{
  x[0] = cdof->x[dof];
  if(dim==2 || dim==3)
    x[1] = cdof->y[dof];
  if(dim==3)
    x[2] = cdof->z[dof];

  return;
}
/******************************************************************************/

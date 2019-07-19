#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <getopt.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include "hazmath.h"
#include "grid_defs.h"
#ifndef INT
#define INT int
#endif
#ifndef REAL
#define REAL double
#endif
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
scomplex *umesh(INT *iindex,				\
		INT *nsall0,				\
		INT *nvall0,				\
		const INT nsall,			\
		const INT nvall,			\
		const INT dim,				\
		INT *nd, cube2simp *c2s,		\
		INT *isbndf, INT *codef,		\
		INT elflag,				\
		const INT face, const INT face_parent,	\
		const scomplex *sc_parent,		\
		INT *nd_parent,				\
		INT *iindex_parent,			\
		const INT intype);
void unirefine(INT *nd,scomplex *sc);
iCSRmat *set_mmesh(input_grid *g0,					\
		   cube2simp *c2s,					\
		   ivector *etree0,					\
		   INT **elneib, INT **el2fnum,				\
		   ivector *isbface0, ivector *bcodesf0,		\
		   INT *cc,  INT *bndry_cc,				\
		   INT *wrk);
void set_edges(input_grid *g0,cube2simp *c2s);
INT set_ndiv_edges(input_grid *g,		\
		   input_grid *g0,		\
		   cube2simp *c2s,		\
		   INT **nd,			\
		   const INT iter);
void map2mac(scomplex *sc,cube2simp *c2s, input_grid *g);
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
void scomplex_merge(scomplex **sc0,			\
		    const INT nsall, const INT nvall,	\
		    const INT cc, const INT bndry_cc,	\
		    input_grid *g0,cube2simp *c2s)
{
  /* combains an array of simplicial complexes together */
  if(g0->nel==1) return;
  scomplex *sc=sc0[0];
  INT n1=(sc->n+1),nv0,ns0,nv=nvall,ns=nsall;
  INT kel,i,ii,j,in1,iin1;
  sc->marked=realloc(sc->marked,ns*sizeof(INT));
  sc->gen=realloc(sc->gen,ns*sizeof(INT));
  sc->nbr=realloc(sc->nbr,ns*n1*sizeof(INT));
  sc->parent=realloc(sc->parent,ns*sizeof(INT));
  sc->child0=realloc(sc->child0,ns*sizeof(INT));
  sc->childn=realloc(sc->childn,ns*sizeof(INT));
  sc->nodes=realloc(sc->nodes,ns*n1*sizeof(INT));
  sc->bndry=realloc(sc->bndry,nv*sizeof(INT));
  sc->csys=realloc(sc->csys,nv*sizeof(INT));/* coord sys: 1 is polar, 2
					    is cyl and so on */
  /*connected components*/
  sc->cc=cc;sc->bndry_cc=bndry_cc;
  sc->flags=(INT *)realloc(sc->flags,ns*sizeof(INT));
  sc->x=(REAL *)realloc(sc->x,nv*(sc->n)*sizeof(REAL));
  sc->vols=(REAL *)realloc(sc->vols,ns*sizeof(REAL));
  sc->fval=(REAL *)realloc(sc->fval,nv*sizeof(REAL)); // function values at every vertex; not used in general;
  //  fprintf(stdout,"\nnsall=%d,nvall=%d",nsall,nvall);fflush(stdout);
  for(kel=1;kel<g0->nel;kel++){
    ns0=sc->ns;nv0=sc->nv;
    for (ii = 0;ii<sc0[kel]->ns;ii++) {
      i=ii+ns0;
      sc->marked[i] = sc0[kel]->marked[ii];
      sc->gen[i] = sc0[kel]->gen[ii];
      sc->parent[i]=sc0[kel]->parent[ii];
      sc->child0[i]=sc0[kel]->child0[ii];
      sc->childn[i]=sc0[kel]->childn[ii];
      sc->flags[i]=sc0[kel]->flags[ii];
      sc->vols[i]=sc0[kel]->vols[ii];
      in1=i*n1;
      iin1=ii*n1;
      for(j=0;j<n1;j++){
	sc->nodes[in1+j]=sc0[kel]->nodes[iin1+j]+nv0;
	sc->nbr[in1+j]=sc0[kel]->nbr[iin1+j]+ns0;
      }
    }
    for (ii = 0;ii<sc0[kel]->nv;ii++) {
      i=ii+nv0;
      sc->bndry[i]=sc0[kel]->bndry[ii];
      sc->csys[i]=sc0[kel]->csys[ii];
      sc->fval[i]=sc0[kel]->fval[ii];
      in1=i*sc->n;
      iin1=ii*sc->n;
      for(j=0;j<sc->n;j++)
	sc->x[in1+j]=sc0[kel]->x[iin1+j];
    }
    sc->nv+=sc0[kel]->nv;
    sc->ns+=sc0[kel]->ns;
    haz_scomplex_free(sc0[kel]);
  }
  //  fprintf(stdout,"\nsc->nv=%d,sc->ns=%d; nvall=%d,nsall=%d\n",sc->nv,sc->ns,nvall,nsall);fflush(stdout);
  return;
}
/*ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ*/
scomplex *macro_split(input_grid *g0,cube2simp *c2s)
{
  /* 
     From an input grid loops over the macroelements and generates
     meshes depending on the division given in every dimension. 
  */
  input_grid *g;
  INT i,j,j0,j1,kel,ke,pmem;
  INT nvcube=c2s->nvcube,nvface=c2s->nvface;
  scomplex **sc=malloc(g0->nel*sizeof(scomplex *));
  pmem=2*g0->nv;
  if(pmem<2*g0->ne) pmem=2*g0->ne;
  if(pmem<2*g0->nel) pmem=2*g0->nel;
  if(pmem<2*g0->nf) pmem=2*g0->nf;
  INT *p=calloc(pmem,sizeof(INT));
  //  print_full_mat_int(g0->nel,(c2s->nvcube+1),g0->mnodes,"ex");
  ilexsort(g0->nf, (c2s->nvface+1),g0->mfaces,p);
  ilexsort(g0->nel,(c2s->nvcube+1),g0->mnodes,p);
  /*-------------------------------------------------------------------*/
  //  INT *efound=calloc(c2s->ne*(g0->nel),sizeof(INT));
  INT **nd=calloc(g0->nel,sizeof(INT *));
  for(i=0;i<g0->nel;i++){
    nd[i]=calloc(c2s->n,sizeof(INT)); /* to hold the number of
 					 divisions in every coordinate
					 direction */
  }
  for(kel=0;kel<g0->nel;kel++)
    for(i=0;i<c2s->nf;i++){
      nd[kel][i]=-1;
    }
  g=malloc(1*sizeof(input_grid));
  /**/
  g->title=g0->title;
  g->dgrid=g0->dgrid;
  g->fgrid=g0->fgrid;
  g->dvtu=g0->dvtu;
  g->fvtu=g0->fvtu;
  g->print_level=g0->print_level;
  g->ref_type=g0->ref_type;
  g->nref=g0->nref;
  g->err_stop=g0->err_stop;
  /**/
  g->dim=c2s->n;
  g->ncsys=g0->ncsys;
  g->nv=c2s->nvcube;
  g->nf=c2s->nf;
  g->ne=c2s->ne;
  g->nel=1;
  input_grid_arrays(g);
  /* reassign this as these are the same as g0 */
  free(g->systypes);   g->systypes=g0->systypes;
  free(g->syslabels);   g->syslabels=g0->syslabels;
  free(g->ox); g->ox=g0->ox;
  INT chng=1,iter=0,maxiter=1024;  
  //  INT je,kj,k2,iel2v,jel2v,k1,kface,kbnd,found;
  set_edges(g0,c2s);
  while(chng && (iter<maxiter)){
    iter++;
    // make the divisions in g0->seg consistent;
    chng=set_ndiv_edges(g,g0,c2s,nd,iter);
  }
  //  print_full_mat_int(g0->ne,3,g0->seg,"newseg");  
  /*-------------------------------------------------------------------*/
  // here we should end the function which sets all divisions
  /*-------------------------------------------------------------------*/  
  // form macroelement neighboring list. Use transposition; the sparse
  // matrices here are only used to compute the number of connected
  // components in the domain or on the boundary.  
  INT **elneib=calloc(g0->nel,sizeof(INT *));/* element neighboring
					       list where the
					       position of the
					       neighbor is the same
					       as the position of the
					       face in c2s->faces
					       shared by the two
					       el.
					    */
  INT **el2fnum=calloc(g0->nel,sizeof(INT *)); /* for every element
						 this gives the local
						 to global face number
						 map;
					      */
  for(i=0;i<g0->nel;i++){
    elneib[i]=calloc(c2s->nf,sizeof(INT)); /* to hold the neighbors */
    el2fnum[i]=calloc(c2s->nf,sizeof(INT)); /* to hold the face
					       numbers for every
					       element */
  }
  ivector *bcodesf=malloc(1*sizeof(ivector));
  ivector *isbface=malloc(1*sizeof(ivector));
  ivector *etree=malloc(1*sizeof(ivector));
  INT cc=1, bndry_cc=1;
  iCSRmat *bfs0=set_mmesh(g0,c2s,etree,		\
			  elneib, el2fnum,	\
			  isbface, bcodesf,&cc,&bndry_cc,p);
  fprintf(stdout,"\n%%DFS(domains): %d connected components",cc);
  fprintf(stdout,"\n%%DFS(boundaries): %d connected components",bndry_cc);
  /* fprintf(stdout,"\nbfs0=["); */
  /* icsr_print_matlab_val(stdout,bfs0); */
  /* fprintf(stdout,"];"); */
  /* fprintf(stdout,"\nbfs=sparse(bfs0(:,1),bfs0(:,2),bfs0(:,3));\n"); */
  /* print_full_mat_int(1,isbface->row,isbface->val,"isbface"); */
  /* fprintf(stdout,"\n*****   nf=%d (%d)******* \n",g0->nf,isbface->row); */
  /***********************************************************************/    
  /* set the divisions on every edge now; since they are consistent we
   have: */
  if(set_ndiv_edges(g,g0,c2s,nd,0)) {
    fprintf(stderr,"\n\n***ERR in %s: the divisions of the edges cannod be inconsistent during second call of set_ndiv_edges()\n\n",__FUNCTION__);
    exit(4);
  }
  /* for(kel=0;kel<g0->nel;kel++){ */
  /*   print_full_mat_int(1,c2s->nf,elneib[kel],"neib"); */
  /* } */
  for(kel=0;kel<g0->nel;kel++){
    for(i=0;i<c2s->n;i++){
      if(nd[kel][i]<=0) nd[kel][i]=1;
    }
    //    print_full_mat_int(1,c2s->n,nd[kel],"ndnd"); 
  }
  // use bfs to split elements:
  if(g0->print_level>3) input_grid_print(g0);
  INT lvl,intype=-1,kj,je,jel;  
  INT nsall,nvall,nsall0,nvall0,elflag;
  INT *codef=calloc(c2s->nf,sizeof(INT));
  INT *isbndf=calloc(c2s->nf,sizeof(INT));
  INT **iindex=malloc(g0->nel*sizeof(INT *));
  for(kel=0;kel<g0->nel;kel++){
    nvall=1;
    for(i=0;i<g0->dim;i++)
      nvall*=(nd[kel][i]+1);
    iindex[kel]=calloc(nvall,sizeof(INT));
  }
  nsall0=0;nvall0=0;
  nsall=0;
  nvall=0;
  for(lvl=0;lvl<bfs0->row;lvl++){    
    j0=bfs0->IA[lvl];
    j1=bfs0->IA[lvl+1];
    for(kj=j0;kj<j1;kj++){
      jel=bfs0->JA[kj];
      //      fprintf(stdout,"\n **** %d",jel);
      //      print_full_mat_int(1,c2s->nf,elneib[jel],"neib");
      kel=etree->val[jel];// ancestor, this stays unchanged
      memcpy(g->mnodes,(g0->mnodes+jel*(nvcube+1)),(nvcube+1)*sizeof(INT));
      for(i=0;i<c2s->nvcube;i++){
	j=g->mnodes[i];// vertex number (global)
	g->csysv[i]=g0->csysv[j]; 
	g->labelsv[i]=g0->csysv[j];
	memcpy((g->xv+i*g->dim),(g0->xv+j*g0->dim),g->dim*sizeof(REAL));
      }
      /***************************************************/
      /*elment and face codes:*/
      elflag=g->mnodes[nvcube];
      for(i=0;i<c2s->nf;i++){
	codef[i]=bcodesf->val[el2fnum[jel][i]];
	// locally: is boundary face?
	isbndf[i]=isbface->val[el2fnum[jel][i]];       
      }      
      //      print_full_mat_int(1,c2s->nf,codef,"bcodes"); fflush(stdout);
      //      print_full_mat_int(1,c2s->nf,isbndf,"isbnd"); fflush(stdout);
      if(kel<0){
	sc[jel]=umesh(iindex[jel],	                \
		      &nsall0,&nvall0,nsall,nvall,	\
		      g->dim,nd[jel],c2s,		\
		      isbndf,codef,elflag,		\
		      -1,-1,NULL,NULL,NULL,intype);
      } else {
	je=locate0(jel,elneib[kel], c2s->nf);
	ke=locate0(kel,elneib[jel], c2s->nf);
	fprintf(stdout,"\nface(%d) in a divided el(%d) is also face(%d) in the current el(%d)",je,kel,ke,jel);
	/* je=face_parent (was divide),ke= face (to be divided now)*/
	sc[jel]=umesh(iindex[jel],					\
		      &nsall0,&nvall0,nsall,nvall,			\
		      g->dim,nd[jel],c2s,				\
		      isbndf,codef,elflag,				\
		      ke,je,sc[kel],nd[kel],iindex[kel],intype);
      }
      /*copy vertices and coord systems*/
      nsall+=sc[jel]->ns;
      nvall+=sc[jel]->nv;
      //      fprintf(stdout,"\n%%Mapping back to the macroelement...\n");
      map2mac(sc[jel],c2s,g);
      fprintf(stdout,"\n%%mesh(macroelement=%d): nv=%d; nsimp=%d",jel,sc[jel]->nv,sc[jel]->ns);      
      //      haz_scomplex_print(sc[jel],0,"HAHA");
    }
  }
  //  fprintf(stdout,"\n%%Generated a uniform mesh in dim=%d; total number of simplices=%d",g->dim,nsall);      
  scomplex_merge(sc,			\
		 nsall, nvall,		\
		 cc, bndry_cc,		\
		 g0,c2s);
  fprintf(stdout,"\n%%");
  fprintf(stdout,"\n%%merged(macroelements=%d:%d): nv=%d; nsimp=%d",0,g0->nel-1,sc[0]->nv,sc[0]->ns);      
  fprintf(stdout,"\n%%\n");
  //  haz_scomplex_print(sc[0],0,"MERGED");
  //  for(i=0;i<g0->nel;i++){
  //    
  //  }
  free(p);
  free(isbndf);
  free(codef);
  icsr_free(bfs0);
  ivec_free(etree);
  ivec_free(bcodesf);
  ivec_free(isbface);
  for(i=0;i<g0->nel;i++){
    free(nd[i]);
    free(elneib[i]);
    free(el2fnum[i]);
    free(iindex[i]);
  }
  free(nd);
  free(elneib);
  free(el2fnum);
  free(iindex);
  return sc[0];  
}
/****************************************************************/
INT main(INT argc, char **argv)
{
  //  INT i=-1;
  input_grid *g=parse_input_grid("grid.input");
  INT dim=g->dim;
  cube2simp *c2s=cube2simplex(dim);
  scomplex *sc=macro_split(g,c2s);
  if(dim==2||dim==3) {
    fprintf(stdout,"Writing vtk file...\n");
    vtkw("newmesh.vtu",sc,0,0,1.);
  }
  /*FREE*/
  haz_scomplex_free(sc);
  input_grid_free(g); 
  cube2simp_free(c2s);
  fprintf(stdout,"\nDone.\n");
  return 0;
}
/*********************EOF**********************************/

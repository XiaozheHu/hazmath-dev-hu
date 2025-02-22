/*! \file src/graphs/basic.c
 *
 *  Created by James Adler, Xiaozhe Hu, and Ludmil Zikatanov on 01/03/2020.
 *  Copyright 2020__HAZMATH__. All rights reserved.
 *
 *  \note basic subroutines for graphs;
 *
 */

#include "hazmath.h"

/***********************************************************************************************/
/*!
 * \fn dCSRmat get_adjacency_from_transition(dCSRmat *P, dvector *weighted_degree)
 *
 * \brief get adjacency matrix from transition matrix (A = D*P)
 *
 * \param P                pointer to the transition matrix
 * \param weighted_degree  pointer to the weighted degree
 *
 * \return A               pointer to the adjacency matrix
 *
 * \author Xiaozhe Hu
 * \date   01/02/2020
 */
dCSRmat get_adjacency_from_transition(dCSRmat *P,
                                      dvector *weighted_degree)
{
  // local variables
  dCSRmat A;

  // copy transition matrix
  dcsr_alloc(P->row, P->col, P->nnz, &A);
  dcsr_cp(P, &A);

  // A = D*P
  dcsr_row_scale(&A, weighted_degree);

  // return adjacency matrix
  return A;
}

/***********************************************************************************************/
/*!
 * \fn dCSRmat get_graphLaplacian_from_adj_wdeg(dCSRmat *A, dvector *weighted_degree)
 *
 * \brief get graph Laplacian from adjacency matrix and weighted degree matrix (L = D-A)
 *
 * \param A                pointer to the adjacency matrix
 * \param weighted_degree  pointer to the weighted degree
 *
 * \return L               pointer to the graph Laplacian
 *
 * \author Xiaozhe Hu
 * \date   01/03/2020
 */
dCSRmat get_graphLaplacian_from_adjacency(dCSRmat *A,
                                          dvector *weighted_degree)
{
  // local variables
  dCSRmat L;

  // form diagonal weighted degree matrix
   dCSRmat D = dcsr_create_diagonal_matrix(weighted_degree);

  // L =  D-A
  dcsr_add(&D, 1.0, A, -1.0, &L);

  // clean
  dcsr_free(&D);

  // return adjacency matrix
  return L;
}

/***********************************************************************************************/
/*!
 * \fn dCSRmat get_normalizedgraphLaplacian_from_L_wdeg_inv(dCSRmat *L, dvector *weighted_degree_half_inv)
 *
 * \brief get normalized graph Laplacian from graph Laplacian and inverse half weighted degree matrix (N = D^-1/2*L*D^-1/2)
 *
 * \param L                pointer to the graph Laplacian
 * \param weighted_degree_half_inv  pointer to the inverse half weighted degree
 *
 * \return N               pointer to the normalized graph Laplacian
 *
 * \author Xiaozhe Hu
 * \date   01/03/2020
 */
dCSRmat get_normalizedgraphLaplacian_from_L_wdeg_inv(dCSRmat *L,
                                                     dvector *weighted_degree_half_inv)
{
  // local variables
  dCSRmat N;

  // form diagonal weighted degree matrix
  dCSRmat D_half_inv = dcsr_create_diagonal_matrix(weighted_degree_half_inv);

  // N = D^-1/2*L*D^-1/2
  dcsr_rap(&D_half_inv, L, &D_half_inv, &N);

  // clean
  dcsr_free(&D_half_inv);

  // return adjacency matrix
  return N;
}

/***********************************************************************************************/
/*!
 * \fn dvector pairwise_distance(dDENSEmat *X, const INT norm_type)
 *
 * \brief compute pairwise distance of the nodes
 *
 * \param X             pointer to the coordinate matrix of the nodes (each row corresponds to one node)
 * \param norm_type     use what type of norm to compute the distance
 *
 * \return distance     pointer to the distance vector with ordering: (0,1),(0,2),... (0,n-1),(1,2),...(1,n),...(n-2,n-1)
 *
 * \author Xiaozhe Hu
 * \date   01/05/2020
 */
dvector pairwise_distance(dDENSEmat *X,
                           const INT norm_type)
{

  // local variable
  INT i,j,count;
  const INT n = X->row; // number of nodes
  const INT d = X->col; // dimension

  dvector Xi, Xj;
  Xi.row = d; Xj.row = d;

  dvector diff = dvec_create(d);

  // allocate memory
  const INT n_dis = (n*(n-1))/2;
  dvector distance = dvec_create(n_dis);

  // compute the distance
  count = 0;
  for (i=0;i<(n-1);i++){
    for (j=i+1;j<n;j++){

      // get coordinate of node i and node j
      Xi.val = &(X->val[i*d]);
      Xj.val = &(X->val[j*d]);

      // diff = Xi - Xj
      dvec_axpyz(-1.0, &Xj, &Xi, &diff);

      // compute diffusion distance between node i and node j
      switch (norm_type) {
        case TWONORM:
          distance.val[count] = dvec_norm2(&diff);
          break;

        default:
          distance.val[count] = dvec_norm1(&diff);
      }

      // update count
      count = count + 1;

    }
  }

  // clean and return
  dvec_free(&diff);

  return distance;

}

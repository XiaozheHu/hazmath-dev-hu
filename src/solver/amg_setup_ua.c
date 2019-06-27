/*! \file src/solver/amg_setup_ua.c
 *
 *  Unsmoothed Aggregation AMG: SETUP phase
 *
 *  Created by James Adler, Xiaozhe Hu, and Ludmil Zikatanov on 12/24/15.
 *  Copyright 2015__HAZMATH__. All rights reserved.
 *
 *  \note   Done cleanup for releasing -- Xiaozhe Hu 03/11/2017
 *
 *  \todo   Add safe guard for the overall computatinal complexity -- Xiaozhe Hu
 * \todo    Add maximal weighted matching coarsning -- Xiaozhe Hu
 * \todo    Add maximal independent set aggregation -- Xiaozhe Hu
 *
 */

#include "hazmath.h"

/*---------------------------------*/
/*--      Private Functions      --*/
/*---------------------------------*/
/***********************************************************************************************/
/**
 * \fn static void form_tentative_p (ivector *vertices, dCSRmat *tentp,
 *                                   REAL **basis, INT levelNum, INT num_aggregations)
 *
 * \brief Form aggregation based on strong coupled neighbors
 *
 * \param vertices           Pointer to the aggregation of vertices
 * \param tentp              Pointer to the prolongation operators
 * \param basis              Pointer to the near kernel
 * \param levelNum           Level number
 * \param num_aggregations   Number of aggregations
 *
 * \author Xiaozhe Hu
 * \date   09/29/2009
 *
 * \note Modified by Xiaozhe Hu on 05/25/2014
 *
 * \note this subroutine uses given near-null basis to form tentative prolongation
 *       and the basis is not necessary constant
 *
 */
static void form_tentative_p(ivector *vertices,
                             dCSRmat *tentp,
                             REAL **basis,
                             INT levelNum,
                             INT num_aggregations)
{
    INT i, j;

    /* Form tentative prolongation */
    tentp->row = vertices->row;
    tentp->col = num_aggregations;
    tentp->nnz = vertices->row;

    tentp->IA  = (INT *)calloc(tentp->row+1,sizeof(INT));

    // local variables
    INT  *IA = tentp->IA;
    INT  *vval = vertices->val;
    const INT row = tentp->row;

    // first run
    for ( i = 0, j = 0; i < row; i++ ) {
        IA[i] = j;
        if (vval[i] > UNPT) j++;
    }
    IA[row] = j;

    // allocate memory for P
    tentp->nnz = j;
    tentp->JA  = (INT *)calloc(tentp->nnz, sizeof(INT));
    tentp->val = (REAL *)calloc(tentp->nnz, sizeof(REAL));

    INT  *JA = tentp->JA;
    REAL *val = tentp->val;

    // second run
    for (i = 0, j = 0; i < row; i ++) {
        IA[i] = j;
        if (vval[i] > UNPT) {
            JA[j] = vval[i];
            val[j] = basis[0][i];
            j ++;
        }
    }
}

/***********************************************************************************************/
/**
 * \fn static void form_boolean_p (ivector *vertices, dCSRmat *tentp, INT levelNum,
 *                                 INT num_aggregations)
 *
 * \brief Form aggregation based on strong coupled neighbors
 *
 * \param vertices           Pointer to the aggregation of vertices
 * \param tentp              Pointer to the prolongation operators
 * \param levelNum           Level number
 * \param num_aggregations   Number of aggregations
 *
 * \author Xiaozhe Hu
 * \date   09/29/2009
 *
 * Modified by Xiaozhe Hu on 05/25/2014
 */
void form_boolean_p(ivector *vertices,
                           dCSRmat *tentp,
                           INT levelNum,
                           INT num_aggregations)
{
    INT i, j;

    /* Form tentative prolongation */
    tentp->row = vertices->row;
    tentp->col = num_aggregations;
    tentp->nnz = vertices->row;

    tentp->IA  = (INT *)calloc(tentp->row+1,sizeof(INT));

    // local variables
    INT  *IA = tentp->IA;
    INT  *vval = vertices->val;
    const INT row = tentp->row;

    // first run
    for ( i = 0, j = 0; i < row; i++ ) {
        IA[i] = j;
        if (vval[i] > UNPT) j++;
    }
    IA[row] = j;

    // allocate memory for P
    tentp->nnz = j;
    tentp->JA  = (INT *)calloc(tentp->nnz, sizeof(INT));
    tentp->val = (REAL *)calloc(tentp->nnz, sizeof(REAL));

    INT  *JA = tentp->JA;
    REAL *val = tentp->val;

    // second run
    for (i = 0, j = 0; i < row; i ++) {
        IA[i] = j;
        if (vval[i] > UNPT) {
            JA[j] = vval[i];
            val[j] = 1.0;
            j ++;
        }
    }
}

/***********************************************************************************************/
static void construct_strong_couped(dCSRmat *A,
                                    AMG_param *param,
                                    dCSRmat *Neigh)
{

    // local variables
    const INT  row = A->row, col = A->col, nnz = A->IA[row]-A->IA[0];
    const INT  *AIA = A->IA, *AJA = A->JA;
    const REAL *Aval = A->val;

    INT  i, j, index, row_start, row_end;
    REAL strongly_coupled = param->strong_coupled;
    REAL strongly_coupled2 = pow(strongly_coupled,2);

    INT  *NIA, *NJA;
    REAL *Nval;

    // get the diagonal entries
    dvector diag;
    dcsr_getdiag(0, A, &diag);

    // allocate Neigh
    dcsr_alloc(row, col, nnz, Neigh);

    NIA  = Neigh->IA; NJA  = Neigh->JA;
    Nval = Neigh->val;

    // set IA for Neigh
    for ( i = row; i >= 0; i-- ) NIA[i] = AIA[i];

    // main loop of finding strongly coupled neighbors
    for ( index = i = 0; i < row; ++i ) {
        NIA[i] = index;
        row_start = AIA[i]; row_end = AIA[i+1];
        for ( j = row_start; j < row_end; ++j ) {

            if ( (AJA[j] == i)
              || ( (pow(Aval[j],2) >= strongly_coupled2*ABS(diag.val[i]*diag.val[AJA[j]])) && (Aval[j] < 0) )
               )
            {
                NJA[index] = AJA[j];
                Nval[index] = Aval[j];
                index++;
            }

        } // end for ( j = row_start; j < row_end; ++j )
    } // end for ( index = i = 0; i < row; ++i )
    NIA[row] = index;

    Neigh->nnz = index;
    Neigh->JA  = (INT*) realloc(Neigh->JA,  (Neigh->IA[row])*sizeof(INT));
    Neigh->val = (REAL*)realloc(Neigh->val, (Neigh->IA[row])*sizeof(REAL));

    dvec_free(&diag);

}

/***********************************************************************************************/
/**
 * \fn static SHORT aggregation_vmb (dCSRmat *A, ivector *vertices, AMG_param *param,
 *                                   dCSRmat *Neigh, INT *num_aggregations, INT lvl)
 *
 * \brief Form aggregation based on strong coupled neighbors
 *
 * \param A                 Pointer to the coefficient matrices
 * \param vertices          Pointer to the aggregation of vertices
 * \param param             Pointer to AMG parameters
 * \param Neigh             Pointer to strongly coupled neighbors
 * \param num_aggregations  Pointer to number of aggregations
 * \param lvl               Level number
 *
 * \author Xiaozhe Hu
 * \date   09/29/2009
 *
 * \note Setup A, P, PT and levels using the unsmoothed aggregation algorithm;
 *       Refer to P. Vanek, J. Madel and M. Brezina
 *       "Algebraic Multigrid on Unstructured Meshes", 1994
 *
 */
static SHORT aggregation_vmb(dCSRmat *A,
                             ivector *vertices,
                             AMG_param *param,
                             dCSRmat *Neigh,
                             INT *num_aggregations, INT lvl)
{
    // local variables
    const INT    row = A->row;
    const INT    max_aggregation = param->max_aggregation;

    // return status
    SHORT  status = SUCCESS;

    // local variables
    INT    num_left = row;
    INT    subset, count;
    INT    *num_each_agg;

    INT    i, j, row_start, row_end;
    INT    *NIA = NULL, *NJA = NULL;

    // find strongly coupled neighbors
    construct_strong_couped(A, param, Neigh);

    NIA  = Neigh->IA; NJA  = Neigh->JA;

    /*------------------------------------------*/
    /*             Initialization               */
    /*------------------------------------------*/
    ivec_alloc(row, vertices);
    iarray_set(row, vertices->val, -2);
    *num_aggregations = 0;

    /*-------------*/
    /*   Step 1.   */
    /*-------------*/
    //    for ( i = 0; i < row; ++i ) {
    for ( i=row-1; i>=0;i-- ) {
        if ( (NIA[i+1] - NIA[i]) == 1 ) {
            vertices->val[i] = UNPT;
            num_left--;
        }
        else {
            subset = TRUE;
            row_start = NIA[i]; row_end = NIA[i+1];
            for ( j = row_start; j < row_end; ++j ) {
                if ( vertices->val[NJA[j]] >= UNPT ) {
                    subset = FALSE;
                    break;
                }
            }
            if ( subset ) {
                count = 0;
                vertices->val[i] = *num_aggregations;
                num_left--;
                count++;
                row_start = NIA[i]; row_end = NIA[i+1];
                for ( j = row_start; j < row_end; ++j ) {
                    if ( (NJA[j]!=i) && (count < max_aggregation) ) {
                        vertices->val[NJA[j]] = *num_aggregations;
                        num_left--;
                        count ++;
                    }
                }
                (*num_aggregations)++;
            }
        }
    }

    /*-------------*/
    /*   Step 2.   */
    /*-------------*/
    INT *temp_C = (INT*)calloc(row,sizeof(INT));

    if ( *num_aggregations < MIN_CDOF ) {
        status = ERROR_AMG_COARSEING; goto END;
    }

    num_each_agg = (INT*)calloc(*num_aggregations,sizeof(INT));

    //for ( i = 0; i < *num_aggregations; i++ ) num_each_agg[i] = 0; // initialize

    for ( i = row; i--; ) {
        temp_C[i] = vertices->val[i];
        if ( vertices->val[i] >= 0 ) num_each_agg[vertices->val[i]] ++;
    }

    //    for ( i = 0; i < row; ++i ) {
    for ( i = row-1; i >= 0; i-- ) {
        if ( vertices->val[i] < UNPT ) {
            row_start = NIA[i]; row_end = NIA[i+1];
            for ( j = row_start; j < row_end; ++j ) {
                if ( temp_C[NJA[j]] > UNPT
                    && num_each_agg[temp_C[NJA[j]]] < max_aggregation ) {
                    vertices->val[i] = temp_C[NJA[j]];
                    num_left--;
                    num_each_agg[temp_C[NJA[j]]] ++ ;
                    break;
                }
            }
        }
    }
    /*-------------*/
    /*   Step 3.   */
    /*-------------*/
    while ( num_left > 0 ) {
        for ( i = 0; i < row; ++i ) {
            if ( vertices->val[i] < UNPT ) {
                count = 0;
                vertices->val[i] = *num_aggregations;
                num_left--;
                count++;
                row_start = NIA[i]; row_end = NIA[i+1];
                for ( j = row_start; j < row_end; ++j ) {
                    if ( (NJA[j]!=i) && (vertices->val[NJA[j]] < UNPT)
						             && (count<max_aggregation) ) {
                        vertices->val[NJA[j]] = *num_aggregations;
                        num_left--;
                        count++;
                    }
                }
                (*num_aggregations)++;
            }
        }
    }

    free(num_each_agg);

END:
    free(temp_C);

    return status;
}

/***********************************************************************************************/
/**
 * \fn static SHORT aggregation_hec (dCSRmat *A, ivector *vertices, AMG_param *param,
 *                                   dCSRmat *Neigh, INT *num_aggregations,INT lvl)
 *
 * \brief Heavy edge coarsening aggregation based on strong coupled neighbors
 *
 * \param A                 Pointer to the coefficient matrices
 * \param vertices          Pointer to the aggregation of vertices
 * \param param             Pointer to AMG parameters
 * \param Neigh             Pointer to strongly coupled neighbors
 * \param num_aggregations  Pointer to number of aggregations
 * \param lvl               Level number
 *
 * \author Xiaozhe Hu
 * \date   03/12/2017
 *
 * \todo Add control of maximal size of each aggregates
 *
 * \note Refer to J. Urschel, X. Hu, J. Xu and L. Zikatanov
 *       "A Cascadic Multigrid Algorithm for Computing the Fiedler Vector of Graph Laplacians", 2015
 *
 */
static SHORT aggregation_hec(dCSRmat *A,
                             ivector *vertices,
                             AMG_param *param,
                             dCSRmat *Neigh,
                             INT *num_aggregations, INT lvl)
{
    // local variables
    const INT    row = A->row;
    //const INT    max_aggregation = param->max_aggregation;

    // return status
    SHORT  status = SUCCESS;

    INT  i, j, k, ii, jj, row_start, row_end;;
    INT  *NIA = NULL, *NJA = NULL;
    REAL *Nval = NULL;
    REAL maxval = 0.0;

    INT *perm =(INT *)calloc(row, sizeof(INT));

    // find strongly coupled neighbors
    construct_strong_couped(A, param, Neigh);

    NIA  = Neigh->IA; NJA  = Neigh->JA;
    Nval = Neigh->val;

    /*------------------------------------------*/
    /*             Initialization               */
    /*------------------------------------------*/
    ivec_alloc(row, vertices);
    iarray_set(row, vertices->val, -2);
    *num_aggregations = 0;

    // get random permutation
    for(i=0; i<row; i++) perm[i] = i;
    iarray_shuffle(row, perm);
    /*
    //    fprintf(stdout,"\nlvl=%d",lvl);
    if(!lvl){
      for(i=0; i<row; i++) perm[i] = row-i-1;
    } else {
      for(i=0; i<row; i++) perm[i] = i;
    }
    */
    // main loop
    for ( ii = 0; ii < row; ii++ ) {
        i = perm[ii];
        if ( (NIA[i+1] - NIA[i]) == 1 ) {
            vertices->val[i] = UNPT;
        }
        else {
            // find the most strongly connected neighbor
            row_start = NIA[i]; row_end = NIA[i+1];
            maxval = 0.0;
            for (jj = row_start; jj < row_end; jj++)
            {
                if (NJA[jj] != i) {
                    if ( ABS(Nval[jj]) > maxval ) {
                        k = jj;
                        maxval = ABS(Nval[jj]);
                    }
                }
            } // end for (jj = row_start+1; jj < row_end; jj++)
            j = NJA[k];

            // create a new aggregates if the most strongly conncected neighbor
            // is still avaliable
            if (vertices->val[j] < UNPT)
            {
                vertices->val[j] = *num_aggregations;
                vertices->val[i] = *num_aggregations;
                (*num_aggregations)++;
            }
            else
            {
                vertices->val[i] = vertices->val[j];
            } // end if (vertices->val[j] < UNPT)
        } // end if ( (NIA[i+1] - NIA[i]) == 1 )

    } // end for ( ii = 0; ii < row; ii++ )

    // free spaces
    free(perm);

    return status;

}

/*---------------------------------*/
/*--        End of File          --*/
/*---------------------------------*/


static SHORT amg_setup_unsmoothP_unsmoothR(AMG_data *, AMG_param *);

/*---------------------------------*/
/*--      Public Functions       --*/
/*---------------------------------*/

/***********************************************************************************************/
/**
 * \fn SHORT amg_setup_ua (AMG_data *mgl, AMG_param *param)
 *
 * \brief Set up phase of unsmoothed aggregation AMG
 *
 * \param mgl    Pointer to AMG data: AMG_data
 * \param param  Pointer to AMG parameters: AMG_param
 *
 * \return       SUCCESS if successed; otherwise, error information.
 *
 * \author Xiaozhe Hu
 * \date   12/28/2011
 */
SHORT amg_setup_ua (AMG_data *mgl,
                    AMG_param *param)
{

    SHORT status = amg_setup_unsmoothP_unsmoothR(mgl, param);

    return status;
}

/*---------------------------------*/
/*--      Private Functions      --*/
/*---------------------------------*/

/***********************************************************************************************/
/**
 * \fn static SHORT amg_setup_unsmoothP_unsmoothR (AMG_data *mgl, AMG_param *param)
 *
 * \brief Setup phase of plain aggregation AMG, using unsmoothed P and unsmoothed A
 *
 * \param mgl    Pointer to AMG_data
 * \param param  Pointer to AMG_param
 *
 * \return       SUCCESS if succeed, error otherwise
 *
 * \author Xiaozhe Hu
 * \date   02/21/2011
 *
 */
static SHORT amg_setup_unsmoothP_unsmoothR(AMG_data *mgl,
                                           AMG_param *param)
{
    // local variables
    const SHORT prtlvl     = param->print_level;
    const SHORT cycle_type = param->cycle_type;
    const SHORT csolver    = param->coarse_solver;
    const SHORT min_cdof   = MAX(param->coarse_dof,50);
    const INT   m          = mgl[0].A.row;

    // local variables
    SHORT         max_levels = param->max_levels, lvl = 0, status = SUCCESS;
    INT           i;
    REAL          setup_start, setup_end;
    Schwarz_param swzparam;

    get_time(&setup_start);

    // level info (fine: 0; coarse: 1)
    ivector *vertices = (ivector *)calloc(max_levels,sizeof(ivector));

    // each level stores the information of the number of aggregations
    INT *num_aggs = (INT *)calloc(max_levels,sizeof(INT));

    // each level stores the information of the strongly coupled neighborhoods
    dCSRmat *Neighbor = (dCSRmat *)calloc(max_levels,sizeof(dCSRmat));

    // Initialize level information
    for ( i = 0; i < max_levels; ++i ) num_aggs[i] = 0;

    mgl[0].near_kernel_dim   = 1;
    mgl[0].near_kernel_basis = (REAL **)calloc(mgl->near_kernel_dim,sizeof(REAL*));

    for ( i = 0; i < mgl->near_kernel_dim; ++i ) {
        mgl[0].near_kernel_basis[i] = (REAL *)calloc(m,sizeof(REAL));
        array_set(m, mgl[0].near_kernel_basis[i], 1.0);
    }

    // Initialize Schwarz parameters
    mgl->Schwarz_levels = param->Schwarz_levels;
    if ( param->Schwarz_levels > 0 ) {
        swzparam.Schwarz_mmsize = param->Schwarz_mmsize;
        swzparam.Schwarz_maxlvl = param->Schwarz_maxlvl;
        swzparam.Schwarz_type   = param->Schwarz_type;
        swzparam.Schwarz_blksolver = param->Schwarz_blksolver;
    }

    // Initialize AMLI coefficients
    if ( cycle_type == AMLI_CYCLE ) {
        const INT amlideg = param->amli_degree;
        param->amli_coef = (REAL *)calloc(amlideg+1,sizeof(REAL));
        REAL lambda_max = 2.0, lambda_min = lambda_max/4;
        amg_amli_coef(lambda_max, lambda_min, amlideg, param->amli_coef);
    }

#if DIAGONAL_PREF
    dcsr_diagpref(&mgl[0].A); // reorder each row to make diagonal appear first
#endif

    /*----------------------------*/
    /*--- checking aggregation ---*/
    /*----------------------------*/
    // Main AMG setup loop
    while ( (mgl[lvl].A.row > min_cdof) && (lvl < max_levels-1) ) {

      /*-- Setup Schwarz smoother if necessary */
      if ( lvl < param->Schwarz_levels ) {
          mgl[lvl].Schwarz.A=dcsr_sympat(&mgl[lvl].A);
          dcsr_shift(&(mgl[lvl].Schwarz.A), 1);
          Schwarz_setup(&mgl[lvl].Schwarz, &swzparam);
      }

        /*-- Aggregation --*/
        switch ( param->aggregation_type ) {

            case VMB: // VMB aggregation
                status = aggregation_vmb(&mgl[lvl].A, &vertices[lvl], param,
                                         &Neighbor[lvl], &num_aggs[lvl],lvl);
                break;

            case HEC: // Heavy edge coarsening aggregation
                status = aggregation_hec(&mgl[lvl].A, &vertices[lvl], param,
                                         &Neighbor[lvl], &num_aggs[lvl],lvl);
                break;

            default: // wrong aggregation type
                status = ERROR_AMG_AGG_TYPE;
                check_error(status, __FUNCTION__);
                break;
        }

        /*-- Choose strength threshold adaptively --*/
        if ( num_aggs[lvl]*4 > mgl[lvl].A.row )
            param->strong_coupled /= 2;
        else if ( num_aggs[lvl]*1.25 < mgl[lvl].A.row )
            param->strong_coupled *= 2;

        // Check 1: Did coarsening step succeed?
        if ( status < 0 ) {
            // When error happens, stop at the current multigrid level!
            if ( prtlvl > PRINT_MIN ) {
                printf("### HAZMATH WARNING: Forming aggregates on level-%d failed!\n", lvl);
            }
            status = SUCCESS; break;
        }

        /*-- Form Prolongation --*/
        form_tentative_p(&vertices[lvl], &mgl[lvl].P, mgl[0].near_kernel_basis,
                         lvl+1, num_aggs[lvl]);

        // Check 2: Is coarse sparse too small?
        if ( mgl[lvl].P.col < MIN_CDOF ) break;

#if 0
        // Check 3: Does this coarsening step too aggressive?
        if ( mgl[lvl].P.row > mgl[lvl].P.col * MAX_CRATE ) {
            if ( prtlvl > PRINT_MIN ) {
                printf("### HAZMATH WARNING: Coarsening might be too aggressive!\n");
                printf("### HAZMATH: Fine level = %d, coarse level = %d. Discard!\n",
                       mgl[lvl].P.row, mgl[lvl].P.col);
            }
            break;
        }
#endif

#if 0
        // Check 4: Is this coarsening ratio too small?
        if ( (REAL)mgl[lvl].P.col > mgl[lvl].P.row * MIN_CRATE ) {
            if ( prtlvl > PRINT_MIN ) {
                printf("### WARNING: Coarsening rate is too small!\n");
                printf("### WARNING: Fine level = %d, coarse level = %d. Discard!\n",
                       mgl[lvl].P.row, mgl[lvl].P.col);
            }
            break;
        }
#endif

        /*-- Form restriction --*/
        dcsr_trans(&mgl[lvl].P, &mgl[lvl].R);

        /*-- Form coarse level stiffness matrix --*/
        dcsr_rap_agg(&mgl[lvl].R, &mgl[lvl].A, &mgl[lvl].P,
                               &mgl[lvl+1].A);

        dcsr_free(&Neighbor[lvl]);
        ivec_free(&vertices[lvl]);

        ++lvl;

    }

#if DIAGONAL_PREF
    dcsr_diagpref(&mgl[lvl].A); // reorder each row to make diagonal appear first
#endif

    // Setup coarse level systems for direct solvers
    switch (csolver) {

#if WITH_SUITESPARSE
        case SOLVER_UMFPACK: {
            // Need to sort the matrix A for UMFPACK to work
            dCSRmat Ac_tran;
            dcsr_trans(&mgl[lvl].A, &Ac_tran);
            // It is equivalent to do transpose and then sort
            //     fasp_dcsr_trans(&mgl[lvl].A, &Ac_tran);
            //     fasp_dcsr_sort(&Ac_tran);
            dcsr_cp(&Ac_tran, &mgl[lvl].A);
            dcsr_free(&Ac_tran);
            mgl[lvl].Numeric = umfpack_factorize(&mgl[lvl].A, 0);
            break;
        }
#endif
        default:
            // Do nothing!
            break;
    }

    // setup total level number and current level
    mgl[0].num_levels = max_levels = lvl+1;
    mgl[0].w          = dvec_create(m);

    for ( lvl = 1; lvl < max_levels; ++lvl) {
        INT mm = mgl[lvl].A.row;
        mgl[lvl].num_levels = max_levels;
        mgl[lvl].b          = dvec_create(mm);
        mgl[lvl].x          = dvec_create(mm);

        mgl[lvl].cycle_type     = cycle_type; // initialize cycle type!

        if ( cycle_type == NL_AMLI_CYCLE )
            mgl[lvl].w = dvec_create(3*mm);
        else
            mgl[lvl].w = dvec_create(2*mm);
    }

    if ( prtlvl > PRINT_NONE ) {
        get_time(&setup_end);
        print_amg_complexity(mgl,prtlvl);
        print_cputime("Unsmoothed aggregation setup", setup_end - setup_start);
    }

    free(Neighbor);
    free(vertices);
    free(num_aggs);

    return status;
}

/*---------------------------------*/
/*--        End of File          --*/
/*---------------------------------*/

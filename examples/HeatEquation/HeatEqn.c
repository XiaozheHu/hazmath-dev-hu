/*
 *  HeatEqn.c
 *
 *  Created by James Adler on 10/16/16.
 *  Copyright 2015_HAZMAT__. All rights reserved.
 *
 *  Discussion:
 *
 *    This program solves the following PDE using finite elements
 *
 *      du/dt - div(a(x)grad(u)) = 0
 *
 *    where du/dt is discretized with Crank-Nicolson or BDF-1 (Backward Euler)
 *
 *    in 2D or 3D
 *
 *   Along the boundary of the region, Dirichlet conditions are imposed:
 *
 *      u = 0 for P1, P2
 */

/*********** HAZMAT FUNCTIONS and INCLUDES ****************************************/
#include "hazmat.h"
/**********************************************************************************/

/******** Data Input **************************************************************/
// PDE Coefficients
void diffusion_coeff(REAL *val,REAL* x,REAL time) {
  *val = 1.0;
}

// True Solution (if you have one)
// Pick one of these and rename it truesol
//void truesol_2D(REAL *val,REAL* x,REAL time) {
void truesol(REAL *val,REAL* x,REAL time) {
  // 2D
  //*val = sin(M_PI*x[0])*sin(M_PI*x[1])*exp(-2.0*M_PI*M_PI*time);
  // 3D
  *val = sin(M_PI*x[0])*sin(M_PI*x[1])*sin(M_PI*x[2])*exp(-3*M_PI*M_PI*time);
}

// Right-hand Side
void myrhs(REAL *val,REAL* x,REAL time) {
  *val = 0.0;
}

// Boundary Conditions
void bc(REAL *val,REAL* x,REAL time) {
  *val= 0.0;
}

// Initial Conditions
void initial_conditions(REAL *val,REAL* x,REAL time) {
  // 2D
  //*val = sin(M_PI*x[0])*sin(M_PI*x[1]);
  // 3D
  *val = sin(M_PI*x[0])*sin(M_PI*x[1])*sin(M_PI*x[2]);
}

/**********************************************************************************/

/****** MAIN DRIVER ***************************************************************/
int main (int argc, char* argv[])
{
  
  printf("\n===========================================================================\n");
  printf("Beginning Program to solve the Heat Equation.\n");
  printf("===========================================================================\n");
  
  /****** INITIALIZE PARAMETERS **************************************************/

  // Counters
  INT j;

  // Overall Timing
  clock_t clk_overall_start = clock();

  // Set Parameters from Reading in Input File
  input_param inparam;
  param_input_init(&inparam);
  param_input("./input.dat", &inparam);

  // Open gridfile for reading
  printf("\nCreating mesh and FEM spaces:\n");
  FILE* gfid = HAZ_fopen(inparam.gridfile,"r");

  // Dimension is needed for all this to work
  INT dim = inparam.dim;

  // Create the mesh (now we assume triangles in 2D or tetrahedra in 3D)
  // File types possible are 0 - old format; 1 - vtk format
  clock_t clk_mesh_start = clock(); // Time mesh generation FE setup
  INT mesh_type = 0;
  trimesh mesh;
  printf(" --> loading grid from file: %s\n",inparam.gridfile);
  creategrid_fread(gfid,mesh_type,&mesh);
  fclose(gfid);

  // Get Quadrature Nodes for the Mesh
  INT nq1d = inparam.nquad; /* Quadrature points per dimension */
  qcoordinates *cq = get_quadrature(&mesh,nq1d);

  // Time stepping parameters
  timestepper time_stepper;
  initialize_timestepper(&time_stepper,&inparam);

  // Get info for and create FEM spaces
  // Order of Elements: 0 - P0; 1 - P1; 2 - P2; 20 - Nedelec; 30 - Raviart-Thomas
  INT order = inparam.FE_type;
  fespace FE;
  create_fespace(&FE,&mesh,order);
  char elmtype[8];
  sprintf(elmtype,"P%d",order);

  // Set Dirichlet Boundaries
  set_dirichlet_bdry(&FE,&mesh,1);

  // Dump some of the data
  if(inparam.print_level > 3) {
    char varu[1];
    char dir[20];
    sprintf(dir,"output");
    sprintf(varu,"u");
    dump_fespace(&FE,varu,dir);

    char* namevtk = "output/mesh.vtu";
    dump_mesh_vtk(namevtk,&mesh);
  }
    
  clock_t clk_mesh_end = clock(); // End of timing for mesh and FE setup
  printf(" --> elapsed CPU time for mesh and FEM space construction = %f seconds.\n\n",
         (REAL) (clk_mesh_end - clk_mesh_start)/CLOCKS_PER_SEC);
  /*******************************************************************************/
    
  printf("***********************************************************************************\n");
  printf("Number of Elements = %d\tElement Type = %s\tOrder of Quadrature = %d\n",mesh.nelm,elmtype,2*nq1d-1);
  printf("\n\t--- Degrees of Freedom ---\n");
  printf("Vertices: %-7d\tEdges: %-7d\tFaces: %-7d",mesh.nv,mesh.nedge,mesh.nface);
  printf("\t--> DOF: %d\n",FE.ndof);
  printf("\n\t--- Boundaries ---\n");
  printf("Vertices: %-7d\tEdges: %-7d\tFaces: %-7d",mesh.nbv,mesh.nbedge,mesh.nbface);
  printf("\t--> Boundary DOF: %d\n",FE.nbdof);
  printf("***********************************************************************************\n\n");
    
  /*** Assemble the matrix and right hand side *******************************/
  printf("Assembling the matrix and right-hand side:\n");
  clock_t clk_assembly_start = clock();
    
  // Allocate the right-hand side and declare the csr matrix
  dvector b;
  dCSRmat A;
  dCSRmat M;
    
  // Assemble the matrix without BC
  // Diffusion block
  assemble_global(&A,&b,assemble_DuDv_local,&FE,&mesh,cq,myrhs,diffusion_coeff,0.0);

  // Time-Derivative block
  assemble_global(&M,NULL,assemble_mass_local,&FE,&mesh,cq,NULL,one_coeff_scal,0.0);

  // Create Time Operator (one with BC and one without)
  time_stepper.A = &A;
  time_stepper.M = &M;
  get_timeoperator(&time_stepper);

  clock_t clk_assembly_end = clock();
  printf(" --> elapsed CPU time for assembly = %f seconds.\n\n",(REAL)
         (clk_assembly_end-clk_assembly_start)/CLOCKS_PER_SEC);
  /*******************************************************************************/

  /**************** Solve ********************************************************/

  // Create Solution Vector
  dvector sol = dvec_create(FE.ndof);
  dvector true_sol = dvec_create(FE.ndof);
  FE_Evaluate(true_sol.val,truesol,&FE,&mesh,0.0);


  // Set parameters for linear iterative methods
  linear_itsolver_param linear_itparam;
  param_linear_solver_init(&linear_itparam);
  param_solver_set(&linear_itparam, &inparam);
  INT solver_flag=-20;
  // Set parameters for algebriac multigrid methods
  AMG_param amgparam;
  param_amg_init(&amgparam);
  param_amg_set(&amgparam, &inparam);
  param_amg_print(&amgparam);

  // Set parameters for ILU methods
  ILU_param iluparam;
  param_ilu_init(&iluparam);
  param_ilu_set(&iluparam, &inparam);
  param_ilu_print(&iluparam);

  // Get Initial Conditions
  FE_Evaluate(sol.val,initial_conditions,&FE,&mesh,0.0);
  time_stepper.sol = &sol;
  char solout[40];
  char trueout[40];
  if (inparam.output_type==2) {
    sprintf(solout,"output/solution_ts000.vtu");
    dump_sol_onV_vtk(solout,&mesh,time_stepper.sol->val,1);
  }

  // Store current RHS
  time_stepper.rhs = &b;

  // Begin Timestepping Loop
  printf("Performing %d Time Steps with step size dt = %1.3f\n",time_stepper.tsteps,time_stepper.dt);
  printf("--------------------------------------------------------------\n\n");
  printf("============================\n");
  printf("Time Step %d: Time = %1.8f\n",time_stepper.current_step,time_stepper.time);
  printf("============================\n");
  REAL* uerr = (REAL *) calloc(time_stepper.tsteps+1,sizeof(REAL));
  uerr[0] = L2error(time_stepper.sol->val,truesol,&FE,&mesh,cq,time_stepper.time);
  REAL* unorm = (REAL *) calloc(time_stepper.tsteps+1,sizeof(REAL));
  unorm[0] = L2norm(time_stepper.sol->val,&FE,&mesh,cq);
  REAL* utnorm = (REAL *) calloc(time_stepper.tsteps+1,sizeof(REAL));
  utnorm[0] = L2norm(true_sol.val,&FE,&mesh,cq);
  printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
  printf("L2 Norm of u            = %26.13e\n",unorm[0]);
  printf("L2 Norm of true u       = %26.13e\n",utnorm[0]);
  printf("L2 Norm of u error      = %26.13e\n",uerr[0]);
  printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n\n");

  clock_t clk_timeloop_start = clock();

  for(j=0;j<time_stepper.tsteps;j++) {
    clock_t clk_timestep_start = clock();

    // Update Time Step Data (includes time, counters, solution, and rhs)
    update_timestep(&time_stepper);

    printf("============================\n");
    printf("Time Step %d: Time = %1.3f\n",time_stepper.current_step,time_stepper.time);
    printf("============================\n");

    // Recompute RHS if it's time-dependent
    if(time_stepper.rhs_timedep) {
      assemble_global_RHS(time_stepper.rhs,&FE,&mesh,cq,myrhs,time_stepper.time);
    }

    // Update RHS
    update_time_rhs(&time_stepper);

    // For first time step eliminate boundary conditions in matrix and rhs
    if(j==0) {
      eliminate_DirichletBC(bc,&FE,&mesh,time_stepper.rhs_time,time_stepper.At,time_stepper.time);
    } else {
      eliminate_DirichletBC_RHS(bc,&FE,&mesh,time_stepper.rhs_time,time_stepper.At_noBC,time_stepper.time);
    }

    // Solve
    clock_t clk_solve_start = clock();
    dcsr_shift(time_stepper.At, -1);  // shift A
    if(linear_itparam.linear_itsolver_type == 0) { // Direct Solver
      printf(" --> using UMFPACK's Direct Solver:\n");
      //solver_flag = directsolve_UMF_symmetric(&A,&b,u.val,linear_itparam.linear_print_level);
    } else { // Iterative Solver
      // Use AMG as iterative solver
      if (linear_itparam.linear_itsolver_type == SOLVER_AMG){
        solver_flag = linear_solver_amg(time_stepper.At,time_stepper.rhs_time,time_stepper.sol, &amgparam);
      } else { // Use Krylov Iterative Solver
        // Determine Preconditioner
        switch (linear_itparam.linear_precond_type) {
        case PREC_DIAG:  // Diagonal Preconditioner
          solver_flag = linear_solver_dcsr_krylov_diag(time_stepper.At,time_stepper.rhs_time,time_stepper.sol,&linear_itparam);
          break;
        case PREC_AMG:  // AMG preconditioner
          solver_flag = linear_solver_dcsr_krylov_amg(time_stepper.At,time_stepper.rhs_time,time_stepper.sol, &linear_itparam, &amgparam);
          break;
        case PREC_ILU:  // ILU preconditioner
          solver_flag = linear_solver_dcsr_krylov_ilu(time_stepper.At,time_stepper.rhs_time,time_stepper.sol, &linear_itparam, &iluparam);
          break;
        default:  // No Preconditioner
          solver_flag = linear_solver_dcsr_krylov(time_stepper.At,time_stepper.rhs_time,time_stepper.sol,&linear_itparam);
          break;
        }
      }
      dcsr_shift(time_stepper.At, 1);   // shift A back
    }

    // Error Check
    if (solver_flag < 0) printf("### ERROR: Solver does not converge with error code = %d!\n", solver_flag);

    clock_t clk_solve_end = clock();
    printf("Elapsed CPU Time for Solve = %f seconds.\n\n",(REAL) (clk_solve_end-clk_solve_start)/CLOCKS_PER_SEC);

    clock_t clk_timestep_end = clock();
    printf("Elapsed CPU Time for Time Step = %f seconds.\n\n",(REAL) (clk_timestep_end-clk_timestep_start)/CLOCKS_PER_SEC);

    /**************** Compute Errors if you have true solution ********************/
    clock_t clk_error_start = clock();

    uerr[j+1] = L2error(time_stepper.sol->val,truesol,&FE,&mesh,cq,time_stepper.time);
    unorm[j+1] = L2norm(time_stepper.sol->val,&FE,&mesh,cq);
    FE_Evaluate(true_sol.val,truesol,&FE,&mesh,time_stepper.time);
    utnorm[j+1] = L2norm(true_sol.val,&FE,&mesh,cq);
    clock_t clk_error_end = clock();
    printf("Elapsed CPU time for getting errors = %lf seconds.\n\n",(REAL)
           (clk_error_end-clk_error_start)/CLOCKS_PER_SEC);
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    printf("L2 Norm of u            = %26.13e\n",unorm[j+1]);
    printf("L2 Norm of true u       = %26.13e\n",utnorm[j+1]);
    printf("L2 Norm of u error      = %26.13e\n",uerr[j+1]);
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    /******************************************************************************/

    if (inparam.output_type==2) {
      sprintf(solout,"output/solution_ts%03d.vtu",time_stepper.current_step);
      dump_sol_onV_vtk(solout,&mesh,time_stepper.sol->val,1);
      sprintf(trueout,"output/true_solution_ts%03d.vtu",time_stepper.current_step);
      dump_sol_onV_vtk(trueout,&mesh,true_sol.val,1);
    }
    printf("\n");
  } // End Timestepping Loop
  printf("----------------------- Timestepping Complete ---------------------------------------\n\n");
  clock_t clk_timeloop_end = clock();
  printf("Elapsed CPU Time ALL Time Steps = %f seconds.\n\n",(REAL) (clk_timeloop_end-clk_timeloop_start)/CLOCKS_PER_SEC);
  /******************************************************************************/

  /******** Summary Print *******************************************************/
  printf("Summary of Timestepping\n");
  printf("Time Step\tTime\t\t\t||u||\t\t\t\t||u_true||\t\t\t||error||\n\n");
  for(j=0;j<=time_stepper.tsteps;j++) {
    printf("%02d\t\t%f\t%25.16e\t%25.16e\t%25.16e\n",j,j*time_stepper.dt,unorm[j],utnorm[j],uerr[j]);
  }

  /******** Free All the Arrays *************************************************/
  if(unorm) free(unorm);
  if(utnorm) free(utnorm);
  if(uerr) free(uerr);
  dvec_free(&true_sol);
  free_timestepper(&time_stepper);
  free_fespace(&FE);
  if(cq) {
    free_qcoords(cq);
    free(cq);
    cq = NULL;
  }
  free_mesh(&mesh);
  /******************************************************************************/
    
  clock_t clk_overall_end = clock();
  printf("\nEnd of Program: Total CPU Time = %f seconds.\n\n",(REAL) (clk_overall_end-clk_overall_start)/CLOCKS_PER_SEC);
  return 0;

}	/* End of Program */
/*******************************************************************************************/



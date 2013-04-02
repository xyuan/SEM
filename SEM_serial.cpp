// -*- coding: utf-8 -*-
/* FEM implementation using spectral elements */
/* Lagrange polynomials are used as shape functions.

   Since the integration points are gauss Lobatto points, and Lagrange
   polonomials are orthogonal in these points(and only in these points(η,ζ)),
   the integration over the "mass" yields(2D)
   M_{ij}=∫₁¹ ρ⋅L(η)⋅L(ζ) dη dζ = ρ⋅δ_{ij}
   eg. the mass matrix is diagonal
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for memset */
#include <math.h>
#include "fedata.h"
#include <iostream>
using namespace std;

/* Force function */
double ricker(double t, double f0, double to);

int main(){

  /* Initialize mesh and material */
  FEMclass mesh(5,5,5,1,1,1,3);
  MATPROPclass mat(1,1,1,1);

  /* size along each dimension (number of element dofs) */
  /* http://www.cplusplus.com/reference/cstring/memset/ */
  int sizeKe = pow(mesh.ngll,3)*3;
  double KKe[sizeKe*sizeKe];
  double Me[sizeKe];
  memset(KKe,0,sizeof(KKe));
  memset(Me,0,sizeof(Me));
  // NOTE Henrik: Vær forsigtig med at bruge sizeof(KKe). Det duer kun her fordi
  // KKe virkelig er et array. Hvis den fx. blev overført til en funktion,
  // skulle man nok skrive sizeof(KKe[0])*sizeKe*sizeKe.
  // cout << "sizeof ME" << sizeof(Me) << endl; // = 648/8 = 81. Korrekt

  /* Read pre-integrated local mass(diagonal due to SEM) and Ke from file*/
  /* emacs:
     apply-macro-to-region-lines*/
  /* THIS is WHACK, HACK and not good practice. But... */
  /* And remember the file must NOT be included as a source file! */
  double lx=mesh.lx, ly=mesh.ly, lz=mesh.lz;
  double E=mat.e,nu=mat.nu;
#include "Me3.c"
#include "Ke3.c"


  /* Time stepping */
  int NT = 1000;
  int ii;
  /* Force function. Gauss like: Mexican hat */
  double Ff0 = 0.25;            /* fundamental frequency */
  double Ft0 = 1.5/Ff0;         /* delay */
  double Ft;

  /* ############################################ */
  /* Testet output HERTIL. Giver samme som SEM.m  */

  /* The timestep dt is set by the stability condition CLF */
  /* http://en.wikipedia.org/wiki/Courant%E2%80%93Friedrichs%E2%80%93Lewy_condition#The_two_and_general_n-dimensional_case */

  /* Ask Boyan  CLF and Δx/Δy Is that the element length, or minimum distance
     between nodes?. */
  double CLF = 1;
  double dt = 1;
  double half_dt = 0.5*dt;

  /* Global elements first node(N0). Used to calculate rest of elements nodes*/
  double No, node;
  int edof[sizeKe];

  double d[mesh.nn*3], v[mesh.nn*3], a[mesh.nn*3];
  memset(d,0, sizeof d), memset(v,0, sizeof v), memset(a,0, sizeof a);
  double KeDe;
  /* Explicit time stepping */
  for(int it; it<NT; it++){

    /* prediction of mid-step displacement: */
    /* d_mid = d_old + 0.5*dt*v_old */
    for(int i=0;i<mesh.nn*3;i++)
      d[i] = d[i] + half_dt*v[i];

     /* source time function */
    Ft = ricker(it*dt-0.5*dt, Ff0,Ft0)*0.01;


    for(int elx=0; elx<mesh.nelx; elx++){
      for(int ely=0; ely<mesh.nely; ely++){
        for(int elz=0; elz<mesh.nelz; elz++){
          No = mesh.N0(elx,ely,elz);

          /* get global element nodes and */
          /* corressponding dof, edof */
          ii = 0;
          for(int k=0; k<mesh.ngll; k++){
            for(int j=0; j<mesh.ngll; j++){
              for(int i=0; i<mesh.ngll; i++){
                node = No+i+j*mesh.nnx+k*mesh.nnx*mesh.nny;
                edof[ii] = node*3;
                edof[ii+1] = node*3+1;
                edof[ii+2] = node*3+2;
                ii = ii + 3;
              }
            }
          }

          /* accelearation: a = (F-K*d)/M */
          for(int i=0;i<sizeKe;i++){
            KeDe = 0;
            for(int j=0;j<sizeKe;j++){
              KeDe += KKe[j+i*sizeKe]*d[edof[j]];
            }
            a[edof[i]] = a[edof[i]] + (Ft - KeDe)/Me[i];
            //a(edof +1) = a(edof +1) + ( F(edof +1) - KKe*d(edof +1))/M;
          }
        }
      }
    }



    /* update */
    /* v_New = v_old + dt*a_New; */
    /* d_New = d_old + dt*v_old + 0.5*dt^2*a_New */
    /*       = d_mid + 0.5*dt*v_New */
    for(int i=0;i<mesh.nn*3;i++){
      v[i] = v[i] + dt*a[i];
      d[i] = d[i] + half_dt*v[i];
    }

    /* Output stuff */
  }
}


double ricker(double t, double f0, double t0){
  /* Return the force, given by an: */
  /* Mexican hat wavelet */
  /* http://en.wikipedia.org/wiki/Mexican_hat_wavelet */

  double arg = M_PI*f0*(t-t0);
  arg = arg*arg;
  double f = (2*arg-1)*exp(-arg);
  return f;

}

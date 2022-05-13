
#ifndef SPH_DOMAIN_H
#define SPH_DOMAIN_H

#include <stdio.h>    // for NULL
#include <algorithm>  // for min,max

#include <omp.h>


//#ifdef _WIN32 /* __unix__ is usually defined by compilers targeting Unix systems */
#include <sstream>
//#endif
#include <sstream>
#include <string>
#include <cmath>
#include <vector>


//C++ Enum used for easiness of coding in the input files

namespace SPH {

class Vec3_t {
  public:
  Vec3_t(){x[0]=x[1]=x[2]=0.;}
  Vec3_t(const double &x_, const double &y_, const double &z_) {x[0]=x_; x[1]= y_; x[2] = z_;}
  double x[3];
  const double& operator()(const int &i)const{return x[i];}
};
  
class Particle{
public:
  Vec3_t x;
  Particle						(int Tag, Vec3_t const & x0, Vec3_t const & v0, double Mass0, double Density0, double h0, bool Fixed=false){x=Vec3_t(x0);}
private:
 // friend Domain;
};

class Domain
{
public:
    // Constructor
    Domain();

    // Destructor
    ~Domain();

    // Domain Part
    void AddSingleParticle	(int tag, Vec3_t const & x, double Mass, double Density, double h, bool Fixed);		//Add one particle
    void AddBoxLength				(int tag, Vec3_t const &V, double Lx, double Ly, double Lz,double r, double Density,
																	double h,int type, int rotation, bool random, bool Fixed);									//Add a cube of particles with a defined dimensions

	void AddCylinderLength(int tag, Vec3_t const & V, double Rxy, double Lz, 
									double r, double Density, double h, bool Fixed, bool ghost = false);

  void AddQuarterCylinderLength(int tag, double Rxy, double Lz, 
																				double r, double Density, double h, bool Fixed, bool symlength = false);
                                          
	//Cylinder Slice 
	void AddDoubleSymCylinderLength(int tag, double Rxy, double Lz, 
								double r, double Density, double h, bool Fixed, bool symlength = false);
									
	void AddTractionProbeLength(int tag, Vec3_t const & V, double Rxy, double Lz_side,
											double Lz_neckmin,double Lz_necktot,double Rxy_center,
											double r, double Density, double h, bool Fixed);
											
	void Calculate3DMass(double Density);
	void Add3DCubicBoxParticles(int tag, Vec3_t const & V, double Lx, double Ly, double Lz, 
									double r, double Density, double h);


  
  
  
  bool contact_mesh_auto_update;
  inline void ContactNbSearch();	//Performed AFTER neighbour search
	int contact_surf_id;						//particles id from surface


  //TEST
  //Forces calculation time spent
  double forces_tensile_inst_calctime, forces_stressstrain_calctime,forces_acc_calctime,
          forces_artif_visc_calctime;
	
	/////////////// MEMBERS //
    // Data
	//    std::vector< *Particle >				Particles; 	///< Array of particles
	std::vector <Particle*>				Particles; 	///< Array of particles
    double					R;		///< Particle Radius in addrandombox

		double					sqrt_h_a;				//Coefficient for determining Time Step based on acceleration (can be defined by user)
		double 					min_force_ts;		//min time step size due to contact forces
		
    int 					Dimension;    	///< Dimension of the problem

    double					MuMax;		///< Max Dynamic viscosity for calculating the timestep
    double					CsMax;		///< Max speed of sound for calculating the timestep
	double 					Vol;		///LUCIANO
	
		bool cont_heat_gen;
	
		int 						first_fem_particle_idx;			//The rest are ridig bodies
		/*Array<*/int/*>*/ 			id_free_surf;								//TODO: 
    Vec3_t					Gravity;       	///< Gravity acceleration


    Vec3_t                 			TRPR;		///< Top right-hand point at rear of the domain as a cube
    Vec3_t                  			BLPF;           ///< Bottom left-hand point at front of the domain as a cube
    Vec3_t                  			CellSize;      	///< Calculated cell size according to (cell size >= 2h)
    int		                		CellNo[3];      ///< No. of cells for linked list
    double 					hmax;		///< Max of h for the cell size  determination
    Vec3_t                 			DomSize;	///< Each component of the vector is the domain size in that direction if periodic boundary condition is defined in that direction as well
    double					rhomax;

    int						*** HOC;	///< Array of "Head of Chain" for each cell

    bool					FSI;						///< Selecting variable to choose Fluid-Structure Interaction
		int						contact_type;		//0: no contact 1: node to surface 2: node 2 node
		bool					thermal_solver;
	
	// BONET KERNEL CORRECTION
	bool 					gradKernelCorr;	
	
    double 					XSPH;		///< Velocity correction factor
    double 					InitialDist;	///< Initial distance of particles for Inflow BC

    double					AvgVelocity;	///< Average velocity of the last two column for x periodic constant velocity
	//double 					getCellfac(){return Cellfac;}





	//double 	& getTime (){return Time;}		//LUCIANO
		bool 									update_contact_surface;

  std::vector <std::pair<size_t,size_t> >		Initial;
    //Mat3_t I;

	double T_inf;			//LUCIANO: IN CASE OF ONLY ONE CONVECTION TEMPERAURE
	

	bool					m_isNbDataCleared;
	bool						auto_ts;				//LUCIANO: Auto Time Stepping
	

	//CONTACT 
	double PFAC, DFAC;		// Penalty and damping factors
	bool 		contact;
	double max_contact_force;
  
  double contact_force_sum;
  
  double m_scalar_prop;  //User Defined Domain Property
		
    
  //ATTENTION: REDUNDANT, ghost pairs and reference
	//Array<std::pair<size_t,size_t> > GhostPairs;	//If used
	
	/////////////////////// SOA (Since v0.4) ///////////////////////////////////
	Vec3_t **m_x,*m_v,*m_a;
	double **m_h;
	double **m_T, **m_Tinf, **m_kT, **m_hcT, **m_cpT, **m_dTdt;
	double **m_qconvT,**m_qT;	//thermal source terms 
	double **m_rho, **m_mass;
  //Mechanics
  double *sigma;
	double *strrate,*rotrate;//all flattened, six component (rotation rate is upped matrix component, since it is antisymm)
	double *shearstress,*shearstressa,*shearstressb;
	double *strain,*straina,*strainb;
	

  
	private:
		// bool  Domain::CheckRadius(Particle* P1, Particle *P2);
		// void Periodic_X_Correction	(Vec3_t & x, double const & h, Particle * P1, Particle * P2);		//Corrects xij for the periodic boundary condition
		// void AdaptiveTimeStep				();		//Uses the minimum time step to smoothly vary the time step

		// void PrintInput			(char const * FileKey);		//Print out some initial parameters as a file
		// void InitialChecks	();		//Checks some parameter before proceeding to the solution
		// void TimestepCheck	();		//Checks the user time step with CFL approach

		// size_t					VisEq;					//Choose viscosity Eq based on different SPH discretisation
		// size_t					KernelType;			//Choose a kernel
		// size_t					GradientType;		//Choose a Gradient approach 1/Rho i^2 + 1/Rho j^2 or 1/(Rho i * Rho j)
		// double 					Cellfac;				//Define the compact support of a kernel

		// double					Time;    				//Current time of simulation at each solving step
		// double					deltat;					//Time Step
    // double					deltatmin;			//Minimum Time Step
    // double					deltatint;			//Initial Time Step
		
		// int 						cont_pairs;
		// bool enable_th_exp;
		// bool enable_plastic_heat_gen;
		// void AllocateNbPair(const int &temp1, const int &temp2, const int &T);
		

};


}; // namespace SPH

#endif // SPH_DOMAIN_H

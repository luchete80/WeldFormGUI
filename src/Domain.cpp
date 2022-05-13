/***********************************************************************************
* PersianSPH - A C++ library to simulate Mechanical Systems (solids, fluids        *
*             and soils) using Smoothed Particle Hydrodynamics method              *
* Copyright (C) 2013 Maziar Gholami Korzani and Sergio Galindo-Torres              *
*                                                                                  *
* This file is part of PersianSPH                                                  *
*                                                                                  *
* This is free software; you can redistribute it and/or modify it under the        *
* terms of the GNU General Public License as published by the Free Software        *
* Foundation; either version 3 of the License, or (at your option) any later       *
* version.                                                                         *
*                                                                                  *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY  *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A  *
* PARTICULAR PURPOSE. See the GNU General Public License for more details.         *
*                                                                                  *
* You should have received a copy of the GNU General Public License along with     *
* PersianSPH; if not, see <http://www.gnu.org/licenses/>                           *
************************************************************************************/

#include "Domain.h"
#include <chrono>
//#include <time.h>       /* time_t, struct tm, difftime, time, mktime */
#include <ctime> //Clock

#include <vector>

#define MIN_PS_FOR_NBSEARCH		1.e-6//TODO: MOVE TO CLASS MEMBER

#include <set>

#include <iostream>

using namespace std;

namespace SPH {


// Constructor
Domain::Domain ()
{
 
}

Domain::~Domain ()
{
	// size_t Max = Particles.size();
	// for (size_t i=1; i<=Max; i++)  Particles.DelItem(Max-i);
}

void Domain::AddBoxLength(int tag, Vec3_t const & V, double Lx, double Ly, double Lz, 
									double r, double Density, double h, int type, int rotation, bool random, bool Fixed) {
    if ( !(type == 0 || type == 1) ) {
	   	std::cout << "Packing Type is out of range. Please correct it and run again" << std::endl;
		std::cout << "0 => Hexagonal Close Packing" << std::endl;
		std::cout << "1 => Cubic Packing" << std::endl;
	    abort();
    }

    if (!(rotation==0 || rotation==90)) {
	   	std::cout << "Packing Rotation Angle is out of range. Please correct it and run again" << std::endl;
		std::cout << "0 => " << std::endl;
		std::cout << "0 0 0 0" << std::endl;
		std::cout << " 0 0 0 0" << std::endl;
		std::cout << "0 0 0 0" << std::endl;
		std::cout << " 0 0 0 0" << std::endl;
		std::cout << std::endl;
		std::cout << "90 => Cubic Close Packing" << std::endl;
		std::cout << "  0   0" << std::endl;
		std::cout << "0 0 0 0" << std::endl;
		std::cout << "0 0 0 0" << std::endl;
		std::cout << "0   0  " << std::endl;
		abort();
    }

//	Util::Stopwatch stopwatch;
    std::cout << "\n--------------Generating particles by AddBoxLength with defined length of particles-----------" << std::endl;

    size_t PrePS = Particles.size();

    double x,y,xp,yp;
    size_t i,j;

    double qin = 0.03;
    srand(100);
		
		//For new SOA accessing
		std::vector <Vec3_t> x_sta;

    if (Dimension==3) {
    	if (type==0) {
    		//Hexagonal close packing
    		double z,zp;
			size_t k=0;
			zp = V(2);

			while ( zp <= (V(2)+Lz-r) ) {
				
				j = 0;
				yp = V(1);
				while (yp <= (V(1)+Ly-r)) {
					i = 0;
					xp = V(0);
					while (xp <= (V(0)+Lx-r))
					{
						if ((k%2!=0) && (j%2!=0)) x = V(0) + (2*i+(j%2)+(k%2)-1)*r; else x = V(0) + (2*i+(j%2)+(k%2)+1)*r;
						y = V(1) + (sqrt(3.0)*(j+(1.0/3.0)*(k%2))+1)*r;
						z = V(2) + ((2*sqrt(6.0)/3)*k+1)*r;
						if (random) Particles.push_back(new Particle(tag,Vec3_t((x + qin*r*double(rand())/RAND_MAX),(y+ qin*r*double(rand())/RAND_MAX),(z+ qin*r*double(rand())/RAND_MAX)),Vec3_t(0,0,0),0.0,Density,h,Fixed));
						else    		{Particles.push_back(new Particle(tag,Vec3_t(x,y,z),Vec3_t(0,0,0),0.0,Density,h,Fixed));
													x_sta.push_back(Vec3_t(x,y,z));
						}
						i++;
						if ((k%2!=0) && (j%2!=0)) xp = V(0) + (2*i+(j%2)+(k%2)-1)*r; else xp = V(0) + (2*i+(j%2)+(k%2)+1)*r;
					}
					j++;
					yp = V(1) + (sqrt(3.0)*(j+(1.0/3.0)*(k%2))+1)*r;
				}
				k++;
				zp = V(2) + ((2*sqrt(6.0)/3)*k+1)*r;
				//cout << "Z: "<<z<<endl;
			}
    	}
    	else {
    		//Cubic packing
    		double z,zp;
			size_t k=0;
			zp = V(2);

			while (zp <= (V(2)+Lz-r)) {
				j = 0;
				yp = V(1);
				while (yp <= (V(1)+Ly-r))
				{
					//cout << "Y: "<<yp<<endl;
					i = 0;
					xp = V(0);
					while (xp <= (V(0)+Lx-r))
					{
						x = V(0) + (2.0*i+1)*r;
						y = V(1) + (2.0*j+1)*r;
						z = V(2) + (2.0*k+1)*r;
						if (random) Particles.push_back(new Particle(tag,Vec3_t((x + qin*r*double(rand())/RAND_MAX),(y+ qin*r*double(rand())/RAND_MAX),(z+ qin*r*double(rand())/RAND_MAX)),Vec3_t(0,0,0),0.0,Density,h,Fixed));
						else    		Particles.push_back(new Particle(tag,Vec3_t(x,y,z),Vec3_t(0,0,0),0.0,Density,h,Fixed));
						x_sta.push_back(Vec3_t(x,y,z));
						i++;
						xp = V(0) + (2*i+1)*r; //COMMENTED BY LUCIANO
						//cout << "X: "<<xp<<endl;
					}
					j++;
					yp = V(1) + (2.0*j+1)*r;//COMMENTED BY LUCIANO
				}
				k++;
				zp = V(2) + (2.0*k+1)*r;//COMMENTED BY LUCIANO
				cout << "Z: "<<z<<endl;
			}
    	}

        //Calculate particles' mass in 3D
        Vec3_t temp, Max=V;
		// for (size_t i=PrePS; i<Particles.size(); i++) {
			// if (Particles[i]->x(0) > Max(0)) Max(0) = Particles[i]->x(0);
			// if (Particles[i]->x(1) > Max(1)) Max(1) = Particles[i]->x(1);
			// if (Particles[i]->x(2) > Max(2)) Max(2) = Particles[i]->x(2);
		// }
		// Max +=r;
		// temp = Max-V;
		cout << "BoxDimensions: "<<temp(0)<<", "<<temp(1)<<", "<<temp(2)<<", "<<endl;
		double Mass = temp(0)*temp(1)*temp(2)*Density/(Particles.size()-PrePS);
		
		cout << "Particle mass: " << Mass <<endl;
		
		// New SOA members
		cout << "Allocating "<<endl;
		// Initiate (&m_x,Particles.size());
		// Initiate (&m_h,Particles.size());
		// Initiate (&m_kT,Particles.size());
		// Initiate (&m_cpT,Particles.size());
		// Initiate (&m_hcT,Particles.size());
		// Initiate (&m_qconvT,Particles.size());
		// Initiate (&m_T,Particles.size());		
		// Initiate (&m_Tinf,Particles.size());
		// Initiate (&m_dTdt,Particles.size());
		// Initiate (&m_rho,Particles.size());
		// Initiate (&m_mass,Particles.size());
		cout << "Done."<<endl;


		// #pragma omp parallel for num_threads(Nproc)
		// #ifdef __GNUC__
		// for (size_t i=0; i<Particles.size(); i++)	//Like in Domain::Move
		// #else
		// for (int i=0; i<Particles.size(); i++)//Like in Domain::Move
		// #endif
		// {
			// Particles[i]->Mass = Mass;
		// }
    } else if (Dimension==2) {
    	if (type==0)
    	{
    		//Hexagonal close packing
    		if (rotation==0)
    		{
				j = 0;
				yp = V(1);

				while (yp <= (V(1)+Ly-r))
				{
					i = 0;
					xp = V(0);
					while (xp <= (V(0)+Lx-r))
					{
						x = V(0) + (2*i+(j%2)+1)*r;
						y = V(1) + (sqrt(3.0)*j+1)*r;
						if (random) Particles.push_back(new Particle(tag,Vec3_t((x + qin*r*double(rand())/RAND_MAX),(y+ qin*r*double(rand())/RAND_MAX),0.0),Vec3_t(0,0,0),(sqrt(3.0)*r*r)*Density,Density,h,Fixed));
							else    Particles.push_back(new Particle(tag,Vec3_t(x,y,0.0),Vec3_t(0,0,0),(sqrt(3.0)*r*r)*Density,Density,h,Fixed));
						i++;
						xp = V(0) + (2*i+(j%2)+1)*r;
					}
					j++;
					yp = V(1) + (sqrt(3.0)*j+1)*r;
				}
			}
    		else
    		{
				i = 0;
				xp = V(0);

				while (xp <= (V(0)+Lx-r))
				{
					j = 0;
					yp = V(1);
					while (yp <= (V(1)+Ly-r))
					{
						x = V(0) + (sqrt(3.0)*i+1)*r;
						y = V(1) + (2*j+(i%2)+1)*r;
						if (random) Particles.push_back(new Particle(tag,Vec3_t((x + qin*r*double(rand())/RAND_MAX),(y+ qin*r*double(rand())/RAND_MAX),0.0),Vec3_t(0,0,0),(sqrt(3.0)*r*r)*Density,Density,h,Fixed));
							else    Particles.push_back(new Particle(tag,Vec3_t(x,y,0.0),Vec3_t(0,0,0),(sqrt(3.0)*r*r)*Density,Density,h,Fixed));
						j++;
						yp = V(1) + (2*j+(i%2)+1)*r;
					}
					i++;
					xp = V(0) + (sqrt(3.0)*i+1)*r;
				}
    		}
    	}
    	else
    	{
    		//Cubic packing
    		j = 0;
			yp = V(1);

			while (yp <= (V(1)+Ly-r))
			{
				i = 0;
				xp = V(0);
				while (xp <= (V(0)+Lx-r))
				{
					x = V(0) + (2*i+1)*r;
					y = V(1) + (2*j+1)*r;
					if (random) Particles.push_back(new Particle(tag,Vec3_t((x + qin*r*double(rand())/RAND_MAX),(y+ qin*r*double(rand())/RAND_MAX),0.0),Vec3_t(0,0,0),(sqrt(3.0)*r*r)*Density,Density,h,Fixed));
						else    Particles.push_back(new Particle(tag,Vec3_t(x,y,0.0),Vec3_t(0,0,0),2.0*r*2.0*r*Density,Density,h,Fixed));
					i++;
					xp = V(0) + (2*i+1)*r;
				}
				j++;
				yp = V(1) + (2*j+1)*r;
			}

    	}
    }
		
		cout << "Particle Count: "<<Particles.size()<< endl;

	R = r;
}

}; // namespace SPH

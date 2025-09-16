#ifndef _MATERIAL_H_
#define _MATERIAL_H_
///////////////////////////////////////////////////////
/////// DUMMY MATERIAL CLASS //////////////////////////
///////////////////////////////////////////////////////

#define BILINEAR				0
#define HOLLOMON				1 //POWER LAW
#define JOHNSON_COOK		2
#define _GMT_         	3

class Elastic_{
	private:
	double E_m, nu_m;	//Poisson and young
	double K_m, G_m;
	
	public:
	Elastic_(){}
	Elastic_(const double &e, const double &nu):E_m(e),nu_m(nu){}
	const double& E()const{return E_m;}
  const double& nu()const{return E_m;}
	
};

class Plastic_{
	
	public:  
  int			Material_model;	//TODO: Change to enum

	//virtual inline double CalcYieldStress();

  ////// NO //virtual FUNCTIONS, HOLLOMON MATERIAL ///////
  double K, m;
	double eps0, eps1;
  double sy0;
  void InitHollomon(){}

  ////// NO //virtual FUNCTIONS, JOHNSON COOK MATERIAL ///////
	double T_t,T_m;	//transition and melting temps
  
  double T_min, T_max;
  double e_min, e_max;
  double er_min, er_max;
 

  virtual const int getType()const{return Material_model;}
  //THERMAL
  double k_T, cp_T; ///MAYBE MOVE TO element or nodes
  
};

class Material_{
	
	public:
	Material_(){
    m_plastic = nullptr;
    m_isplastic = false;
  }
	Material_(const Elastic_ el):elastic_m(el){
    m_plastic = nullptr;
    m_isplastic = false;
  }
	virtual inline double CalcTangentModulus(){return 0.0;};
	virtual inline double CalcTangentModulus(const double &strain, const double &strain_rate, const double &temp){return 0.0;};
	virtual inline double CalcTangentModulus(const double &strain){return 0.0;};
	virtual inline double CalcYieldStress(){return 0.0;}
	virtual inline double CalcYieldStress(const double &strain){return 0.0;};
	virtual inline double CalcYieldStress(const double &strain, const double &strain_rate, const double &temp){return 0.0;}
	const Elastic_& Elastic()const{return elastic_m;}
  ~Material_(){};
  
  void setDensityConstant(const double &val){m_density = val;};
  const double& getDensityConstant()const {return m_density;}; //COULD DEFINE IT AS TEMP 0 and 
  Plastic_* getPlastic(){return m_plastic;}
  const bool & isPlastic()const{return m_isplastic;}
  
	
  double m_density;
  int			Material_model;	//TODO: Change to enum
  Elastic_ elastic_m;
  Plastic_ *m_plastic;
	double E_m, nu;	//TODO, move to elastic class
  bool m_isplastic = false;
  
  

};


//TODO: derive johnson cook as plastic material flow
class JohnsonCook:
public Plastic_{
	double T_t,T_m;	//transition and melting temps
	double A, B, C;
	double n, m;
	double eps_0;
	
	public:
	JohnsonCook(){}
	//You provide the values of A, B, n, m, 
	//θmelt, and  θ_transition
	//as part of the metal plasticity material definition.
	//~ JohnsonCook(const Elastic_ &el,const double &a, const double &b, const double &n_, 
              //~ const double &c, const double &eps_0_,
              //~ const double &m_, const double &T_m_, const double &T_t_):
	//~ Material_(el),A(a),B(b),C(c),
  //~ m(m_),n(n_),eps_0(eps_0_),T_m(T_m_),T_t(T_t_)
  //~ {}
	//~ inline double CalcYieldStress(){return 0.0;}	
	//~ inline double CalcYieldStress(const double &plstrain){
     //~ double Et =0.;

    //~ // if (plstrain > 0.)
      //~ // Et = n * B * pow(plstrain,n-1.);
    //~ // else 
      //~ // Et = Elastic().E()*0.1; //ARBITRARY! TODO: CHECK MATHEMATICALLY
    //~ return Et;
  //~ } //TODO: SEE IF INCLUDE	
	//~ inline double CalcYieldStress(const double &strain, const double &strain_rate, const double &temp);	
	//~ inline double CalcTangentModulus(const double &strain, const double &strain_rate, const double &temp);

};

class Hollomon:
public Plastic_{
	double K, m;
	double eps0;
  double eps1;  //if has a perfectly plastic algoritm
  double sy0;
	
	public:
	Hollomon(){
    Material_model = HOLLOMON;
    }
	//You provide the values of A, B, n, m, 
	//θmelt, and  θ_transition
	//as part of the metal plasticity material definition.
	//ASSUMING AT FIRST COEFFICIENTS ARE GIVEN TO TOTAL STRAIN-STRESS
	//~ Hollomon(const double eps0_, const double &k_, const double &m_):
	//~ K(k_), m(m_){ eps0 = eps0_;}
	//~ Hollomon(const Elastic_ &el, const double sy0_, const double &k_, const double &m_);
  
	//~ inline double CalcTangentModulus(const double &strain);
	//~ inline double CalcYieldStress(){return 0.0;}	
	//~ inline double CalcYieldStress(const double &strain);	
};


//TODO: derive johnson cook as plastic material flow
class GMT:
public Plastic_{
	public:
  
	double C1, C2;
  double n1, n2; //Strain hardening exponent
	double m1, m2;
  double I1, I2; //EXPONENTIAL TERMS
	double eps_0; //ONLY FOR JC DAMAGE , CORRECT THIS
  
	
	GMT(){
    Material_model = _GMT_;
  }
	//You provide the values of A, B, n, m, 
	//θmelt, and  θ_transition
	//as part of the metal plasticity material definition.
	GMT(//const Elastic_ &el, 
              const double &n1_,  const double &n2_,  
              const double &C1_,  const double &C2_, 
              const double &m1_,  const double &m2_,
              const double &I1_,  const double &I2_,
              const double &e_min_  = 0.0,const double &e_max_  =1.0e10, /*const double &e_0 = 1.0,*/
              const double &er_min_ = 0.0,const double &er_max_ =1.0e10,
              const double &T_min_  = 0.0,const double &T_max_  =1.0e10):
	//Material_(el),
  C1(C1_),C2(C2_),
  n1(n1_),n2(n2_),
  m1(m1_),m2(m2_),
  I1(I1_),I2(I2_)
  {
    e_min =e_min_; e_max =e_max_;
    er_min=er_min_;er_max=er_max_;
    T_min =T_min_; T_max =T_max_;
    Material_model = _GMT_;
		// T_m=T_m_;
		// T_t=T_t_;
	}
	inline double CalcYieldStress(){return 0.0;}	
	inline double CalcYieldStress(const double &plstrain){
     // double Et =0.;

    // if (plstrain > 0.)
      // Et = n * B * pow(plstrain,n-1.);
    // else 
      // Et = Elastic().E()*0.1; //ARBITRARY! TODO: CHECK MATHEMATICALLY
    // return Et;
  } //TODO: SEE IF INCLUDE	
	inline double CalcYieldStress(const double &strain, const double &strain_rate, const double &temp);	
	inline double CalcTangentModulus(const double &strain, const double &strain_rate, const double &temp);
  double &getRefStrainRate(){return eps_0;}//only for JC
  //~JohnsonCook(){}
};

#endif

#ifndef _MATERIAL_H_
#define _MATERIAL_H_
///////////////////////////////////////////////////////
/////// DUMMY MATERIAL CLASS //////////////////////////
///////////////////////////////////////////////////////

#define NONE				    0
#define BILINEAR				1
#define HOLLOMON				2 //POWER LAW
#define JOHNSON_COOK		3
#define _GMT_         	4
#define NORTON_HOFF     5

#include <cmath>
#include <cstdio>
#include <vector>
class Elastic_{
	private:
	double E_m, nu_m;	//Poisson and young
	double K_m, G_m;
	
	public:
	Elastic_(){}
	Elastic_(const double &e, const double &nu):E_m(e),nu_m(nu){
    K_m = e / (3.0 * (1.0 - 2.0 * nu));
    G_m = e / (2.0 * (1.0 + nu));
  }
	const double& E()const{return E_m;}
  const double& nu()const{return nu_m;}
  const double& Poisson()const{return nu_m;}
  const double& G()const{return G_m;}
  const double& BulkMod()const{return K_m;}
	
};



class Plastic_{
	
	public:  
  int			Material_model;	//TODO: Change to enum

	//virtual inline double CalcYieldStress();

  ////// NO //virtual FUNCTIONS, HOLLOMON MATERIAL ///////
  double K, m;
	double eps0, eps1;
  double sy0;
  double Et;
  void InitHollomon(){}

  ////// NO //virtual FUNCTIONS, JOHNSON COOK MATERIAL ///////
	double T_t,T_m;	//transition and melting temps
  
  double T_min, T_max;
  double e_min, e_max;
  double er_min, er_max;

  virtual ~Plastic_() {}
    
  virtual Plastic_* clone() const = 0;

  virtual const int getType()const{return Material_model;}
  //THERMAL

  virtual std::vector<double> getPlasticConstants(){
    return std::vector<double>(); // Return empty vector    
  }
};

class Material_{
	
	public:
	Material_(){
    m_plastic = nullptr;
    m_isplastic = false;
    Material_model = NONE;
    InitializeDefaults();
  }
	Material_(const Elastic_ el):elastic_m(el){
    m_plastic = nullptr;
    m_isplastic = false;
    Material_model = NONE;
    InitializeDefaults();
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

  void InitializeDefaults() {
    cs0 = 0.0;
    Ep = 0.0;
    K = 0.0;
    m = 0.0;
    eps0 = 0.0;
    eps1 = 0.0;
    sy0 = yieldStress0;
    visc_relax_time = 0.0;
    perzyna_m = 0.0;
    T_t = 0.0;
    T_m = 0.0;
    T_min = 0.0;
    T_max = 0.0;
    e_min = 0.0;
    e_max = 1.0e10;
    er_min = 0.0;
    er_max = 1.0e10;
    A = 0.0;
    B = 0.0;
    C = 0.0;
    n = 0.0;
    eps_0 = 0.0;
    K_visco = 0.0;
    n_visco = 0.0;
    C1 = 0.0;
    C2 = 0.0;
    m1 = 0.0;
    m2 = 0.0;
    n1 = 0.0;
    n2 = 0.0;
    I1 = 0.0;
    I2 = 0.0;
    exp_T = 0.0;
    K_nh = 0.0;
    m_nh = 0.0;
    epsdot0 = 1.0e-3;
  }

  void InitHollomon() {}

  void InitHollomon(const Elastic_ &el, const double sy0_, const double &k_, const double &m_) {
    elastic_m = el;
    K = k_;
    m = m_;
    Material_model = HOLLOMON;
    eps0 = sy0_ / el.E();
    sy0 = sy0_;
    yieldStress0 = sy0_;
    eps1 = std::pow(sy0_ / k_, 1.0 / m_);
    if (strRange.size() < 2) {
      strRange = {0.0, 0.65};
    }
    e_min = strRange[0];
    e_max = strRange[1];
    if (eps0 > eps1) {
      std::printf("ERROR, Hollomon material bad definition, please correct Yield Stress, Elastic Modulus or Material hardening constants.");
    }
  }

  void Init_JohnsonCook(const Elastic_ &el, const double &a, const double &b, const double &n_,
                        const double &c, const double &eps_0_, const double &m_,
                        const double &T_m_, const double &T_t_) {
    elastic_m = el;
    Material_model = JOHNSON_COOK;
    sy0 = a;
    yieldStress0 = a;
    A = a;
    B = b;
    C = c;
    n = n_;
    m = m_;
    eps_0 = eps_0_;
    T_m = T_m_;
    T_t = T_t_;
  }

  void Init_GMT(const Elastic_ &el, const double &n1_, const double &n2_,
                const double &C1_, const double &C2_, const double &m1_,
                const double &m2_, const double &I1_, const double &I2_,
                const double &e_min_, const double &e_max_,
                const double &er_min_, const double &er_max_,
                const double &T_min_, const double &T_max_) {
    elastic_m = el;
    Material_model = _GMT_;
    n1 = n1_;
    n2 = n2_;
    C1 = C1_;
    C2 = C2_;
    m1 = m1_;
    m2 = m2_;
    I1 = I1_;
    I2 = I2_;
    e_min = e_min_;
    e_max = e_max_;
    er_min = er_min_;
    er_max = er_max_;
    T_min = T_min_;
    T_max = T_max_;
    strRange = {e_min_, e_max_};
  }
  
	
  double m_density;
  int			Material_model;	//TODO: Change to enum
  Elastic_  elastic_m;
  Plastic_ *m_plastic;
	double E_m, nu;	//TODO, move to elastic class
  double cs0;
  double Ep;
  double K, m;
  double eps0, eps1;
  double sy0;
  double visc_relax_time;
  double perzyna_m;
  double T_t, T_m;
  double T_min, T_max;
  double e_min, e_max;
  double er_min, er_max;
  double A, B, C;
  double n;
  double eps_0;
  double K_visco, n_visco;
  double C1, C2, m1, m2, n1, n2, I1, I2;
  bool m_isplastic = false;
  double k_T  = 0.0;
  double cp_T = 0.0; ///MAYBE MOVE TO element or nodes
  double exp_T = 0.0;
  double K_nh = 0.0;
  double m_nh = 0.0;
  double epsdot0 = 1.0e-3;
  double yieldStress0 = 190.0E6;
  std::vector<double> strRange = {0.0, 0.65};

};

inline double ClampMaterialValue(const double value, const double min_value, const double max_value) {
  if (value < min_value) return min_value;
  if (value > max_value) return max_value;
  return value;
}

inline double CalcHollomonYieldStress(const double &strain, Material_ *mat) {
  double e = ClampMaterialValue(strain, mat->e_min, mat->e_max);
  if (e + mat->eps0 > mat->eps1) {
    return mat->K * std::pow(e + mat->eps0, mat->m);
  }
  return mat->sy0;
}

inline double CalcHollomonTangentModulus(const double &strain, Material_ *mat) {
  if (strain + mat->eps0 > mat->eps1) {
    return mat->K * mat->m * std::pow(strain + mat->eps0, mat->m - 1.0);
  }
  return 0.0;
}

inline double CalcNortonHoffEqStress(const double &edot_eq, Material_ *mat) {
  const double ed = edot_eq + mat->epsdot0;
  return mat->K_nh * std::pow(ed, mat->m_nh);
}

inline double CalcJohnsonCookYieldStress(const double &strain, const double &strain_rate,
                                         const double &temp, Material_ *mat) {
  const double thermal_span = mat->T_m - mat->T_t;
  double T_h = 0.0;
  if (thermal_span != 0.0) {
    T_h = (temp - mat->T_t) / thermal_span;
  }
  if (T_h < 0.0) T_h = 0.0;
  if (T_h > 1.0) T_h = 1.0;

  double sr = strain_rate;
  if (sr <= 0.0) sr = 1.0e-5;

  double ref_rate = mat->eps_0;
  if (ref_rate <= 0.0) ref_rate = 1.0;

  return (mat->A + mat->B * std::pow(strain, mat->n)) *
         (1.0 + mat->C * std::log(sr / ref_rate)) *
         (1.0 - std::pow(T_h, mat->m));
}

inline double CalcJohnsonCookTangentModulus(const double &plstrain, const double &strain_rate,
                                            const double &temp, Material_ *mat) {
  const double thermal_span = mat->T_m - mat->T_t;
  double T_h = 0.0;
  if (thermal_span != 0.0) {
    T_h = (temp - mat->T_t) / thermal_span;
  }
  if (T_h < 0.0) T_h = 0.0;
  if (T_h > 1.0) T_h = 1.0;

  double sr = strain_rate;
  if (sr <= 0.0) sr = 1.0e-5;

  double ref_rate = mat->eps_0;
  if (ref_rate <= 0.0) ref_rate = 1.0;

  if (plstrain > 0.0) {
    return mat->n * mat->B * std::pow(plstrain, mat->n - 1.0) *
           (1.0 + mat->C * std::log(sr / ref_rate)) *
           (1.0 - std::pow(T_h, mat->m));
  }
  return mat->Elastic().E() * 0.1;
}

inline double CalcGMTYieldStress(const double &strain, const double &strain_rate,
                                 const double &temp, Material_ *mat) {
  const double e = ClampMaterialValue(strain, mat->e_min, mat->e_max);
  const double er = ClampMaterialValue(strain_rate, mat->er_min, mat->er_max);
  const double T = ClampMaterialValue(temp, mat->T_min, mat->T_max);

  return mat->C1 * std::exp(mat->C2 * T) *
         std::pow(e, mat->n1 * T + mat->n2) *
         std::exp((mat->I1 * T + mat->I2) / e) *
         std::pow(er, mat->m1 * T + mat->m2);
}

inline double CalcGMTTangentModulus(const double &plstrain, const double &strain_rate,
                                    const double &temp, Material_ *mat) {
  const double e = ClampMaterialValue(plstrain, mat->e_min, mat->e_max);
  const double er = ClampMaterialValue(strain_rate, mat->er_min, mat->er_max);
  const double T = ClampMaterialValue(temp, mat->T_min, mat->T_max);

  return mat->C1 * std::exp(mat->C2 * T) * std::pow(er, mat->m1 * T + mat->m2) *
         std::pow(e, T * mat->n1 + mat->n2 - 2.0) *
         (-mat->I1 * T - mat->I2 + e * (mat->n1 * T + mat->n2)) *
         std::exp((mat->I1 * T + mat->I2) / e);
}


class Bilinear:
public Plastic_{
  double sy0;
  double Et;
	
	public:
	Bilinear(){
    Material_model = BILINEAR;
    }
	Bilinear(const double &_sy0, const double &_Et){
    sy0 = _sy0;
    Et = _Et;
    Material_model = BILINEAR;
    }
    
	//You provide the values of A, B, n, m, 
	//θmelt, and  θ_transition
	//as part of the metal plasticity material definition.
	//ASSUMING AT FIRST COEFFICIENTS ARE GIVEN TO TOTAL STRAIN-STRESS
	//~ Hollomon(const double eps0_, const double &k_, const double &m_):
	//~ K(k_), m(m_){ eps0 = eps0_;}
	//~ Hollomon(const Elastic_ &el, const double sy0_, const double &k_, const double &m_);

    Plastic_* clone() const override { return new Bilinear(*this); }

      
	//~ inline double CalcTangentModulus(const double &strain);
	//~ inline double CalcYieldStress(){return 0.0;}	
	//~ inline double CalcYieldStress(const double &strain);	
  virtual std::vector <double> getPlasticConstants(){
    std::vector<double> ret;
    ret.push_back(sy0);ret.push_back(Et);
    return ret;
  }
};


//TODO: derive johnson cook as plastic material flow
class JohnsonCook:
public Plastic_{
	double T_t,T_m;	//transition and melting temps
	double A, B, C;
	double n, m;
	double eps_0;
	
	public:
	JohnsonCook(){
    Material_model = JOHNSON_COOK;
  }
	JohnsonCook(const double &_B, const double &_n, const double &_C,
              const double &_eps_0, const double &_m,
              const double &_T_m, const double &_T_t,
              const double &_A = 0.0) {
    A = _A;
    B = _B;
    C = _C;
    n = _n;
    m = _m;
    eps_0 = _eps_0;
    T_m = _T_m;
    T_t = _T_t;
    Material_model = JOHNSON_COOK;
  }
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

    Plastic_* clone() const override { return new JohnsonCook(*this); }
    virtual std::vector<double> getPlasticConstants() {
      std::vector<double> ret;
      ret.push_back(B);
      ret.push_back(n);
      ret.push_back(C);
      ret.push_back(eps_0);
      ret.push_back(m);
      ret.push_back(T_m);
      ret.push_back(T_t);
      return ret;
    }
    
};

class Hollomon:
public Plastic_{
	double K, n;
	double eps0;
  double eps1;  //if has a perfectly plastic algoritm
  double sy0;
	
	public:
	Hollomon(){
    Material_model = HOLLOMON;
    }
	Hollomon(const double &_K, const double &_n){
    K = _K;
    n = _n;
    Material_model = HOLLOMON;
    }
    
	//You provide the values of A, B, n, m, 
	//θmelt, and  θ_transition
	//as part of the metal plasticity material definition.
	//ASSUMING AT FIRST COEFFICIENTS ARE GIVEN TO TOTAL STRAIN-STRESS
	//~ Hollomon(const double eps0_, const double &k_, const double &m_):
	//~ K(k_), m(m_){ eps0 = eps0_;}
	//~ Hollomon(const Elastic_ &el, const double sy0_, const double &k_, const double &m_);

    Plastic_* clone() const override { return new Hollomon(*this); }

      
	//~ inline double CalcTangentModulus(const double &strain);
	//~ inline double CalcYieldStress(){return 0.0;}	
	//~ inline double CalcYieldStress(const double &strain);	
  virtual std::vector <double> getPlasticConstants(){
    std::vector<double> ret;
    ret.push_back(K);ret.push_back(n);
    return ret;
  }
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
    return 0.0;
  } //TODO: SEE IF INCLUDE	
	inline double CalcYieldStress(const double &strain, const double &strain_rate, const double &temp){return 0.0;}	
	inline double CalcTangentModulus(const double &strain, const double &strain_rate, const double &temp){return 0.0;}
  double &getRefStrainRate(){return eps_0;}//only for JC
  //~JohnsonCook(){}

  Plastic_* clone() const override { return new GMT(*this); }
  virtual std::vector<double> getPlasticConstants() {
    std::vector<double> ret;
    ret.push_back(n1);
    ret.push_back(n2);
    ret.push_back(C1);
    ret.push_back(C2);
    ret.push_back(m1);
    ret.push_back(m2);
    ret.push_back(I1);
    ret.push_back(I2);
    return ret;
  }


};

#endif

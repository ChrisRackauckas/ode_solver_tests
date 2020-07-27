//============================================================================
// Name        : valve_noroot.cpp
// Author      : Lambert Plavecz
// Version     : 2
// Copyright   : no
// Description : Parameter study of a pressure relief valve equation with odeint and rkck54 
//============================================================================


#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <string>
#include <chrono>
#include <boost/numeric/odeint.hpp>

using namespace std;
using namespace boost::numeric::odeint;

const double kappa = 1.25;
const double beta = 20.0;
const double delta = 10.0;
const double r = 0.8;

const int num = 3072; //30720-1199387ms
const double invnum = 1.0 / (num -1);

string file_name = "impact_dyn_output.txt";

double matrix[num][64];

typedef std::vector< double > state_type;

double q = 0.0;

class impact_dyn {
public:
    impact_dyn(){}
    void operator()(const state_type &x, state_type &dxdt, const double t){
    	dxdt[0] = x[1]; //x[0] displacement, x[1] velocity, x[2] pressure in chamber
    	dxdt[1] = -kappa*x[1] - (x[0] + delta) + x[2]; //
    	dxdt[2] = beta * (q - x[0]* sqrt(x[2]));
    }
};

class impact_observer
{
    double extr_prev;
	double y_prev;
	int row_number;
    int count = 0;
    int extremum_count = 0;

public:
    impact_observer(int u): row_number(u){
		extr_prev = 200.0; //some arbitrary large number
	}

    void operator()(state_type &x , double t )
    {
    	if(t==0.0){
    		y_prev = x[1];
    		return;
    	}
		if(x[0] <= 0 && x[1] < 0){ //impact
			x[1] = -r * x[1];
    	}
		
    	if(x[1]*y_prev <= 0){//extremum
    		if(count >= 2048 && count < 2048 + 64){ //saving
    	    	matrix[row_number][extremum_count++] = x[0];
    	    	if( extremum_count == 64) throw 0; //end integration
    	    }
			count++;
			if(fabs(extr_prev - x[0]) < 1.0e-9 && fabs(x[0]) > 1.0e-9){ //check whether solution converges
				//cout << "x_prev: " << x_prev << ", y_prev:" << y_prev << ", x: " << x[0] << endl;
				//if true save the current value and end integration
				while(extremum_count < 64){ //write out the convergent solution into the array
				    matrix[row_number][extremum_count++] = x[0];
				}
				throw -1;
			}
			extr_prev = x[0]; //save extremum for convergence check
    	}
    	y_prev = x[1];
    }
};


//int nums[11] = {256, 768, 1536, 3072, 3840, 5120, 7680, 15360, 30720, 46080, 61440}; // 76800, 92160, 122880, 184320, 307200, 768000, 4147200};

int main() {
	double tolerance = 1e-10;
	cout << "Impact dynamics started\n" << setprecision(17) << num << endl;

	typedef runge_kutta_cash_karp54< state_type , double , state_type , double > stepper_type;
	
	state_type x(3);

	auto t1 = chrono::high_resolution_clock::now();

	impact_dyn impact;
	auto stepper = make_controlled( tolerance, tolerance, stepper_type() );
	for(int u = 0;u < num;u++){
		//cout << u << endl;
		x[0] = 0.2;
		x[1] = 0.0;
		x[2] = 0.0;

		q = 0.2 + u * 9.8 * invnum;
		impact_observer obs(u);
		double t_start = 0.0;
		while(true){
			try{
				integrate_adaptive(stepper, impact, x, t_start, 1.0e23, 0.01, obs);
			}catch(...){
				//cout << "Enough" << endl;
				break;
			}
		}
	}

	ofstream ofs(file_name);
	if(!ofs.is_open())exit(-1);
	ofs.precision(17);
	ofs.flags(ios::scientific);
	for(int u = 0;u < num;u++){ //f1
		ofs << 0.2 + u * 9.8 * invnum;
		for(int i=0; i < 64;i++){
			ofs << " " << matrix[u][i];
		}
		ofs << "\n";
	}

	ofs.flush();
	ofs.close();
	
	auto t2 = chrono::high_resolution_clock::now();
	cout << "Done" << endl;
	cout << "Time (ms):" << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << endl;

	return 0;
}

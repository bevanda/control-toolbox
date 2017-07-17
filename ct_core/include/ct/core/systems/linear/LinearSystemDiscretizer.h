/***********************************************************************************
Copyright (c) 2017, Michael Neunert, Markus Giftthaler, Markus Stäuble, Diego Pardo,
Farbod Farshidian. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of ETH ZURICH nor the names of its contributors may be used
      to endorse or promote products derived from this software without specific
      prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL ETH ZURICH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************************/

#ifndef INCLUDE_CT_CORE_SYSTEMS_LINEAR_LINEARSYSTEMDISCRETIZER_H_
#define INCLUDE_CT_CORE_SYSTEMS_LINEAR_LINEARSYSTEMDISCRETIZER_H_

#include <unsupported/Eigen/MatrixFunctions>

namespace ct {
namespace core {


//! settings for the LinearSystemDiscretizer
struct LinearSystemDiscretizerSettings
{
	enum class APPROXIMATION {
			FORWARD_EULER = 0,
			BACKWARD_EULER,
			TUSTIN,
			MATRIX_EXPONENTIAL
	};

	LinearSystemDiscretizerSettings(double dt, APPROXIMATION approx):
		dt_(dt),
		approximation_(approx)
	{}

	//! discretization time-step
	double dt_;

	//! type of discretization strategy used.
	APPROXIMATION approximation_;
};



//! interface class for a general linear system or linearized system
/*!
 * Defines the interface for a linear system
 *
 * \tparam STATE_DIM size of state vector
 * \tparam CONTROL_DIM size of input vector
 */
template <size_t STATE_DIM, size_t CONTROL_DIM, typename SCALAR = double>
class LinearSystemDiscretizer : public DiscreteLinearSystem<STATE_DIM, CONTROL_DIM, SCALAR>{

public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	typedef typename Eigen::Matrix<SCALAR, STATE_DIM, STATE_DIM> state_matrix_t; //!< state Jacobian type
	typedef typename Eigen::Matrix<SCALAR, STATE_DIM, CONTROL_DIM> state_control_matrix_t; //!< input Jacobian type


	//! constructor
	LinearSystemDiscretizer(
			const SCALAR& dt,
			const std::shared_ptr<LinearSystem<STATE_DIM, CONTROL_DIM, SCALAR>>& linearSystem = nullptr,
			const LinearSystemDiscretizerSettings::APPROXIMATION& approx = LinearSystemDiscretizerSettings::APPROXIMATION::FORWARD_EULER,
			const ct::core::SYSTEM_TYPE& type = ct::core::SYSTEM_TYPE::GENERAL):
				DiscreteLinearSystem<STATE_DIM, CONTROL_DIM, SCALAR>(type),
				linearSystem_(linearSystem),
				settings_(dt, approx)
				{}


	//! constructor
	LinearSystemDiscretizer(
			const LinearSystemDiscretizerSettings& settings,
			const std::shared_ptr<LinearSystem<STATE_DIM, CONTROL_DIM, SCALAR>>& linearSystem = nullptr,
			const ct::core::SYSTEM_TYPE& type = ct::core::SYSTEM_TYPE::GENERAL):
				DiscreteLinearSystem<STATE_DIM, CONTROL_DIM, SCALAR>(type),
				linearSystem_(linearSystem),
				settings_(settings)
				{}


	//! copy constructor
	LinearSystemDiscretizer(const LinearSystemDiscretizer<STATE_DIM, CONTROL_DIM, SCALAR>& other) :
		settings_(other.settings_)
	{
		if(other.linearSystem_ != nullptr)
			linearSystem_ = std::shared_ptr<LinearSystem<STATE_DIM, CONTROL_DIM, SCALAR>> (other.linearSystem_->clone());
	}


	//! destructor
	virtual ~LinearSystemDiscretizer(){};


	//! deep cloning
	virtual LinearSystemDiscretizer<STATE_DIM, CONTROL_DIM, SCALAR>* clone() const override
	{
		return new LinearSystemDiscretizer<STATE_DIM, CONTROL_DIM, SCALAR>(*this);
	}


	//! update the approximation type for the discrete-time system
	void setApproximation(const LinearSystemDiscretizerSettings::APPROXIMATION& approx)
	{
		settings_.approximation_ = approx;
	}


	//! retrieve the approximation type for the discrete-time system
	LinearSystemDiscretizerSettings::APPROXIMATION getApproximation() const
	{
		return settings_.approximation_;
	}


	//! update the linear system
	void setLinearSystem(std::shared_ptr<LinearSystem<STATE_DIM, CONTROL_DIM, SCALAR>>& linearSystem)
	{
		linearSystem_ = linearSystem;
	}


	//! update the time discretization
	void setTimeDiscretization(const SCALAR& dt)
	{
		settings_.dt_ = dt;
	}


	//! update the settings
	void updateSettings(const LinearSystemDiscretizerSettings& settings)
	{
		settings_ = settings;
	}


	//! get A and B matrix for linear time invariant system
	/*!
	 * compute discrete-time linear system matrices A and B
	 * @param x	the state setpoint
	 * @param u the control setpoint
	 * @param n the time setpoint
	 * @param A the resulting linear system matrix A
	 * @param B the resulting linear system matrix B
	 */
	virtual void getAandB(
			const StateVector<STATE_DIM, SCALAR>& x,
			const ControlVector<CONTROL_DIM, SCALAR>& u,
			const int n,
			state_matrix_t& A,
			state_control_matrix_t& B) override
	{
		if(linearSystem_ == nullptr)
			throw std::runtime_error("Error in LinearSystemDiscretizer: linearSystem not properly set.");

		/*!
		 * for an LTI system A and B won't change with time n, hence the linearizations result from the following LTV special case.
		 */
		getAandB(x, u, x, u, n, A, B);
	}


	//! get A and B matrix for linear time varying system
	/*!
	 * compute discrete-time linear system matrices A and B for a linear time-varying system
	 * @param x_n the state at the start of the discretization interval
	 * @param u_n the control at the start of the discretization interval
	 * @param x_n_next the state at the end of the discretization interval
	 * @param u_n_next the control at the end of the discretization interval
	 * @param n the time setpoint
	 * @param A the resulting linear system matrix A
	 * @param B the resulting linear system matrix B
	 */
	virtual void getAandB(
			const StateVector<STATE_DIM, SCALAR>& x_n,
			const ControlVector<CONTROL_DIM, SCALAR>& u_n,
			const StateVector<STATE_DIM, SCALAR>& x_n_next,
			const ControlVector<CONTROL_DIM, SCALAR>& u_n_next,
			const int& n,
			state_matrix_t& A,
			state_control_matrix_t& B)
		{
		if(linearSystem_ == nullptr)
			throw std::runtime_error("Error in LinearSystemDiscretizer: linearSystem not properly set.");


		switch(settings_.approximation_)
		{
		case LinearSystemDiscretizerSettings::APPROXIMATION::FORWARD_EULER:
		{
			/*!
			 * the Forward Euler approximation uses the state and control at the *start* of the ZOH interval to
			 * generate linear approximations A and B.
			 */
			A = state_matrix_t::Identity();
			A += settings_.dt_ * linearSystem_->getDerivativeState(x_n, u_n, n*settings_.dt_);

			B = settings_.dt_ * linearSystem_->getDerivativeControl(x_n, u_n, n*settings_.dt_);

			break;
		}
		case LinearSystemDiscretizerSettings::APPROXIMATION::BACKWARD_EULER:
		{
			/*!
			 * the Backward Euler approximation uses the state and control at the *end* of the ZOH interval to
			 * generate linear approximations A and B.
			 */
			state_matrix_t aNew = settings_.dt_ * linearSystem_->getDerivativeState(x_n_next, u_n_next, (n+1)*settings_.dt_);
			A = (state_matrix_t::Identity() -  aNew).colPivHouseholderQr().inverse();

			B = A * settings_.dt_ * linearSystem_->getDerivativeControl(x_n_next, u_n_next, (n+1)*settings_.dt_);

			break;
		}
		case LinearSystemDiscretizerSettings::APPROXIMATION::TUSTIN:
		{
			/*!
			 * the Tustin (also known as 'Heun') approximation uses the state and control at the *start* and at the *end*
			 * of the ZOH interval to generate linear approximations A and B in a trapezoidal fashion.
			 */

			//! continuous-time A matrices
			state_matrix_t Ac_front = settings_.dt_ * linearSystem_->getDerivativeState(x_n, u_n, n*settings_.dt_);
			state_matrix_t Ac_back = settings_.dt_ * linearSystem_->getDerivativeState(x_n_next, u_n_next, (n+1)*settings_.dt_);

			//! the continuous-time B matrices
			state_control_matrix_t Bc_front = linearSystem_->getDerivativeControl(x_n, u_n, n*settings_.dt_);
			state_control_matrix_t Bc_back = linearSystem_->getDerivativeControl(x_n_next, u_n_next, (n+1)*settings_.dt_);

			//! midpoint rule
			state_matrix_t Ac_mid = 0.5*(Ac_front + Ac_back);
			state_control_matrix_t Bc_mid = 0.5*(Bc_front + Bc_back);

			//! tustin approximation
			state_matrix_t aNewInv = (state_matrix_t::Identity() -  Ac_mid).colPivHouseholderQr().inverse();
			A = aNewInv * (state_matrix_t::Identity() + Ac_mid);
			B = aNewInv * settings_.dt_ * Bc_mid;

			break;
		}
		case LinearSystemDiscretizerSettings::APPROXIMATION::MATRIX_EXPONENTIAL:
		{
			state_matrix_t Ac = linearSystem_->getDerivativeState(x_n, u_n, settings_.dt_*n);
			state_matrix_t Adt = settings_.dt_ * Ac;

			A = Adt.exp();
			B = Ac.inverse() * (A - state_matrix_t::Identity()) *  linearSystem_->getDerivativeControl(x_n, u_n, settings_.dt_*n);

			break;
		}
		default:
			throw std::runtime_error("Unknown Approximation type in LinearSystemDiscretizer.");
		}	// end switch
		}


private:

	//! shared_ptr to a continuous time linear system (system to be discretized)
	std::shared_ptr<LinearSystem<STATE_DIM, CONTROL_DIM, SCALAR>> linearSystem_;

	//! discretization settings
	LinearSystemDiscretizerSettings settings_;
};



}	// namespace core
}	// namespace ct



#endif /* INCLUDE_CT_CORE_SYSTEMS_LINEAR_LINEARSYSTEMDISCRETIZER_H_ */
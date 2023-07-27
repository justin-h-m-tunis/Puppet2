#pragma once

#ifndef PUPPET_MOTIONCONSTRAINT
#define PUPPET_MOTIONCONSTRAINT

#include <Eigen/dense>
#include <string>
#include <iostream>
#include <unordered_set>

#include "surface.hpp"

using Eigen::seq;

class BoundaryConstraint{

	const Surface<3>* boundary_;
	const Eigen::Matrix4f* boundary_position_;

	Eigen::Vector3f limitTranslate(Eigen::Matrix4f position, Eigen::Vector3f delta_pos, int n_iters = 5) const {
		//Eigen::Vector3f delta_pos = first_guess - position;
		float delta_norm = delta_pos.norm();
		float len_ratio = 1.;
		Eigen::Vector3f longest_vec = Eigen::Vector3f::Zero();
		for (int i = 0; i < n_iters; i++) {
			Eigen::Matrix4f new_tform = position;
			new_tform(seq(0, 2), 3) += longest_vec + len_ratio * delta_pos;
			if (breaksConstraint(position, new_tform)) {

			} else {
				if (len_ratio == 1.) {
					return delta_pos;
				}
				longest_vec += delta_pos * len_ratio;
			}
			len_ratio /= 2;
		}
		return longest_vec;
	}
protected:

	virtual bool invalidPosition(Eigen::Matrix4f position) const {
		return false;
	};

public:
	bool breaksConstraint(Eigen::Matrix4f old_tform, Eigen::Matrix4f new_tform) const { 
		if (boundary_ == nullptr) return false;
		return invalidPosition(new_tform) || 
			boundary_->crossesSurface(old_tform(seq(0, 2), 3)-(*boundary_position_)(seq(0,2),3),
										new_tform(seq(0, 2), 3)-(*boundary_position_)(seq(0,2),3));
	}

	Eigen::Vector3f bestTranslate(Eigen::Matrix4f current, Eigen::Vector3f delta_pos, Eigen::Vector3f normal, Eigen::Vector3f binormal) const {
			//Eigen::Matrix3f delta_pos = target(seq(0, 2), 3) - current(seq(0, 2), 3);
		float delta_norm = delta_pos.norm();
		Eigen::Vector3f fwd = limitTranslate(current, delta_pos);
		float fwd_ratio = fwd.norm() / delta_pos.norm();
		if (fwd_ratio == 1.) {
			return fwd;
		}
		Eigen::Matrix4f next = current;
		next(seq(0,2),3) += fwd;
		Eigen::Vector3f pos_norm = limitTranslate(next, (1 - fwd_ratio) * delta_norm * normal);
		Eigen::Vector3f neg_norm = limitTranslate(next, -(1 - fwd_ratio) * delta_norm * normal);
		Eigen::Vector3f pos_binorm = limitTranslate(next, (1 - fwd_ratio) * delta_norm * binormal);
		Eigen::Vector3f neg_binorm = limitTranslate(next, -(1 - fwd_ratio) * delta_norm * binormal);
		return fwd + (pos_norm + neg_norm) + (pos_binorm + neg_binorm);
	}

	BoundaryConstraint() : boundary_(nullptr),boundary_position_(nullptr) {}

	BoundaryConstraint(const Surface<3>* boundary, const Eigen::Matrix4f* boundary_position) :
		boundary_(boundary),
		boundary_position_(boundary_position){}

	void setBoundary(const Surface<3>* boundary) {
		boundary_ = boundary;
	}
	void setBoundaryPosition(const Eigen::Matrix4f* boundary_position) {
		boundary_position_ = boundary_position;
	}

	const Surface<3>* getBoundary() const {
		return boundary_;
	}

	const Eigen::Matrix4f* getBoundaryPosition() const {
		return boundary_position_;
	}
	// this should be overwritten in the case where both old and new are valid but the movement from one to the other is impossible
	//virtual bool breaksConstraint(Eigen::Vector3f current_pos, Eigen::Vector3f delta_pos) { return true; }// = 0;
	
	//virtual Eigen::Matrix4f getConstrainedTwist(Eigen::Matrix4f current_tform, Eigen::Matrix4f delta_tform) const { return Eigen::Matrix4f::Identity(); }// = 0;
	//virtual Eigen::Vector3f getConstrainedTranslate(Eigen::Vector3f current_pos, Eigen::Vector3f delta_pos) const { return Eigen::Vector3f::Zero(); }// = 0;
};
//all motion constraint 



class PositionConstraint {
	//MotionConstraint* bounds_;
protected:
	const Eigen::Matrix4f* root_transform_;

public:
	virtual const Eigen::Matrix4f& getConstraintTransform() const {
		return Eigen::Matrix4f::Identity();
	};
	virtual const Eigen::Matrix4f& getEndTransform() const {
		return Eigen::Matrix4f::Identity();
	}

	virtual void setRootTransform(const Eigen::Matrix4f* root_transform) {
		root_transform_ = root_transform;
	}
	const Eigen::Matrix4f* getRootTransform() const {
		return root_transform_;
	}
	//PositionConstraint() :root_transform_(nullptr){}
	virtual void refresh() {

	}

};

template<class T>
concept Connector = std::derived_from<T, PositionConstraint> && requires{
	T::getDoF();
} && requires(const T& getter, T& setter, Eigen::Vector<float, T::getDoF()> state_vec) {
	getter.getState();
	setter.setState(state_vec);
};

template<int n_dofs>
class ConnectorConstraint : public PositionConstraint {

	Eigen::Vector<float, n_dofs> state_;
	Eigen::Vector<float, n_dofs> lower_bounds;
	Eigen::Vector<float, n_dofs> upper_bounds;

	Eigen::Matrix4f connector_transform_;
	Eigen::Matrix4f end_transform_;


	//this must be a member of ConnectorConstraint and not simply a new type of connector
	//is because you can only ever have one of these per chain transform
	//its possible to make an "observer" derived class which has two connection points and "observes" the inverse kinematics

	//has unintuitive behavior if called publically
	void boundedMove(Eigen::Vector<float, n_dofs> new_state, const std::vector<BoundaryConstraint*>& bounds, int n_iters) {
		if (n_iters == 0) {
			return;
		} else {
			Eigen::Matrix4f new_tform = computeConnectorTransform(new_state);
			Eigen::Matrix4f old_tform = getConstraintTransform();
			Eigen::Vector<float, n_dofs> delta_state = new_state - getState();
			bool breaks_constraint = false;
			if (root_transform_ != nullptr) {
				old_tform = *root_transform_ * old_tform;
				new_tform = *root_transform_ * new_tform;
			}
			for (auto& bc : bounds) {
				if (bc->breaksConstraint(old_tform,new_tform)) {
					breaks_constraint = true;
					break;
				}
			}
			if (!breaks_constraint) {
				setState(new_state);
			}
			boundedMove(getState() + delta_state / 2, bounds, n_iters - 1);
		}

	}

protected:
	//virtual Eigen::Matrix<float, 6, -1> getJacobian(Eigen::Matrix4f current_tform) {}// = 0;
	//virtual Eigen::Matrix<float, 6, -1> getPositveJacobian(Eigen::Matrix4f current_tform) {}// = 0;
	//virtual Eigen::Matrix4f computeConnectorTransform(float) const = 0;


public:
	static consteval int getDoF() { return n_dofs; };

	virtual Eigen::Matrix4f computeConnectorTransform(Eigen::Vector<float, n_dofs> state_vec) const = 0;

	ConnectorConstraint() :
		state_(Eigen::Vector<float, n_dofs>::Constant(0)),
		connector_transform_(Eigen::Matrix4f::Identity()),
		end_transform_(Eigen::Matrix4f::Identity()){
		setRootTransform(nullptr);//this could be bad: calling virtual function in constructor

	}
	/*
	ConnectorConstraint(const Eigen::Matrix4f* root_transform) :
		state_(Eigen::Vector<float, n_dofs>::Constant(0)),
		connector_transform_(Eigen::Matrix4f::Identity()){
	setRootTransform(root_transform);
	}*/
	

	const Eigen::Matrix4f& getConstraintTransform() const override {
		//i think child position needs to be inverted?
		return connector_transform_;
		//probably not correct kinematics lol
	}

	const Eigen::Matrix4f& getEndTransform() const override {
		return end_transform_;
	}

	virtual void setState(Eigen::Vector<float,n_dofs> new_state) {
		state_ = new_state;
		connector_transform_ = computeConnectorTransform(new_state);
		if (root_transform_ == nullptr) {
			end_transform_ = connector_transform_;
		} else {
			end_transform_ = *root_transform_ * connector_transform_;
		}
	}

	virtual Eigen::Vector<float, n_dofs> getState() const {
		return state_;
	}

	template<int max_iters = 5>
	void boundedMove(Eigen::Vector<float, n_dofs> new_state, const std::vector<BoundaryConstraint*>& bounds) {
		Eigen::Matrix4f new_transform = computeConnectorTransform(new_state);
		Eigen::Matrix4f old_transform = getConstraintTransform();
		bool breaks_constraint = false;
		if (root_transform_ != nullptr) {
			new_transform = *root_transform_ * new_transform;
			old_transform = *root_transform_ * old_transform;
		}
		for (auto& bc : bounds) {
			if (bc->breaksConstraint(old_transform, new_transform)) {
				breaks_constraint = true;
				break;
			}
		}
		if (breaks_constraint) {
			Eigen::Vector<float, n_dofs> delta_state = new_state - getState();
			boundedMove(getState() + delta_state / 2, bounds, max_iters);
		} else {
			setState(new_state);
		}
	}

	void refresh() override {
		setState(getState());
	}


};

template <>
class ConnectorConstraint<0> : public PositionConstraint {
private:
	const Eigen::Matrix4f connector_transform_;
	Eigen::Matrix4f end_transform_;

protected:
public:
	static constexpr int getDoF() { return 0; };

	ConnectorConstraint(Eigen::Matrix4f offset) :
		connector_transform_(offset),
		end_transform_(offset){
		setRootTransform(nullptr);
	}
	ConnectorConstraint(const Eigen::Matrix4f* root_transform,Eigen::Matrix4f offset) :
		connector_transform_(offset),
		end_transform_(*root_transform*offset){
		setRootTransform(root_transform);
	}

	const Eigen::Matrix4f& getConstraintTransform() const override {
		//i think child position needs to be inverted?
		return connector_transform_;
		//probably not correct kinematics lol
	}
	const Eigen::Matrix4f& getEndTransform() const override {
		return end_transform_;
	}

	virtual Eigen::Matrix4f computeConnectorTransform(Eigen::Vector<float, 0> state_vec) {
		return connector_transform_;
	}

	Eigen::Vector<float, 0> getState() const {
		return Eigen::Vector<float, 0>();
	};

	void setState(Eigen::Vector<float, 0> state_vec) {
		if (root_transform_ == nullptr) {
			end_transform_ = connector_transform_;
		}
		else {
			end_transform_ = *root_transform_ * connector_transform_;
		}
	}

	void refresh() override {
		setState(getState());
	}

};

class PrismaticJoint : public ConnectorConstraint<1> {
	const Eigen::Vector3f direction_;

protected:
	

public:
	/*
	Eigen::Matrix4f computeConnectorTransform(float length) const {
		return computeConnectorTransform(Eigen::Vector<float, 1>(length));
	} //maybe this should be the other way around?
	*/
	using ConnectorConstraint::setState;

	void setState(float new_state) {
		setState(Eigen::Vector<float, 1>(new_state));
	}

	Eigen::Matrix4f computeConnectorTransform(Eigen::Vector<float, 1> state_vec) const override {
		Eigen::Matrix4f ret(Eigen::Matrix4f::Identity());
		ret << 1, 0, 0, direction_(0)* state_vec(0),
			0, 1, 0, direction_(1)* state_vec(0),
			0, 0, 1, direction_(2)* state_vec(0),
			0, 0, 0, 1;
		return ret;
	}
	PrismaticJoint(Eigen::Vector3f direction) :
		ConnectorConstraint(),
		direction_(direction){
	}
	/*
	void setState(float new_state) {
		ConnectorConstraint::setState(Eigen::Vector<float, 1>(new_state));
	}*/
	/*
	PrismaticJoint(const Eigen::Matrix4f* root_transform,Eigen::Vector3f direction) :
		ConnectorConstraint(root_transform),
		direction_(direction) {
	}*/
};

class RotationJoint : public ConnectorConstraint<1> {
	const Eigen::Vector3f axis_;
	const Eigen::Matrix3f w_hat_;

	static Eigen::Matrix3f makeSkewMatrix(Eigen::Vector3f axis_) {
		Eigen::Matrix3f w_hat;
		w_hat << 0, -axis_(2), axis_(1),
			axis_(2), 0, -axis_(0),
			-axis_(1), axis_(0), 0;
		return w_hat;
	}

protected:

public:

	/*Eigen::Matrix4f computeConnectorTransform(float angle) const {
		return computeConnectorTransform(Eigen::Vector<float, 1>(angle));
	}*/
	Eigen::Matrix4f computeConnectorTransform(Eigen::Vector<float, 1> state_vec) const override {
		Eigen::Matrix4f ret = Eigen::Matrix4f::Identity();
		//ret(seq(0,2),seq(0,2)) = Matrix3f::Identity() + w_hat_ * sin(state_vec(0)) + w_hat_ * w_hat_ * (1 - cos(state_vec(0)));
		ret(seq(0, 2), seq(0, 2)) += w_hat_ * sin(state_vec(0)) + w_hat_ * w_hat_ * (1 - cos(state_vec(0)));
		return ret;
	}
	using ConnectorConstraint::setState;
	void setState(float new_state) {
		setState(Eigen::Vector<float, 1>(new_state));
	}

	RotationJoint(Eigen::Vector3f axis) :
		ConnectorConstraint(),
		axis_(axis),
		w_hat_(makeSkewMatrix(axis)) {
	}
	/*
	RotationJoint(const Eigen::Matrix4f* root_transform,Eigen::Vector3f axis) :
		ConnectorConstraint(root_transform),
		axis_(axis),
		w_hat_(makeSkewMatrix(axis)) {
	}*/


};

class OffsetConnector : public ConnectorConstraint<0> {
public:
	OffsetConnector(const Eigen::Matrix4f& root_position, Eigen::Matrix4f initial_child_position) :
		ConnectorConstraint(root_position.inverse()* initial_child_position) {
		setRootTransform(&root_position);
	}
	OffsetConnector(Eigen::Matrix4f offset) :
		ConnectorConstraint(offset) {
	}

	OffsetConnector(float x, float y, float z) :
		ConnectorConstraint((Eigen::Matrix4f() << 1., 0., 0., x,
												  0., 1., 0., y,
												  0., 0., 1., z,
												  0., 0., 0., 1).finished()) {

	}
	OffsetConnector(Eigen::Vector3f offset_global, Eigen::Vector3f prev_offset_global):
		OffsetConnector(offset_global(0)-prev_offset_global(0),
						offset_global(1) - prev_offset_global(1),
						offset_global(2) - prev_offset_global(2)){
	}
};

class CartesianJoint : public ConnectorConstraint<3> {
public:
	Eigen::Matrix4f computeConnectorTransform(Eigen::Vector<float, 3> state_vec) const override {
		Eigen::Matrix4f ret = Eigen::Matrix4f::Identity();
		ret(seq(0, 2), 3) = state_vec;
		return ret;
	}
};

class BallJoint : public ConnectorConstraint<3> {
private:

public:
	Eigen::Matrix4f computeConnectorTransform(Eigen::Vector<float, 3> state_vec) const override {
		Eigen::Matrix4f ret;
		const float& alpha = state_vec(0);
		const float& beta = state_vec(1);
		const float& gamma = state_vec(2);

		float c1 = cos(alpha);
		float c2 = cos(beta);
		float c3 = cos(gamma);
		float s1 = sin(alpha);
		float s2 = sin(beta);
		float s3 = sin(gamma);
		switch (angle_order_) {
			case XZX:
				ret << c2, -c3 * s2, s2* s3, 0,
					c1* s2, c1* c2* c3 - s1 * s3, -c3 * s1 - c1 * c2 * s3, 0,
					s1* s2, c1* s3 + c2 * c3 * s1, c1* c3 - c2 * s1 * s3, 0,
					0, 0, 0, 1;
				break;
			case XYX:
				ret << c2, s2* s3, c3* s2, 0,
					s1* s2, c1* c3 - c2 * s1 * s3, -c1 * s3 - c2 * c3 * s1, 0,
					-c1 * s2, c3* s1 + c1 * c2 * s3, c1* c2* c3 - s1 * s3, 0,
					0, 0, 0, 1;
				break;
			case YXY:
				ret << c1*c3 - c2*s1*s3, s1*s2, c1*s3 + c2*c3*s1, 0,
					s2*s3, c2, -c3*s2, 0,
					-c3*s1 - c1*c2*s3, c1*s2, c1*c2* c3 - s1*s3, 0,
					0, 0, 0, 1;
				break;
			case YZY:
				ret << c1 * c2 * c3 - s1 * s3, -c1 * s2, c3* s1 + c1 * c2 * c3, 0,
					c3* s2, c2, s2* s3, 0,
					-c1 * s3 - c2 * c3 * s1, s1* s2, c1* c3 - c2 * s1 * s3, 0,
					0, 0, 0, 1;
				break;
			case ZYZ:
				ret << c1 * c2 * c3 - s1 * s3, -c3 * s1 - c1 * c2 * s3, c1* s2, 0,
					c1* s3 + c2 * c3 * s1, c1* c3 - c2 * s1 * s3, s1* s2, 0,
					-c3 * s2, s2* s3, c2, 0,
					0, 0, 0,1;
				break;
			case ZXZ:
				ret << c1*c3 - c2*s1*s3, -c1*s3-c2*c3*s1, s1*s2, 0,
						c3*s1 + c1*c2*s3, c1*c2*c3 - s1*s3, -c1*s2, 0,
						s2*s3, c3*s2, c2, 0,
						0,0,0,1;
				break;
			case XZY:
				ret <<	c2*c3, -s2, c2*s3, 0,
						s1*s3+c1*c3*s2, c1*c2, c1*s2*s3 - c3*s1, 0,
						c3*s1*s2 - c1*s3, c2*s1, c1*c3+s1*s2*s3, 0,
						0,0,0,1;
				break;
			case XYZ:
				ret <<	c2*c3, -c2*s3, s2, 0,
						c1*s3 + c3*s1*s2, c1*c3 - s1*s2*s3, -c2*s1, 0,
						s1*s3 - c1*c3*s2, c3*s1 + c1*s2*s3, c1*c2, 0,
						0,0,0,1;
				break;
			case YXZ:
				ret << c1*c2 + s1*s2*s3, c3*s1*s2 - c1*s3, c2*s1, 0,
						c2*s3, c2*c3, -s2, 0,
						c1*s2*s3 - c3*s1, c1*c3*s2 + s1*s3, c1*c2, 0,
						0,0,0,1;
				break;
			case YZX:
				ret << c1*c2, s1*s3 - c1*c2*s2, c3*s1 + c1*s2*s3, 0,
						s2, c2*c3, -c2*s3, 0,
						-c2*s1, c1*s3 + c3*s1*s2, c1*c3 - s1*s2*s3, 0,
						0,0,0,1;
				break;
			case ZYX:
				ret << c1*c2, c1*s2*s2 - c3*s1, s1*s3 + c1*c3*s2, 0,
						c2*s1, c1*c3 + s1*s2*s3, c3*s1*s2 - c1*s3, 0,
						-s2, c2*s3, c2*c3, 0,
						0,0,0,1;
				break;
			case ZXY:
				ret << c1*c3 - s1*s2*s3, -c2*s1, c1*s3 + c3*s1*s2, 0,
						c3*s1 + c1*s2*s3, c1*c2, s1*s3 - c1*c3*s2, 0,
						-c2*s3, s2, c2*c3, 0,
						0,0,0,1;
				break;
		}
		return ret;
	}


	enum EulerAngleOrder_T { XYZ, XZY, XYX, XZX, YZX, YXZ, YXY, YZY, ZXY, ZYX, ZXZ, ZYZ };

	BallJoint(EulerAngleOrder_T angle_order) :
		ConnectorConstraint(),
		angle_order_(angle_order) {

	}


private:
	const EulerAngleOrder_T angle_order_;

};

//can this be vastly simplified to a linked list? i.e.
/*
* 
* template<int n_dofs>
* class ChainConstraint : public ConnectorConstraint<n_dofs>{
* 
* const ChainConstraint<n_dofs-1>* next;
* 
* }
* 
*/

template<Connector constraint>
consteval int sumDoF() {
	return constraint::getDoF();
}
template<Connector constraint1,Connector constraint2, Connector... constraints>
consteval int sumDoF() {
	return constraint1::getDoF() + sumDoF<constraint2,constraints...>();
}

template<Connector constraint, Connector...constraints>
class ConnectorChain : public ConnectorConstraint<sumDoF<constraint,constraints...>()> {
	std::tuple<constraint&, constraints&...> connectors_;

	static_assert(ConnectorChain::getDoF() == sumDoF<constraint, constraints...>());

protected:

	Eigen::Matrix4f computeConnectorTransform(Eigen::Vector<float, ConnectorChain::getDoF()> state_vec) const override {
		return computeChainTransform<constraint, constraints...>(state_vec);
	}
	template<Connector constraint_>
	Eigen::Matrix4f computeChainTransform(Eigen::Vector<float, constraint_::getDoF()> state_vec) const {
		constraint_& conn = std::get< sizeof...(constraints)>(connectors_); //get last connector
		const Eigen::Matrix4f& pop_last = conn.computeConnectorTransform(state_vec);
		return pop_last;
	}
	template<Connector constraint_,Connector constraint2_,Connector...constraints_>
	Eigen::Matrix4f computeChainTransform(Eigen::Vector<float, sumDoF<constraint_,constraint2_,constraints_...>()> state_vec) const {
		Eigen::Vector<float, constraint_::getDoF()> next_input = state_vec(seq(0, constraint_::getDoF() - 1));
		constraint_& pop_conn = std::get<sizeof...(constraints) - sizeof...(constraints_) - 1>(connectors_);
		const Eigen::Matrix4f& pop_front = pop_conn.computeConnectorTransform(next_input);
		return pop_front * computeChainTransform<constraint2_,constraints_...>(state_vec(seq(constraint_::getDoF(), Eigen::last)));
	}
	template<Connector constraint_>
	void setSubstate(Eigen::Vector<float, constraint_::getDoF()> state_vec) {
		constraint_& conn = std::get< sizeof...(constraints)>(connectors_); //get last connector
		conn.setState(state_vec);
	}
	template<Connector constraint_, Connector constraint2_, Connector...constraints_>
	void setSubstate(Eigen::Vector<float, sumDoF<constraint_, constraint2_, constraints_...>()> state_vec) {
		constraint_& pop_conn = std::get<sizeof...(constraints) - sizeof...(constraints_) - 1>(connectors_);
		pop_conn.setState(state_vec(seq(0, constraint_::getDoF() - 1)));
		setSubstate<constraint2_, constraints_...>(state_vec(seq(constraint_::getDoF(), Eigen::last)));
	}

	template<int start_index, Connector constraint_>
	void writeSubstate(Eigen::Vector<float, ConnectorChain::getDoF()>& state_vec) const {
		constraint_& conn = std::get< sizeof...(constraints)>(connectors_); //get last connector
		state_vec(seq(start_index, Eigen::last)) = conn.getState();
	}
	template<int start_index, Connector constraint_, Connector constraint2_, Connector...constraints_>
	void writeSubstate(Eigen::Vector<float, ConnectorChain::getDoF()>& state_vec) const {
		constraint_& pop_conn = std::get<sizeof...(constraints) - sizeof...(constraints_) - 1>(connectors_);
		state_vec(seq(start_index, start_index + constraint_::getDoF() - 1)) = pop_conn.getState();
		writeSubstate<start_index + constraint_::getDoF(), constraint2_, constraints_...>(state_vec);
	}

	template<Connector constraint_>
	void setSubconnectorRootTransforms() {
	}
	template<Connector constraint_, Connector constraint2_, Connector...constraints_>
	void setSubconnectorRootTransforms() {
		constraint_& root_conn = std::get<sizeof...(constraints) - sizeof...(constraints_) - 1>(connectors_);
		constraint2_& pop_conn = std::get<sizeof...(constraints) - sizeof...(constraints_)>(connectors_);
		pop_conn.setRootTransform(&root_conn.getEndTransform());
		setSubconnectorRootTransforms<constraint2_, constraints_...>();
	}

public:
	ConnectorChain(constraint& constraint0,constraints&... constraint1_end) : ConnectorConstraint< ConnectorChain::getDoF()>(), connectors_(constraint0, constraint1_end...) {
		setSubconnectorRootTransforms<constraint,constraints...>();
	}


	void setState(Eigen::Vector<float,ConnectorChain::getDoF()> new_state) override {
		//set child states using template magic
		setSubstate<constraint, constraints...>(new_state); //this can likely be sped up by combining these two calls
		ConnectorConstraint<ConnectorChain::getDoF()>::setState(new_state);
		/*issue is HERE the connector_transform_ is only recomputed here so we have to check that all children are 
		not stale before using connector_transform_ to get connector tform*/
		//connector_transform_ = computeConnectorTransform(new_state);
	}
	void setRootTransform(const Eigen::Matrix4f* root_transform) override {
		ConnectorConstraint<ConnectorChain::getDoF()>::setRootTransform(root_transform);
		std::get<0>(connectors_).setRootTransform(root_transform);
	}

	Eigen::Vector<float, ConnectorChain::getDoF()> getState() const override {
		Eigen::Vector<float,ConnectorChain::getDoF()> state;
		writeSubstate<0,constraint, constraints...>(state);
		//read from children using template magic
		return state;
	}

};

template<Connector constraint>
class ConnectorChain<constraint> : public constraint {};

/*
class ParametricCurveConstraint : public ConnectorConstraint<1> {

};
*/

#endif

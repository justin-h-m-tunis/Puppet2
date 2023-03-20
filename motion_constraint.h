#pragma once

#ifndef PUPPET_MOTIONCONSTRAINT
#define PUPPET_MOTIONCONSTRAINT

#include <Eigen/dense>
#include <string>
#include <iostream>
#include <unordered_set>

#include "Model.h"
using Eigen::seq;
//boundaryConstraint -> cant cross specified boundary, motion is adjusted to stay within bounds
//coupleConstraint -> motion is tied to parent motion except for set degrees of freedom

template<int n_dims>
class Surface {
public:
	virtual bool crossesSurface(Eigen::Vector<float,n_dims> first_state, Eigen::Vector<float,n_dims> second_state) const = 0;
};


template<int n_dims>
class Region : public Surface<n_dims> { //Region==bounded surface
public:
	virtual bool insideRegion(Eigen::Vector<float, n_dims> state) const = 0;

	virtual bool crossesSurface(Eigen::Vector<float, n_dims> first_state, Eigen::Vector<float, n_dims> second_state) const override {
		return insideRegion(first_state) != insideRegion(second_state);
	};

};

//hitbox is then a child of region
class MeshSurface : public Surface<3> {
	std::vector<Eigen::Vector3f> verts_;
	std::vector<std::pair<int, int>> edges_;

	virtual bool crossesSurface(Eigen::Vector<float, 3> first_state, Eigen::Vector<float, 3> second_state) const override {
		std::cerr << "not implemented to function as primary surface";
		return true;
	}
	
	struct edgeHasher {
		size_t operator()(const std::pair<int, int>& p) const {
			auto hash1 = std::hash<int>{}(p.first);
			auto hash2 = std::hash<int>{}(p.second);

			if (hash1 != hash2) {
				return hash1 ^ hash2;
			}

			// If hash1 == hash2, their XOR is zero.
			return hash1;
		}
	};

public:
	const std::vector<Eigen::Vector3f>& getVerts() const {
		return verts_;
	}
	const std::vector<std::pair<int, int>>& getEdges() const{
			return edges_;
	}

	explicit MeshSurface(const Model& model) {
		for (int i = 0; i < model.vlen(); i++) {
			verts_.emplace_back(model.getVerts()[3 * i],model.getVerts()[3 * i + 1],model.getVerts()[3 * i + 2]);
		}
		for (int i = 0; i < model.flen(); i++) {
			std::unordered_set<std::pair<int, int>,edgeHasher> edges;
			for (int first = 0; first < 3; first++) {
				for (int second = 0; second < 3; second++) {
					if (first == second) continue;
					std::pair<int, int>e(model.getFaces()[3 * i+first], model.getFaces()[3 * i + second]);
					if (!edges.contains(e)) {
						edges.emplace(e);
						edges_.push_back(e);
					}
				}
			}
		}
	}

	MeshSurface(std::string filename) : MeshSurface(Model(filename)) {

	}
};

class Ellipse : public Region<3> {//technially generalizable into N dimensions but not useful for making games lol
	const Eigen::Vector3f E_;
	const Eigen::Vector3f E_inv_sq_;
public:
	bool insideRegion(Eigen::Vector<float, 3> state) const {
		return state.array().pow<int>(2).matrix().transpose() * E_inv_sq_;
	};

};

template<class A, class B>
bool checkCollision(const A& PrimarySurf, const B& SecondarySurf, Eigen::Matrix4f PrimaryPosition, Eigen::Matrix4f SecondaryPosition) {
	//static_assert(false, "Not A Function");
	return true;
};

bool SurfaceNodeCollision(const Surface<3>& PrimarySurf, const MeshSurface& SecondarySurf, Eigen::Matrix4f SecondaryPosition) {
	Eigen::Matrix3f R = SecondaryPosition(seq(0, 2), seq(0, 2));
	Eigen::Vector3f p = SecondaryPosition(seq(0, 2), 3);
	for (auto& e : SecondarySurf.getEdges()) {
		if (PrimarySurf.crossesSurface(R*SecondarySurf.getVerts()[e.first]+p, R * SecondarySurf.getVerts()[e.second]+p)) {
			return true;
		}
	}
}

template<>
bool checkCollision<Surface<3>, MeshSurface>(const Surface<3>& PrimarySurf, const MeshSurface& SecondarySurf, const Eigen::Matrix4f PrimaryPosition,const Eigen::Matrix4f SecondaryPosition) {
	return SurfaceNodeCollision(PrimarySurf, SecondarySurf, PrimaryPosition.inverse() * SecondaryPosition);
}

template<>
bool checkCollision<Ellipse, Ellipse>(const Ellipse& PrimarySurf, const Ellipse& SecondarySurf, const Eigen::Matrix4f PrimaryPosition, const Eigen::Matrix4f SecondaryPosition) {
	return true;
}

class BoundaryConstraint{


	Eigen::Vector3f limitTranslate(Eigen::Vector3f position, Eigen::Vector3f delta_pos, int n_iters = 5) const {
		//Eigen::Vector3f delta_pos = first_guess - position;
		float delta_norm = delta_pos.norm();
		float len_ratio = 1.;
		Eigen::Vector3f longest_vec = Eigen::Vector3f::Zero();
		for (int i = 0; i < n_iters; i++) {
			if (boundary_->crossesSurface(position, position + longest_vec + len_ratio * delta_pos )) {
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
	const Surface<3>* boundary_;

	virtual bool isValidPosition(Eigen::Matrix4f position) const { return true; };

public:
	bool breaksConstraint(Eigen::Matrix4f old_tform, Eigen::Matrix4f new_tform) const { 
		return !isValidPosition(new_tform) || boundary_->crossesSurface(old_tform(seq(0,2),3), new_tform(seq(0,2),3) - old_tform(seq(0, 2), 3));
	}

	virtual Eigen::Vector3f bestTranslate(Eigen::Vector3f current, Eigen::Vector3f delta_pos, Eigen::Vector3f normal, Eigen::Vector3f binormal) const {
			//Eigen::Matrix3f delta_pos = target(seq(0, 2), 3) - current(seq(0, 2), 3);
		float delta_norm = delta_pos.norm();
		Eigen::Vector3f fwd = limitTranslate(current, delta_pos);
		float fwd_ratio = fwd.norm() / delta_pos.norm();
		if (fwd_ratio == 1.) {
			return fwd;
		}
		Eigen::Vector3f next = current + fwd;
		Eigen::Vector3f pos_norm = limitTranslate(next, (1 - fwd_ratio) * delta_norm * normal);
		Eigen::Vector3f neg_norm = limitTranslate(next, -(1 - fwd_ratio) * delta_norm * normal);
		Eigen::Vector3f pos_binorm = limitTranslate(next, (1 - fwd_ratio) * delta_norm * binormal);
		Eigen::Vector3f neg_binorm = limitTranslate(next, -(1 - fwd_ratio) * delta_norm * binormal);
		return fwd + (pos_norm + neg_norm) + (pos_binorm + neg_binorm);
	}

	BoundaryConstraint() : boundary_(nullptr) {}

	BoundaryConstraint(const Surface<3>* boundary) : boundary_(boundary){}

	void setBoundary(const Surface<3>* boundary) {
		boundary = boundary_;
	}
	// this should be overwritten in the case where both old and new are valid but the movement from one to the other is impossible
	//virtual bool breaksConstraint(Eigen::Vector3f current_pos, Eigen::Vector3f delta_pos) { return true; }// = 0;
	
	//virtual Eigen::Matrix4f getConstrainedTwist(Eigen::Matrix4f current_tform, Eigen::Matrix4f delta_tform) const { return Eigen::Matrix4f::Identity(); }// = 0;
	//virtual Eigen::Vector3f getConstrainedTranslate(Eigen::Vector3f current_pos, Eigen::Vector3f delta_pos) const { return Eigen::Vector3f::Zero(); }// = 0;
};
//all motion constraint 

template<class primary>
concept PrimaryHitbox = std::derived_from<primary, Surface<3>>;

template<class secondary, class primary>
concept SecondaryHitbox = requires(const primary & first, const secondary & second, Eigen::Matrix4f G1, Eigen::Matrix4f G2) {
	checkCollision<primary, secondary>(first, second, G1, G2);
};

template<PrimaryHitbox primary_type,SecondaryHitbox<primary_type> secondary_type>
class NoCollideConstraint : public BoundaryConstraint {

	const primary_type* primary_hitbox_;
	const Eigen::Matrix4f* primary_position_;
	const secondary_type& secondary_hitbox_;
	const Eigen::Matrix4f& secondary_position_;

public:
	virtual bool isValidPosition(Eigen::Matrix4f old_tform, Eigen::Matrix4f new_tform) const {
		return checkCollision(primary_hitbox_, secondary_hitbox_, *primary_position_, new_tform);
	}
	
	NoCollideConstraint(const secondary_type& secondary_hitbox, const Eigen::Matrix4f& secondary_position) :
		primary_hitbox_(nullptr),
		primary_position_(nullptr),
		secondary_hitbox_(secondary_hitbox),
		secondary_position_(secondary_position){
	}

};

class PositionConstraint {
	//MotionConstraint* bounds_;
protected:
	const Eigen::Matrix4f* root_transform_;
public:
	virtual const Eigen::Matrix4f& getConstraintTransform() const {
		return Eigen::Matrix4f::Identity();
	};
	//PositionConstraint():bounds_(nullptr){}

	void setRootTransform(const Eigen::Matrix4f* root_transform) {
		root_transform_ = root_transform;
	}

};

template<class T>
concept Connector = std::derived_from<T, PositionConstraint> && requires{
	T::getDoF();
};

template<int n_dofs>
class ConnectorConstraint : public PositionConstraint {

	Eigen::Vector<float, n_dofs> state_;
	Eigen::Vector<float, n_dofs> lower_bounds;
	Eigen::Vector<float, n_dofs> upper_bounds;

	Eigen::Matrix4f connector_transform_;

	//this must be a member of ConnectorConstraint and not simply a new type of connector
	//is because you can only ever have one of these per chain transform
	//its possible to make an "observer" derived class which has two connection points and "observes" the inverse kinematics

	//has unintuitive behavior if called publically
	void boundedMove(Eigen::Vector<float, n_dofs> new_state, const BoundaryConstraint& bounds, int n_iters) {
		if (n_iters == 0) {
			return;
		} else {
			Eigen::Matrix4f new_transform = computeConnectorTransform(new_state);
			Eigen::Vector<float, n_dofs> delta_state = new_state - state_;
			bool breaks_constraint;
			if (root_transform_ != nullptr) {
				breaks_constraint = bounds.breaksConstraint(*root_transform_ * getConstraintTransform(), *root_transform_ * new_transform);
			}
			else {
				breaks_constraint = bounds.breaksConstraint(getConstraintTransform(), new_transform);
			}
			if (!breaks_constraint) {
				setState(new_state);
			}
			boundedMove(state_ + delta_state / 2, bounds, n_iters - 1);
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
		connector_transform_(Eigen::Matrix4f::Identity()){

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

	void setState(Eigen::Vector<float,n_dofs> new_state) {
		state_ = new_state;
		connector_transform_ = computeConnectorTransform(new_state);
	}

	const Eigen::Vector<float, n_dofs>& getState() const {
		return state_;
	}

	template<int max_iters = 5>
	void boundedMove(Eigen::Vector<float, n_dofs> new_state, const BoundaryConstraint& bounds) {
		Eigen::Matrix4f new_transform = computeConnectorTransform(new_state);
		bool breaks_constraint;
		if (root_transform_ != nullptr) {
			breaks_constraint = bounds.breaksConstraint(*root_transform_ * getConstraintTransform(), *root_transform_ * new_transform);
		}
		else {
			breaks_constraint = bounds.breaksConstraint(getConstraintTransform(), new_transform);
		}
		if (breaks_constraint) {
			Eigen::Vector<float, n_dofs> delta_state = new_state - state_;
			boundedMove(state_ + delta_state / 2, bounds, max_iters);
		} else {
			setState(new_state);
		}
	}


};

template <>
class ConnectorConstraint<0> : public PositionConstraint {
private:
	const Eigen::Matrix4f connector_transform_;
	const Eigen::Matrix4f* root_transform_;

protected:
public:
	static constexpr int getDoF() { return 0; };

	ConnectorConstraint(Eigen::Matrix4f offset) :
		connector_transform_(offset),
		root_transform_(nullptr){

	}
	ConnectorConstraint(const Eigen::Matrix4f* root_transform,Eigen::Matrix4f offset) :
		connector_transform_(offset),
		root_transform_(root_transform){

	}

	const Eigen::Matrix4f& getConstraintTransform() const override {
		//i think child position needs to be inverted?
		return connector_transform_;
		//probably not correct kinematics lol
	}

	virtual Eigen::Matrix4f computeConnectorTransform(Eigen::Vector<float, 0> state_vec) {
		return connector_transform_;
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

	//this shouldnt be necessary
	/*template<int i>
	consteval auto getConnector() const {
		return std::get<i>(connectors_);
	}*/
	static_assert(ConnectorChain::getDoF() == sumDoF<constraint, constraints...>());

protected:

	Eigen::Matrix4f computeConnectorTransform(Eigen::Vector<float, ConnectorChain::getDoF()> state_vec) const override {
		return computeChainTransform<constraint, constraints...>(state_vec);
	}
	template<Connector constraint_>
	Eigen::Matrix4f computeChainTransform(Eigen::Vector<float, constraint_::getDoF()> state_vec) const {
		constraint_ conn = std::get< sizeof...(constraints)>(connectors_); //get last connector
		Eigen::Matrix4f pop_last = conn.computeConnectorTransform(state_vec);
		return pop_last;
	}
	template<Connector constraint_,Connector constraint2_,Connector...constraints_>
	Eigen::Matrix4f computeChainTransform(Eigen::Vector<float, sumDoF<constraint_,constraint2_,constraints_...>()> state_vec) const {
		Eigen::Vector<float, constraint_::getDoF()> next_input = state_vec(seq(0, constraint_::getDoF() - 1));
		constraint_ pop_conn = std::get<sizeof...(constraints) - sizeof...(constraints_) - 1>(connectors_);
		Eigen::Matrix4f pop_front = pop_conn.computeConnectorTransform(next_input);
		return pop_front * computeChainTransform<constraint2_,constraints_...>(state_vec(seq(constraint_::getDoF(), Eigen::last)));
	}

public:
	ConnectorChain(constraint& constraint0,constraints&... constraint1_end) : ConnectorConstraint< ConnectorChain::getDoF()>(), connectors_(constraint0, constraint1_end...) {
		
	}
	/*
	ConnectorChain(const Eigen::Matrix4f* root_transform,constraint& constraint0, constraints&... constraint1_end) :
		ConnectorConstraint< ConnectorChain::getDoF()>(root_transform), connectors_(constraint0, constraint1_end...) {

	}*/

};

template<Connector constraint>
class ConnectorChain<constraint> : public constraint {};

/*
class ParametricCurveConstraint : public ConnectorConstraint<1> {

};
*/

#endif

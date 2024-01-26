#pragma once

#ifndef PUPPET_CONSTRAINT_NOCOLLIDE
#define PUPPET_CONSTRAINT_NOCOLLIDE

#include "collision.hpp"

template<PrimaryHitbox primary_type, SecondaryHitbox<primary_type> secondary_type>
class NoCollideConstraint : public BoundaryConstraint {

	const secondary_type* secondary_hitbox_;

protected:
	bool invalidPosition(Eigen::Matrix4f position) const override {
		if (getBoundary() != nullptr && secondary_hitbox_ != nullptr) {
			return CollisionPair<Surface<3>, secondary_type>::checkCollision(*getBoundary(), *secondary_hitbox_, *getBoundaryPosition(), position);
		}
		else {
			return false;
		}
	}

public:

	NoCollideConstraint(const secondary_type* secondary_hitbox) :
		secondary_hitbox_(secondary_hitbox) {
	}
	NoCollideConstraint(const primary_type* primary_hitbox, const Eigen::Matrix4f* primary_position, const secondary_type* secondary_hitbox) :
		BoundaryConstraint(primary_hitbox, primary_position),
		secondary_hitbox_(secondary_hitbox) {
	}

};

#endif


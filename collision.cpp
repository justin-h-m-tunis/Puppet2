#include "collision.hpp"

template<class A, class B>
bool checkCollision(const A* PrimarySurf, const B* SecondarySurf, Eigen::Matrix4f PrimaryPosition, Eigen::Matrix4f SecondaryPosition,Eigen::Matrix4f secondary_motion) {
	//static_assert(false, "Not A Function");
	std::cerr << "cannot call generic collision function!" << "\n";
	return true;
};

bool SurfaceNodeCollision(const Surface<3>* PrimarySurf, const MeshSurface* SecondarySurf, Eigen::Matrix4f SecondaryPosition) {
	Eigen::Matrix3f R = SecondaryPosition(seq(0, 2), seq(0, 2));
	Eigen::Vector3f p = SecondaryPosition(seq(0, 2), 3);
	for (auto& e : SecondarySurf->getEdges()) {
		//if (PrimarySurf->crossesSurface(R*SecondarySurf->getVerts()[e.first]+p, R*SecondarySurf->getVerts()[e.second]+p)) {
		//should at least let the user choose is the mesh is irrotational?
		if (PrimarySurf->crossesSurface(R*SecondarySurf->getVerts()[e.first] + p, R*SecondarySurf->getVerts()[e.second] + p)) {
			return true;
		}
	}
	return false;
}
bool SurfaceNodeCollision(const Surface<3>* PrimarySurf, const MeshSurface* SecondarySurf, Eigen::Matrix4f SecondaryPosition, CollisionInfo<Surface<3>, MeshSurface>* collision_info) {
	//not fully implemented!
	Eigen::Matrix3f R = SecondaryPosition(seq(0, 2), seq(0, 2));
	Eigen::Vector3f p = SecondaryPosition(seq(0, 2), 3);
	for (auto& e : SecondarySurf->getEdges()) {
		//if (PrimarySurf->crossesSurface(R*SecondarySurf->getVerts()[e.first]+p, R*SecondarySurf->getVerts()[e.second]+p)) {
		//should at least let the user choose is the mesh is irrotational?
		if (PrimarySurf->crossesSurface(R * SecondarySurf->getVerts()[e.first] + p, R * SecondarySurf->getVerts()[e.second] + p)) {
			return true;
		}
	}
	return false;
}

bool SurfaceNodeCollision(const MeshSurface* PrimarySurf, const MeshSurface* SecondarySurf, Eigen::Matrix4f SecondaryPosition) {
	Eigen::Matrix3f R = SecondaryPosition(seq(0, 2), seq(0, 2));
	Eigen::Vector3f p = SecondaryPosition(seq(0, 2), 3);
	for (auto& e : SecondarySurf->getEdges()) {
		//if (PrimarySurf->crossesSurface(R*SecondarySurf->getVerts()[e.first]+p, R*SecondarySurf->getVerts()[e.second]+p)) {
		//should at least let the user choose is the mesh is irrotational?
		if (PrimarySurf->crossesSurface(R * SecondarySurf->getVerts()[e.first] + p, R * SecondarySurf->getVerts()[e.second] + p)) {
			return true;
		}
	}
	return false;
}

bool SurfaceNodeCollision(const MeshSurface* PrimarySurf, const MeshSurface* SecondarySurf, Eigen::Matrix4f SecondaryPosition, CollisionInfo<MeshSurface,MeshSurface>* collision_info) {
	Eigen::Matrix3f R = SecondaryPosition(seq(0, 2), seq(0, 2));
	Eigen::Vector3f p = SecondaryPosition(seq(0, 2), 3);
	bool result = false;
	int i = 0;
	for (auto& e : SecondarySurf->getEdges()) {
		//if (PrimarySurf->crossesSurface(R*SecondarySurf->getVerts()[e.first]+p, R*SecondarySurf->getVerts()[e.second]+p)) {
		//should at least let the user choose is the mesh is irrotational?
		if (collision_info->getEdgeInfo_const().size() <= i) {
			collision_info->getEdgeInfo().emplace_back();
		}
		float* collision_loc = &collision_info->getEdgeInfo()[i].collision_location;
		//std::vector<int>* collision_faces = &collision_info->getEdgeInfo()[i].collision_faces;
		collision_info->getEdgeInfo()[i].is_colliding = PrimarySurf->crossesSurface(R * SecondarySurf->getVerts()[e.first] + p, R * SecondarySurf->getVerts()[e.second] + p,collision_loc);
		if (result == false && collision_info->getEdgeInfo()[i].is_colliding) {
			result = true;
		}
		i++;
	}
	collision_info->is_colliding = result;
	return result;
}

template<>
bool checkCollision<Surface<3>, MeshSurface>(const Surface<3>* PrimarySurf, const MeshSurface* SecondarySurf, const Eigen::Matrix4f PrimaryPosition, const Eigen::Matrix4f SecondaryPosition, const Eigen::Matrix4f secondary_motion) {
	Eigen::Matrix4f secondary_last_position = secondary_motion.inverse() * SecondaryPosition;
	Eigen::Matrix4f secondary_relative_position = PrimaryPosition.inverse() * SecondaryPosition;
	Eigen::Matrix4f secondary_relative_last_position = PrimaryPosition.inverse() * secondary_last_position;
	Eigen::Matrix3f R_f = secondary_relative_position(seq(0, 2), seq(0, 2));
	Eigen::Vector3f p_f = secondary_relative_position(seq(0, 2), 3);
	Eigen::Matrix3f R_i = secondary_relative_last_position(seq(0, 2), seq(0, 2));
	Eigen::Vector3f p_i = secondary_relative_last_position(seq(0, 2), 3);
	for (auto& e : SecondarySurf->getEdges()) {
		//if (PrimarySurf->crossesSurface(R*SecondarySurf->getVerts()[e.first]+p, R*SecondarySurf->getVerts()[e.second]+p)) {
		//should at least let the user choose is the mesh is irrotational?
		if (PrimarySurf->crossesSurface(R_f * SecondarySurf->getVerts()[e.first] + p_f, R_f * SecondarySurf->getVerts()[e.second] + p_f)) {
			return true;
		}
	}
	for (const Eigen::Vector3f& v : SecondarySurf->getVerts()) {
		if (PrimarySurf->crossesSurface(R_f * v + p_f, R_i * v + p_i)) {
			return true;
		}
	}
	return false;
}

template<>
bool checkCollision<MeshSurface, MeshSurface>(const MeshSurface* PrimarySurf, const MeshSurface* SecondarySurf, const Eigen::Matrix4f PrimaryPosition, const Eigen::Matrix4f SecondaryPosition, const Eigen::Matrix4f secondary_motion) {
	Eigen::Matrix4f secondary_last_position = secondary_motion.inverse() * SecondaryPosition;
	Eigen::Matrix4f secondary_relative_position = PrimaryPosition.inverse() * SecondaryPosition;
	Eigen::Matrix4f secondary_relative_last_position = PrimaryPosition.inverse() * secondary_last_position;
	Eigen::Matrix3f R_f = secondary_relative_position(seq(0, 2), seq(0, 2));
	Eigen::Vector3f p_f = secondary_relative_position(seq(0, 2), 3);
	Eigen::Matrix3f R_i = secondary_relative_last_position(seq(0, 2), seq(0, 2));
	Eigen::Vector3f p_i = secondary_relative_last_position(seq(0, 2), 3);
	for (auto& e : SecondarySurf->getEdges()) {
		//if (PrimarySurf->crossesSurface(R*SecondarySurf->getVerts()[e.first]+p, R*SecondarySurf->getVerts()[e.second]+p)) {
		//should at least let the user choose is the mesh is irrotational?
		if (PrimarySurf->crossesSurface(R_f * SecondarySurf->getVerts()[e.first] + p_f, R_f * SecondarySurf->getVerts()[e.second] + p_f)) {
			return true;
		}
	}
	for (const Eigen::Vector3f& v : SecondarySurf->getVerts()) {
		if (PrimarySurf->crossesSurface(R_f * v + p_f, R_i * v + p_i)) {
			return true;
		}
	}
	return false;
}

template<>
bool checkCollision<Ellipse, Ellipse>(const Ellipse* PrimarySurf, const Ellipse* SecondarySurf, const Eigen::Matrix4f PrimaryPosition, const Eigen::Matrix4f SecondaryPosition, const Eigen::Matrix4f secondary_motion) {
	return true;
}


template<>
void getFullCollision<MeshSurface, MeshSurface>(const MeshSurface* PrimarySurf, const MeshSurface* SecondarySurf, const Eigen::Matrix4f PrimaryPosition, const Eigen::Matrix4f SecondaryPosition, Eigen::Matrix4f secondary_motion, CollisionInfo<MeshSurface, MeshSurface>* collision_info){
	SurfaceNodeCollision(PrimarySurf, SecondarySurf, PrimaryPosition.inverse() * SecondaryPosition, collision_info);
}

template<>
void getFullCollision<Surface<3>, MeshSurface>(const Surface<3>* PrimarySurf, const MeshSurface* SecondarySurf, const Eigen::Matrix4f PrimaryPosition, const Eigen::Matrix4f SecondaryPosition, Eigen::Matrix4f secondary_motion, CollisionInfo<Surface<3>, MeshSurface>* collision_info) {
	SurfaceNodeCollision(PrimarySurf, SecondarySurf, PrimaryPosition.inverse() * SecondaryPosition, collision_info);
}

/*bool triIntersection(Eigen::Vector3f l1, Eigen::Vector3f l2, Eigen::Vector3f t1, Eigen::Vector3f t2, Eigen::Vector3f t3) {
	Eigen::Vector3f tri_norm = (t1-t2).cross(t3 - t1).normalized();
	//(k(l2-l1)+l1)*tri_norm = 0
	float k = l1.dot(tri_norm) / (l2 - l1).dot(tri_norm);
	if (k > 1 || k < 0) { //line is too "short" to intersect triangle
		return false;
	} else {
		Eigen::Vector3f p = k * (l1 - l2) + l1;
		//...
	}
	return true;

}*/

//deterministic method
/*not necessary sinve crossesSurface is implemented as a virtual function in meshbox
template<>
bool checkCollision<MeshSurface, MeshSurface>(const MeshSurface* PrimarySurf, const MeshSurface* SecondarySurf, const Eigen::Matrix4f PrimaryPosition, const Eigen::Matrix4f SecondaryPosition) {
	Eigen::Matrix4f relative_position = PrimaryPosition.inverse() * SecondaryPosition;
	Eigen::Matrix3f R = relative_position(seq(0, 2), seq(0, 2));
	Eigen::Vector3f p = relative_position(seq(0, 2), 3);
	for (auto& e : SecondarySurf->getEdges()) {
		//if (PrimarySurf->crossesSurface(R*SecondarySurf->getVerts()[e.first]+p, R*SecondarySurf->getVerts()[e.second]+p)) {
		//should at least let the user choose is the mesh is irrotational?
		if (PrimarySurf->crossesSurface(SecondarySurf->getVerts()[e.first] + p, SecondarySurf->getVerts()[e.second] + p)) {
			return true;
		}
	}
	return false;
}
*/

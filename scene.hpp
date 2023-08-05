#pragma once

#ifndef PUPPET_SCENE
#define PUPPET_SCENE

#include <Eigen\Dense>
#include <camera.h>
struct Scene {

	struct light {
		float brightness;
		Eigen::Vector3f color;
		Eigen::Vector3f position;

	};

	struct orthogonalLight : public light {

		Eigen::Vector3f direction_;

	};

	std::vector<const light*> lights;
	const orthogonalLight* shadow_light;
	const Camera* camera;
	Eigen::Vector3f atmosphere_color;
	float atmosphere_strength;

	Scene() : shadow_light(nullptr),camera(nullptr),atmosphere_color(Eigen::Vector3f::Zero()),atmosphere_strength(0){
	}



};



#endif
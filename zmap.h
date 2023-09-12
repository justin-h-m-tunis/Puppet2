#pragma once

#ifndef PUPPET_GRAPHICS_ZMAP
#define PUPPET_GRAPHICS_ZMAP

#include <vector>

#include "zdata.hpp"
#include "Model.h"
#include "motion_constraint.h"
#include "GameObject.h"


class Zmap : public Region<3>{//we can even encode room neighbor information by color coding the out of bounds information
private:

	const int x_resolution_, y_resolution_;
	const float map_width_, map_height_;
	std::vector<std::vector<zdata>> raw_data_;//array data is {z height, x slope, y slope}, as per the ZMapper shader
	float xy_padding_;
	//should really be an ordered vector
	//const zdata background_;

	static float quad_interpolate(float x, float y, float z11, float z12, float z21, float z22, float quad_side_len) {
		// 0 < x < quad_side_len
		// 0 < y < quad_side_len
		// z11 = z(x-,y-)
		// z21 = z(x+,y-)
		// z12 = z(x-,y+)
		// z22 = z(x+,y+)

		if (x > y) {
			//Triangle abc
			Eigen::Vector3f a = Eigen::Vector3f(0, z11, 0);
			Eigen::Vector3f b = Eigen::Vector3f(quad_side_len, z21, 0);
			Eigen::Vector3f c = Eigen::Vector3f(quad_side_len, z11, quad_side_len);
			Eigen::Vector3f plane = (b-a).cross(b-c).normalized();
			// (Eigen::Vector3f(x,ret,y) - b).plane = 0
			// (x-b_x)*plane_x + (ret - b_y)*plane_y + (y-b_z)*plane_z = 0
			// (ret - b_y)*plane_y = (b_x-x)*plane_x  + (b_z - y)*plane_z
			// ret = ((b_x-x)*plane_x  + (b_z - y)*plane_z)/plane_y + b_y
			return ((b(0) - x) * plane(0) + (b(2) - y) * plane(2)) / plane(1) + b(1);
		} else {
			//Triangle abc
			Eigen::Vector3f a = Eigen::Vector3f(0, z11, 0);
			Eigen::Vector3f b = Eigen::Vector3f(quad_side_len, z12, 0);
			Eigen::Vector3f c = Eigen::Vector3f(quad_side_len, z11, quad_side_len);
			Eigen::Vector3f plane = (b - a).cross(b - c).normalized();
			return ((b(0) - x) * plane(0) + (b(2) - y) * plane(2)) / plane(1) + b(1);
		}

	}

	void addLayer(std::vector<uint8_t> frame_data, float frame_z, float z_step) {
		int data_resolution = 255;
		float voxel_height = z_step / data_resolution;
		for (int i = 0; i < frame_data.size(); i += 4) { //4 channel
			//reads raw output from fbo as vector of uints and turns it into a zdata which is usefull so we dont have to memorize order of data in fbo
			//zdata pixel_i({ z_step * static_cast<float>(frame_data[i]) / data_resolution + frame_z - z_step,static_cast<float>(frame_data[i + 2]) / data_resolution,static_cast<float>(frame_data[i + 3]) / data_resolution }, frame_data[i + 1]);
			zdata pixel_i({ z_step * static_cast<float>(frame_data[i]) / data_resolution + frame_z - z_step,0,0}, frame_data[i + 1]);
			if ((pixel_i.room_id != 0) && (raw_data_[i / 4].size() == 0 || (abs(raw_data_[i / 4].back().z - pixel_i.z) > voxel_height))) {
				raw_data_[i / 4].push_back(pixel_i);
			}
		}
	}

protected:
	/*
	bool breaksConstraint(Eigen::Matrix4f current_tform, Eigen::Matrix4f new_tform) const override {
		//check for room_id,max_step,gap_width change in xy and getZdata.first change in z
		if (getZdata(new_tform(seq(0, 2), 3), 0).first.room_id == zdata::BaseRoom) {
			return true;
		}
		return false;
	};
	*/

	bool crossesSurface(Eigen::Vector<float, 3> first_state, Eigen::Vector<float, 3> second_state) const override {
		Eigen::Vector3f move_xy = second_state;
		move_xy(1) = first_state(1);
		Eigen::Vector3f move_z = first_state;
		move_z(1) = second_state(1);
		if (getZdata(second_state, 0).first.room_id == zdata::BaseRoom || getZdata(first_state, 0).first.room_id == zdata::BaseRoom) {
			return true;
		} else if(getZdata(second_state, 0).first != getZdata(move_xy, 0).first){
			return true;
		} else if (getZdata(first_state, 0).first != getZdata(move_z, 0).first) {
			return true;
		}
		return false;
	}

public:
	Zmap(int y_resolution, int x_resolution, float map_height, float map_width, float xy_padding) :
		x_resolution_(x_resolution),
		y_resolution_(y_resolution),
		map_height_(map_height*(1.+xy_padding)),
		map_width_(map_width*(1.+xy_padding)),
		xy_padding_(xy_padding)
	{
		for (int i = 0; i < x_resolution * y_resolution; i++) {
			raw_data_.emplace_back();
		}
	}

	Zmap(int y_resolution, int x_resolution, const Model* model) :
		Zmap(y_resolution, x_resolution, model->getBoundingBox()[2], model->getBoundingBox()[0], .1) {

	}

	//this overload doesnt work for some reason?
	/*
	std::pair<zdata, zdata> getZdata(Eigen::Matrix4f position, float step_height) const {
		return getZdata(Eigen::Vector3f(position(seq(0, 2), 3)), step_height);
	}*/
	std::pair<zdata, zdata> getZdataDiscrete(float z, int x_ind, int y_ind, float step_height = 0) const {

		//int x_ind = static_cast<int>(this->x_resolution_ * (position(0) / map_width_ + .5));
		//int y_ind = static_cast<int>(this->y_resolution_ * (position(2) / map_height_ + .5));
		
		if (x_ind < x_resolution_ && x_ind >= 0 && y_ind < y_resolution_ && y_ind >= 0) {
			//std::cout << "\nx pos: " << position(0) << ", y pos: " << position(2) << "x_ind: " << x_ind << ", y ind: " << y_ind << " index: "<<static_cast<size_t>(y_ind * x_resolution_ + x_ind);
			const std::vector<zdata>& xy_column = raw_data_[static_cast<size_t>(y_ind * x_resolution_ + x_ind)];
			if (xy_column.size() != 0) {
				auto zdata_below = xy_column.begin();
				auto zdata_above = zdata_below + 1;
				while (zdata_above != xy_column.end()) {
					if (zdata_above->z > z + step_height && zdata_below->z < z + step_height) {
						return std::pair<zdata, zdata>(*zdata_below, *zdata_above); //this can be improved for performance
					}
					//zdata_below = zdata_above;
					zdata_below++;
					zdata_above++;
				}
				if ((xy_column.end() - 1)->z < z + step_height) {
					return std::pair<zdata, zdata>(*zdata_below, zdata::ceiling());
				}
				else {
					return std::pair<zdata, zdata>(zdata::floor(), *zdata_below);
				}
			}
			//std::cout << "no entries( " << x_ind << ", " << y_ind << " )\n";
			return std::pair<zdata, zdata>(zdata::floor(), zdata::ceiling());
		}
		else {
			//std::cout << "out of bounds ( " << x_ind << ", " << y_ind << " )\n";
			return std::pair<zdata, zdata>(zdata::floor(), zdata::ceiling());
		}
	}

	std::pair<zdata, zdata> getZdata(Eigen::Vector3f position, float step_height) const {

		float x_pos = static_cast<float>(this->x_resolution_) * (position(0) / map_width_ + .5);
		float y_pos = static_cast<float>(this->y_resolution_) * (position(2) / map_height_ + .5);
		int x_ind_min = static_cast<int>(x_pos);
		int x_ind_max = static_cast<int>(x_pos) + 1;
		int y_ind_min = static_cast<int>(y_pos);
		int y_ind_max = static_cast<int>(y_pos) + 1;
		float x_remainder = x_pos - x_ind_min;
		float y_remainder = y_pos - y_ind_min;
		auto z11 = getZdataDiscrete(position(1), x_ind_min, y_ind_min, step_height);
		auto z21 = getZdataDiscrete(position(1), x_ind_max, y_ind_min, step_height);
		auto z12 = getZdataDiscrete(position(1), x_ind_min, y_ind_max, step_height);
		auto z22 = getZdataDiscrete(position(1), x_ind_max, y_ind_max, step_height);

		float z_below = quad_interpolate(x_remainder, y_remainder, z11.first.z, z21.first.z, z12.first.z, z22.first.z, 1.);
		float z_above = quad_interpolate(x_remainder, y_remainder, z11.second.z, z21.second.z, z12.second.z, z22.second.z, 1.);


		return std::pair<zdata, zdata>(zdata({ z_below,0,0 }, z11.first.room_id), zdata({ z_above,0,0 }, z11.second.room_id));
	}


	int getRoom(Eigen::Vector3f position) const {
		return getZdata(position, 0).first.room_id;
	}

	Eigen::Vector3f findMaxTravel(const Eigen::Vector3f& current_pos, const Eigen::Vector3f& delta_pos, float max_step, float min_gap, float max_slope, int iters = 3) const {
		//assert current_pos is valid
		//assert return is valid
		Eigen::Vector3f delta_pos_half = delta_pos / 2;
		auto next_zdata = getZdata(current_pos + delta_pos_half, max_step);
		zdata next_pos = next_zdata.first;
		Eigen::Vector3f new_pos;
		//need to also check for delta_pos greater than max slope
		if (iters == 0) {
			return current_pos;
		}
		if (next_pos.room_id == zdata::BaseRoom || next_zdata.second.z - next_pos.z < min_gap) {
			new_pos = current_pos;
		}
		else {
			new_pos << current_pos(0) + delta_pos_half(0), next_pos.z, current_pos(2) + delta_pos_half(2);
		}
//		Eigen::Vector3f delta_pos_half(delta_pos_(0), delta_pos_(1), delta_pos_(2));
		return findMaxTravel(new_pos, delta_pos_half, max_step, min_gap, max_slope, iters - 1);
	}

	Eigen::Vector3f getNewPosition(const Eigen::Vector3f& current_pos,const Eigen::Vector3f& delta_pos,float max_step,float min_gap,float clearance_width,float max_slope,bool& freefall_flag,int max_iter=3) const {
		//assert current_pos is valid
		//assert return is valid
		auto next_zdata = getZdata(current_pos + delta_pos, max_step);
		zdata next_pos = next_zdata.first;
		if (delta_pos.norm() == 0) {
			return current_pos;
		}
		if (next_pos.room_id == zdata::BaseRoom || next_zdata.second.z - next_pos.z < min_gap) {
			Eigen::Vector3f delta_pos_perp(-delta_pos(2), delta_pos(1), delta_pos(0));
			Eigen::Vector3f pos_left = findMaxTravel(current_pos, clearance_width / delta_pos_perp.norm() * delta_pos_perp / 2, max_step, min_gap, max_slope, max_iter);
			Eigen::Vector3f pos_right = findMaxTravel(current_pos, -clearance_width / delta_pos_perp.norm() * delta_pos_perp / 2, max_step, min_gap, max_slope, max_iter);
			//5*delta_pos_perp is equivalent to the object width, i.e the object will stay that width away from walls and center itself 
			//if there are two walls less than its width
			//division is OK because of if statement in beginning
			//have to do elipse math to figure out width at max delta depth
			return	(pos_left+pos_right-2*current_pos) * delta_pos_perp.norm()/clearance_width + current_pos;

		} else {
			float next_z;
			if (current_pos(1)-max_step > next_pos.z) {
				next_z = current_pos(1);
				freefall_flag = true;
			}
			else {
				next_z = next_pos.z;
				freefall_flag = false;
			}
			return Eigen::Vector3f(current_pos(0) + delta_pos(0), next_z, current_pos(2) + delta_pos(2));
		}
		/*
		//need to also check for delta_pos greater than max slope
		if (max_iter == 0) {//works like a runnaway iteration guard, so only needs to prevent calls which add another iteration.
			return current_pos;
		}
		if (next_pos.room_id == zdata::BaseRoom || next_zdata.second.z - next_pos.z < min_gap ) {
			if (first_iter) {
				Eigen::Vector3f delta_pos_perp(-delta_pos(2), delta_pos(1), delta_pos(0));
				return (getNewPosition(current_pos, delta_pos_perp, max_step, min_gap, max_slope,freefall_flag, max_iter - 1, false)
					+ getNewPosition(current_pos, -delta_pos_perp, max_step, min_gap, max_slope,freefall_flag, max_iter - 1, false))/2;
			}
			else {
				Eigen::Vector3f delta_pos_half(delta_pos(0)/2, delta_pos(1), delta_pos(2)/2);
				return getNewPosition(current_pos, delta_pos_half, max_step, min_gap, max_slope,freefall_flag, max_iter - 1,false);
			}
		}
		else {
			Eigen::Vector3f new_pos;
			new_pos << current_pos(0) + delta_pos(0), next_pos.z, current_pos(2) + delta_pos(2);
			if (first_iter == true) {
				return new_pos; //here for performance, but would technically work for infinite iterations since the infinite sum of 1/2^2 converges at 1
			}
			else {
				Eigen::Vector3f delta_pos_half(delta_pos(0) / 2, delta_pos(1), delta_pos(2) / 2);
				return getNewPosition(new_pos, delta_pos_half, max_step, min_gap, max_slope, freefall_flag, max_iter - 1, false);
			}
		}*/

	}

	void createData(const GameObject& level, unsigned int n_steps, const std::vector<const GameObject*>& neighbors,void* ZMapper_ptr);

	bool insideRegion(Eigen::Vector3f state) const {
		return (getRoom(state) == 1);
	};

};

#endif
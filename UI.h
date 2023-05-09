#pragma once

#ifndef PUPPET_UI
#define PUPPET_UI

#include "Model.h"
#include "GameObject.h"
#include "Default2d.hpp"

class Rect2d : public Model {
private:

	constexpr static std::vector<float> RectVerts(float height, float width) {
		return std::vector<float>{width/2,height/2,0,-width/2,height/2,0,width/2,-height/2,0,-width/2,-height/2,0 };
	}
	constexpr static std::vector<float> RectNorms() {
		return std::vector<float>{0.,0.,-1.,0,0,-1,0,0,-1,0,0,-1};
	}
	constexpr static std::vector<float> RectTex(float height, float width, float top, float left) {
		//(height ,width, 0,0) is top left
		return std::vector<float>{left, top, left+width, top, left, top+height, left+width, top+height};
	}
	constexpr static std::vector<unsigned int> RectFace() {
		return std::vector<unsigned int>{0,1,2,3};
	}
	constexpr static std::vector<unsigned int> RectFaceNorm() {
		return std::vector<unsigned int>{0, 1, 2, 3};
	}
	constexpr static std::vector<unsigned int> RectFaceTex() {
		return std::vector<unsigned int>{0, 1, 2, 3};
	}


public:

	Rect2d(float height, float width):
	Model(RectVerts(height,width),RectNorms(),RectTex(1.,1.,0.,0.),RectFace(),RectFaceNorm(),RectFaceTex()){}
};


class Button : public GameObject{

	const float height_, width_;
//	float x_, y_; //center coord
	Rect2d button_model_;

	void* callback_input_;
	void (*callback_func_)(void*);

	float getX() const {
		return getPosition()(0, 3);
	}

	float getY() const {
		return getPosition()(1, 3);
	}

	void onMouseDown(int key,float x, float y) override {
		if (!isHidden() &&
			x > getX() - width_ / 2 && x < getX() + width_ / 2
			&& y > getY() - height_ / 2 && y < getY() + height_ / 2) {
			if (callback_func_ != nullptr){
				callback_func_(callback_input_);
			}
			//std::cout << "within button!\n";
		}
	}

public:

	Button(float height, float width) :
		height_(height),
		width_(width),
		button_model_(height_, width_) {

		setModel(&button_model_);
		setTexture(new Texture("rocky.jpg"));
	}

	Button(float height, float width, std::string name) : 
		GameObject(name),
		height_(height),
		width_(width),
		button_model_(height_,width_){

		setModel(&button_model_);
		setTexture(new Texture("rocky.jpg"));
	}

	void setCallback(void (*callback_func)(void*), void* callback_input) {
		callback_func_ = callback_func;
		callback_input_ = callback_input;
	}

	float getHeight() const {
		return height_;
	}
	float getWidth() const {
		return width_;
	}
};

class TextboxObject : public GameObject, public Textbox {
private:
	std::string last_text_;
	TextGraphics* text_graphics_;
protected:

	void onStep() override {
		float new_left = GameObject::getPosition()(0, 3) - box_width / 2;
		float new_top = GameObject::getPosition()(1, 3) - box_height / 2;
		left = GameObject::getPosition()(0, 3) - box_width / 2;
		top = GameObject::getPosition()(1, 3) - box_height / 2;
	}
};

class Slider : public GameObject {
	float lower_limit_, upper_limit_;
	float current_position_;
	float height_, width_;
	float increment_fraction_;

	Rect2d slider_model_;

	void (*slider_change_callback_)(float, void*); //float being the new slider value

	Button increment_;
	Button decrement_;
	Button slider_;

	TextboxObject current_value_;
	TextboxObject lower_limit_value_;
	TextboxObject upper_limit_value_;

	OffsetConnector increment_offset_;
	OffsetConnector decrement_offset_;
	OffsetConnector current_val_offset_;
	OffsetConnector lower_lim_offset_;
	OffsetConnector upper_lim_offset_;
	PrismaticJoint slider_connector_;

	Default2d* graphics_2d_;
	TextGraphics* text_graphics_;

	static void incrementCallback(void* must_be_this) {
		Slider* this_ = static_cast<Slider*>(must_be_this);
		float new_val = this_->current_position_ + this_->increment_fraction_ * (this_->upper_limit_ - this_->lower_limit_);
		if (new_val > this_->upper_limit_) {
			new_val = this_->upper_limit_;
		}
		this_->setCurrentValue(new_val);
	}
	static void decrementCallback(void* must_be_this) {
		Slider* this_ = static_cast<Slider*>(must_be_this);
		float new_val = this_->current_position_ - this_->increment_fraction_ * (this_->upper_limit_ - this_->lower_limit_);
		if (new_val < this_->lower_limit_) {
			new_val = this_->lower_limit_;
		}
		this_->setCurrentValue(new_val);
	}
	

public:
	Slider(float height, float width, float lower_limit, float upper_limit):
		GameObject(),
		height_(height),
		width_(width),
		slider_model_(height,width),
		increment_(height_,width_/10),
		decrement_(height_,width_/10),
		slider_(height_*1.5,width/20),
		slider_change_callback_(nullptr),
		lower_limit_(lower_limit),
		upper_limit_(upper_limit),
		increment_fraction_(.05f),
		current_position_(lower_limit/2 + upper_limit/2),
		increment_offset_(width_*.5 + increment_.getWidth(), 0, 0),
		decrement_offset_(width_*-.5-decrement_.getWidth(), 0, 0),
		current_val_offset_(0, 2*height_,0),
		lower_lim_offset_(0, 2*height_, 0),
		upper_lim_offset_(0, 2*height_, 0),
		slider_connector_(Eigen::Vector3f(1.,0.,0.)){
		
		setTexture(new Texture("rocky.jpg"));
		setModel(&slider_model_);

		increment_.connectTo(this, &increment_offset_);
		decrement_.connectTo(this, &decrement_offset_);
		current_value_.connectTo(this, &current_val_offset_);
		upper_limit_value_.connectTo(&increment_, &upper_lim_offset_);
		lower_limit_value_.connectTo(&decrement_, &lower_lim_offset_);
		slider_.connectTo(this, &slider_connector_);

		increment_.setCallback(incrementCallback, this);
		decrement_.setCallback(decrementCallback, this);

	}

	void load(GLFWwindow* window, Default2d& graphics_2d, TextGraphics& text_graphics) {
		graphics_2d_ = &graphics_2d;
		text_graphics_ = &text_graphics;

		graphics_2d.add(*this);

		increment_.activateMouseInput(window);
		decrement_.activateMouseInput(window);
		slider_.activateMouseInput(window);

		graphics_2d.add(increment_);
		graphics_2d.add(decrement_);
		graphics_2d.add(slider_);
		
		current_value_.text = std::to_string(current_position_);
		lower_limit_value_.text = std::to_string(lower_limit_);
		upper_limit_value_.text = std::to_string(upper_limit_);

		current_value_.box_height = height_;
		lower_limit_value_.box_height = height_;
		upper_limit_value_.box_height = height_;
		current_value_.box_width = width_/4.;
		lower_limit_value_.box_width = width_ / 4.;
		upper_limit_value_.box_width = width_ / 4.;

		current_value_.font_size = .5;// current_value_.box_width / 5;
		lower_limit_value_.font_size = .5;// lower_limit_value_.box_width / 5;
		upper_limit_value_.font_size = .5;// upper_limit_value_.box_width / 5;
		

		text_graphics.add(current_value_);//for some reason removing this and beginning with the menu hidden causes an error
		text_graphics.add(lower_limit_value_);
		text_graphics.add(upper_limit_value_);


	}

	void unload() {

	}
	
	void setCurrentValue(float new_val) {
		if (new_val <= upper_limit_ && new_val >= lower_limit_) {
			current_position_ = new_val;
			if (slider_change_callback_ != nullptr) {
				slider_change_callback_(new_val, this);
			}
		}
	}

	void update(GLFWwindow* window) override {

		slider_connector_.setState(width_ * (current_position_ - lower_limit_) / (upper_limit_ - lower_limit_) - width_ / 2);

		GameObject::update(window);
		increment_.update(window);
		decrement_.update(window);
		slider_.update(window);
		lower_limit_value_.update(window);
		upper_limit_value_.update(window);
		current_value_.update(window);
	}

	void setSliderChangeCallback(void (*slider_change_callback)(float, void*)) {
		slider_change_callback_ = slider_change_callback;
	}
};

/*
template<class T>
concept GameObjectIterator = requires(const T & iterator, unsigned int i) {
	//{*iterator[i]} ->  std::convertible_to<GameObject*>;
	{iterator.size()} -> std::convertible_to<size_t>;
	{iterator.begin()} -> std::convertible_to<std::iterator< std::bidirectional_iterator_tag, GameObject*>>;
};

template<GameObjectIterator iterator_T>
class UIIterator : public GameObject{
	Button next_target_;
	Button prev_target_;

	const iterator_T* iterable_;
	std::iterator< std::bidirectional_iterator_tag, GameObject*> target_iterator_;
	Textbox target_name_;

	void (*callback_on_increment_)(GameObject*, GameObject*);
	void (*callback_on_decrement_)(GameObject*, GameObject*);


	static void nextTargetCallback(void* must_be_this) {
		UIIterator* this_ = static_cast<DebugMenu*>(must_be_this);
		if (this_->iterable == nullptr) { return; };
		if (this_->target_iterator_ == iterable_.end()) {
			target_iterator_ = iterable_.begin();
			callback_on_increment_(nullptr, *target_iterator_);
			return;
		}
		GameObject* prev_target = *target_iterator;
		target_iterator_++;
		callback_on_increment_(prev_target, *target_iterator_);
	}

	static void prevTargetCallback(void* must_be_this) {
		UIIterator* this_ = static_cast<DebugMenu*>(must_be_this);
		if (this_->iterable == nullptr) { return; };
		if (this_->target_iterator_ == iterable_.begin()) {
			target_iterator_ = iterable_.end();
			callback_on_increment_(*iterator_.begin(),nullptr);
			return;
		}
		GameObject* prev_target = *target_iterator;
		target_iterator_--;
		callback_on_increment_(prev_target, *target_iterator_);
	}


public:
	float button_size;
	float text_width;
	float text_height;
	float top;
	float left;

	class UIIterator(std::string name, Default2d graphics) :
		GameObject(name),
		next_target_(.2, .2, name + "_next_target"),
		prev_target_(.2, .2, name + "_prev_target") {


		prev_target_.moveTo(-1, -.4, 0);
		prev_target_.activateMouseInput(window);
		graphics.add(prev_target_);
		next_target_.moveTo(-.2, -.4, 0);
		next_target_.activateMouseInput(window);
		graphics.add(next_target_);
		prev_target_.setCallback(prevTargetCallback,this);
		next_target_.setCallback(nextTargetCallback,this);

		target_name_.box_width = .6;
		target_name_.box_height = .2;
		target_name_.font_size = 1;
		target_name_.left = -.8;
		target_name_.top = -.4;

	}

	GameObject* getTarget() {
		return *target_iterator_;
	}

};
*/
#endif
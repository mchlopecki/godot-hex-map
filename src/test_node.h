#pragma once

#include <godot_cpp/classes/node3d.hpp>
using namespace godot;

class TestNode : public godot::Node3D {
	GDCLASS(TestNode, Node3D)

private:
protected:
	static void _bind_methods() {};

public:
	TestNode(){};
	~TestNode(){};
};

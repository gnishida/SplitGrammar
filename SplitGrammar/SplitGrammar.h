#pragma once

#include <vector>
#include <map>
#include <string>
#include <glm/glm.hpp>

class Region {
public:
	float left;
	float bottom;
	float width;
	float height;

public:
	Region() {}
	Region(float left, float bottom, float width, float height) : left(left), bottom(bottom), width(width), height(height) {}
};

class Terminal {
public:
	std::string name;
	glm::vec3 color;

public:
	Terminal() {}
	Terminal(const std::string& name, const glm::vec3& color) : name(name), color(color) {}
};

class Variable {
public:
	std::string name;
	Region region;

public:
	Variable() {}
	Variable(const std::string& name) : name(name) {}
};

class Rule {
public:
	static enum { DIRECTION_X = 0, DIRECTION_Y };
	static enum { TYPE_SPLIT = 0, TYPE_REPEAT };

public:
	Variable var;
	int type;
	int direction;
	std::vector<Variable> children;

public:
	Rule() {}
	Rule(const std::string& rule_string);
};

class Model {
public:
	std::vector<Variable> model;
	
public:
	int size() const { return model.size(); }
	void push_back(const Variable& var);
	Variable operator[](int index) const { return model[index]; }
	Variable& operator[](int index) { return model[index]; }
	void erase(int index);
	std::vector<Variable>::iterator insert(int index, const Variable& var);
};

std::ostream& operator<<(std::ostream& os, const Model& model);

class SplitGrammar {
public:
	std::map<std::string, Rule> rules;
	std::map<std::string, Terminal> terminals;

	float width;
	float height;

public:
	SplitGrammar();
	Model derive(int max_iterations);
	void draw(const Model& model);
};

std::string trim(const std::string& str);
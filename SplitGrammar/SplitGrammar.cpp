#include "SplitGrammar.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <opencv/cv.h>
#include <opencv/highgui.h>

/**
 * 以下のフォーマットから、ルールを構築する。
 * NT1->split(y){Wall1|NT2|NT3|Wall1|NT4|Wall1}
 */
Rule::Rule(const std::string& rule_string) {
	// Left hand を取得
	int index = rule_string.find("->");
	std::string lh = trim(rule_string.substr(0, index));
	var = Variable(lh);

	// split or repeat
	index += 2;
	int index2 = rule_string.find("(", index);
	if (trim(rule_string.substr(index, index2 - index)) == "split") {
		type = TYPE_SPLIT;
	} else {
		type = TYPE_REPEAT;
	}

	// direction (x or y)
	index = index2 + 1;
	index2 = rule_string.find(")", index);
	if (trim(rule_string.substr(index, index2 - index)) == "x") {
		direction = DIRECTION_X;
	} else {
		direction = DIRECTION_Y;
	}

	index = rule_string.find("{", index2) + 1;

	// children
	bool end = false;
	while (!end) {
		index2 = rule_string.find("|", index);
		if (index2 < 0) {
			index2 = rule_string.find("}", index);
			end = true;
			if (index2 == -1) throw "Invalid rule: " + rule_string;
		}
		std::string child = trim(rule_string.substr(index, index2 - index));
		children.push_back(child);

		index = index2 + 1;
	}
}

void Model::push_back(const Variable& var) {
	model.push_back(var);
}

void Model::erase(int index) {
	model.erase(model.begin() + index);
}

std::vector<Variable>::iterator Model::insert(int index, const Variable& var) {
	return model.insert(model.begin() + index, var);
}

std::ostream& operator<<(std::ostream& os, const Model& model) {
	os << std::setprecision(1);
	for (int i = 0; i < model.size(); ++i) {
		os << "[" << model[i].name << "]";
	}

    return os;
}

SplitGrammar::SplitGrammar() {
	terminals["Wall1"] = Terminal("Wall1", glm::vec3(128, 128, 128));
	terminals["Wall2"] = Terminal("Wall2", glm::vec3(192, 192, 192));
	terminals["Window1"] = Terminal("Window", glm::vec3(0, 0, 255));
	terminals["Window2"] = Terminal("Window", glm::vec3(0, 128, 128));

	rules["NT1"] = Rule("NT1 -> split(y) { Wall1 | NT2 | NT3 | Wall1 | NT4 | Wall1 }");
	rules["NT2"] = Rule("NT2 -> repeat(y) {NT5 }");
	rules["NT3"] = Rule("NT3 -> split(x) { NT6 | Wall1 | Window1 | Wall2 | Window1 | NT7 }");
	rules["NT4"] = Rule("NT4 -> split(x) { NT6 | Wall2 | Wall1 | NT8 | Wall2 | Window2 | Wall1 | NT9 }");
	rules["NT5"] = Rule("NT5 -> split(y) { NT10 | Wall1 }");
	rules["NT6"] = Rule("NT6 -> split(x) { Wall1 | Window1 }");
	rules["NT7"] = Rule("NT7 -> split(x) { Wall1 | Window2 | Wall2 | NT9 }");
	rules["NT8"] = Rule("NT8 -> split(x) { Window1 | Wall1 | Window1 }");
	rules["NT9"] = Rule("NT9 -> split(x) { Window1 | Wall1 }");
	rules["NT10"] = Rule("NT10 -> split(x) { NT6 | Wall2 | NT8 | NT7 }");

	// Facadeのサイズ
	width = 100;
	height = 100;
}

Model SplitGrammar::derive(int max_iterations) {
	Model model;

	// axiom
	Variable axiom = Variable("NT1");
	axiom.region.left = 0;
	axiom.region.bottom = 0;
	axiom.region.width = width;
	axiom.region.height = height;
	model.push_back(axiom);

	for (int iter = 0; iter < max_iterations; ++iter) {
		bool updated = false;
		for (int i = 0; i < model.size(); ++i) {
			if (terminals.find(model[i].name) != terminals.end()) continue;

			Rule rule = rules[model[i].name];
			Region region = model[i].region;

			model.erase(i);

			if (rule.type == Rule::TYPE_SPLIT) {
				if (rule.direction == Rule::DIRECTION_X) {
					float width = region.width / (float)rule.children.size();
					float left = region.left;
					for (int j = 0; j < rule.children.size(); ++j) {
						auto inserted_model = model.insert(i + j, rule.children[j]);
						inserted_model->region = Region(left, region.bottom, width, region.height);

						left += width;
					}
				} else {
					float height = region.height / (float)rule.children.size();
					float bottom = region.bottom;
					for (int j = 0; j < rule.children.size(); ++j) {
						auto inserted_model = model.insert(i + j, rule.children[j]);
						inserted_model->region = Region(region.left, bottom, region.width, height);

						bottom += height;
					}
				}
			} else if (rule.type == Rule::TYPE_REPEAT) {
				// 繰り返し数（とりあえず、固定で）
				int repetition = 2;

				if (rule.direction == Rule::DIRECTION_X) {
					float width = region.width / (float)repetition / (float)rule.children.size();
					float left = region.left;
					for (int j = 0; j < repetition; ++j) {
						for (int k = 0; k < rule.children.size(); ++k) {
							auto inserted_model = model.insert(i + j * rule.children.size() + k, rule.children[k]);
							inserted_model->region = Region(left, region.bottom, width, region.height);

							left += width;
						}
					}
				} else {
					float height = region.height / (float)repetition / (float)rule.children.size();
					float bottom = region.bottom;
					for (int j = 0; j < repetition; ++j) {
						for (int k = 0; k < rule.children.size(); ++k) {
							auto inserted_model = model.insert(i + j * rule.children.size() + k, rule.children[k]);
							inserted_model->region = Region(region.left, bottom, region.width, height);

							bottom += height;
						}
					}
				}
			}

			updated = true;
			break;
		}

		// Non-terminalがないなら、終了
		if (!updated) break;
	}

	return model;
}

void SplitGrammar::draw(const Model& model) {
	cv::Mat img = cv::Mat::zeros(width, height, CV_8UC3);

	for (int i = 0; i < model.size(); ++i) {
		cv::Rect rect(model[i].region.left, model[i].region.bottom, model[i].region.width, model[i].region.height);
		glm::vec3 color = terminals[model[i].name].color;

		cv::rectangle(img, rect, cv::Scalar(color.b, color.g, color.r), CV_FILLED);
	}

	cv::flip(img, img, 0);
	cv::imwrite("result.png", img);
}

std::string trim(const std::string& str) {
    std::string::const_iterator left = std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun(std::isspace)));
    std::string::const_reverse_iterator right = std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun(std::isspace)));
    return (left < right.base()) ? std::string(left, right.base()) : std::string();
}
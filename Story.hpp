#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct Story {
	struct Choice {
		std::string text;
		std::string go_to; //name of node this choice leads to

		//only show this choice if ALL of these flags are set:
		std::vector<std::string> need = {};
		//only show this choice if NONE of these flags are set:
		std::vector<std::string> need_not = {};

		//when chosen, set these flags:
		std::vector<std::string> set_flags = {};
		//when chosen, hide these meshes:
		std::vector<std::string> hide_meshes = {};
	};

	struct Node {
		std::string camera;
		std::string text;
		std::vector<Choice> choices;
	};

	std::string start; //name of the first node
	std::unordered_map<std::string, Node> nodes; //name -> node
};

Story make_story();
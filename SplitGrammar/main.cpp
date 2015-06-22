#include "SplitGrammar.h"
#include <iostream>

int main() {
	SplitGrammar sg;

	Model result = sg.derive(1000);
	std::cout << result << std::endl;
	sg.draw(result);

	return 0;
}

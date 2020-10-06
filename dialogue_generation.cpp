#include "GL.hpp"
#include "load_save_png.hpp"
#include "read_write_chunk.hpp"
#include "data_path.hpp"

#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <stdint.h>

int main(int argc, char **argv) {
	std::ifstream metafile;
	// std::vector<> data;
	metafile.open(data_path("../tmp_dialogue.txt"));
	std::vector<char> characters;
	std::vector<std::pair<int, int>> ranges;
	std::string line;
	if (metafile.is_open()) {
		int cur = 0;
		while (std::getline(metafile, line)) {
			int length = line.length();
			for (int i = 0; i < length; i++) {
				characters.push_back(line[i]);
			}
			ranges.push_back(std::pair<int, int>(cur, cur + length));
			cur += length;
		}
		std::cout << cur;
		metafile.close();
		std::ofstream chars_stream;
		std::ofstream ranges_stream;
		chars_stream.open(data_path("../chars.asset"));
		ranges_stream.open(data_path("../ranges.asset"));
		write_chunk(std::string("char"), characters, &chars_stream);
		write_chunk(std::string("rang"), ranges, &ranges_stream);
		chars_stream.close();
		ranges_stream.close();
	}

	return 0;

}

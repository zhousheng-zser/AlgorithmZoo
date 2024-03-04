#include "PrintTable.hpp"
#include <iostream>
#include <array>

class TableMaker {
	PrintTable pt;
	//std::vector<std::vector<std::string>> rows;
public:
	TableMaker(std::string headinfo) {
		pt.SetTitle(headinfo + " Infer Precision");
        pt.AddColumn("output_name");
        pt.AddColumn("less99");
        pt.AddColumn("less98");
        pt.AddColumn("less95");
        pt.AddColumn("less90");
        pt.AddColumn("less85");
        pt.AddColumn("less80");
        pt.AddColumn("less70");
        pt.AddColumn("min_cos");
	}

	void rowPushLine(std::string output_name, std::array<int,7> less_vec,float min_cos,int infr_count) {
		std::vector<std::string> line{ output_name };
		for (auto v : less_vec) {
			std::string ratioInfo = std::to_string(v) + ", " + std::to_string(v * 1.f / infr_count);
			line.push_back(ratioInfo);
		}
		line.push_back(std::to_string(min_cos));

		pt.AddRow(line);
		//rows.push_back(line);
	}

	void show() {
		std::cout << std::endl << std::endl;
		//pt.AddRows(rows);
		pt.Print();
	}

};
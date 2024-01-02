#include <iostream>
#include <vector>
#include <cmath>
#include <map>

struct cluster_info
{
    int x1;
    int y1;
    int x2;
    int y2;
    double x;
    double y;
};

class standard_scaler {
public:
    void fit(const std::vector<cluster_info>& data) {
        // 计算每列的均值和标准差
        unsigned int num_features = 2;
        means.resize(num_features, 0.0);
        devs.resize(num_features, 0.0);
        for (const auto& row : data) {
            means[0] += row.x;
            means[1] += row.y;
        }
        unsigned int num_rows = data.size();
        means[0] /= num_rows;
        means[1] /= num_rows;
        for (const auto& row : data) {
            devs[0] += (row.x - means[0]) * (row.x - means[0]);
            devs[1] += (row.y - means[1]) * (row.y - means[1]);
        }
        devs[0] = std::sqrt(devs[0] / num_rows);
        devs[1] = std::sqrt(devs[1] / num_rows);
    }

    std::vector<cluster_info> transform(const std::vector<cluster_info>& data) const {
        std::vector<cluster_info> scaled_data = data;
        unsigned int num_features = 2;
        unsigned int num_rows = data.size();
        for (unsigned int j = 0; j < num_rows; ++j) {
            scaled_data[j].x = (scaled_data[j].x - means[0]) / devs[0];
            scaled_data[j].y = (scaled_data[j].y - means[1]) / devs[1];
        }
        return scaled_data;
    }

private:
    std::vector<double> means;
    std::vector<double> devs;
};

class cluster_num {
public:
    // 计算两点之间的欧氏距离
    double calculate_distance(const cluster_info& p1, const cluster_info& p2) {
        return std::sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
    }

    // 为数据集中的每个点查找k个最近邻
    void find_nearest_neighbors(const std::vector<cluster_info>& dataset, int k, std::vector<std::vector<int>>& indices) {
        indices.resize(dataset.size());

        for (unsigned int i = 0; i < dataset.size(); ++i) {
            // 计算与所有其他点的距离
            std::vector<std::pair<double, unsigned int>> distances;
            for (unsigned int j = 0; j < dataset.size(); ++j) {
                if (i != j) {
                    double distance = calculate_distance(dataset[i], dataset[j]);
                    distances.push_back(std::make_pair(distance, j));
                }
            }
            // 对距离进行排序并选择前k个邻居
            std::sort(distances.begin(), distances.end());
            indices[i].resize(k);
            for (int n = 0; n < k; ++n) {
                indices[i][n] = distances[n].second;
            }
        }
    }

    // 寻找最优eps值的函数
    double find_optimal_eps(const std::vector<double>& k_distances) {
        // 计算k-距离的导数
        std::vector<double> derivatives;
        std::transform(k_distances.begin(), k_distances.end() - 1, k_distances.begin() + 1, std::back_inserter(derivatives),
            [](double x, double y) { return y - x; });
        // 找到导数最大值的索引，即对应拐点
        auto optimal_index = std::max_element(derivatives.begin(), derivatives.end()) - derivatives.begin();
        // 返回拐点处的eps值
        return k_distances[optimal_index];
    }

    // 寻找给定点的邻域内的其他点
    std::vector<int> find_neighbors(const std::vector<cluster_info>& dataset, int targetIndex, double eps) {
        std::vector<int> neighbors;
        for (int i = 0; i < dataset.size(); ++i) {
            if (i != targetIndex) {
                double distance = calculate_distance(dataset[targetIndex], dataset[i]);
                if (distance < eps) {
                    neighbors.push_back(i);
                }
            }
        }
        return neighbors;
    }

    // 执行DBSCAN聚类
    void dbscan(const std::vector<cluster_info>& dataset, double eps, int minSamples, std::vector<int>& labels) {
        labels.assign(dataset.size(), -1);  // 初始化标签为-1，表示未分类
        int cluster_idx = 0;
        for (int i = 0; i < dataset.size(); ++i) {
            if (labels[i] != -1) {
                continue;  // 已经分类的点跳过
            }
            std::vector<int> neighbors = find_neighbors(dataset, i, eps);
            if (neighbors.size() < minSamples) {
                labels[i] = 0;  // 标记为噪声点
                continue;
            }
            // 新的簇
            cluster_idx++;
            labels[i] = cluster_idx;
            for (unsigned int j = 0; j < neighbors.size(); ++j) {
                int neighbor_idx = neighbors[j];
                if (labels[neighbor_idx] == 0) {
                    labels[neighbor_idx] = cluster_idx;  // 噪声点归属到当前簇
                }
                if (labels[neighbor_idx] != -1) {
                    continue;  // 已经分类的点跳过
                }
                labels[neighbor_idx] = cluster_idx;  // 将邻居分到当前簇
                std::vector<int> new_neighbors = find_neighbors(dataset, neighbor_idx, eps);
                if (new_neighbors.size() >= minSamples) {
                    neighbors.insert(neighbors.end(), new_neighbors.begin(), new_neighbors.end());
                }
            }
        }
    }


    //exposing::param_vector<crowd::box_info> 
    std::vector<int> find_cluster_num(const std::vector<cluster_info>& detection_points, int min_cluster_size) {
        standard_scaler scaler;
        //data = detection_points;
        // 计算均值和标准差
        scaler.fit(detection_points);
        // 对数据进行标准化
        std::vector<cluster_info> scaled_data = scaler.transform(detection_points);
        int  k = 3;
        std::vector<std::vector<int>> neighbor_indices;
        std::vector<double> k_distances;
        // 为每个点找到k个最近邻
        find_nearest_neighbors(scaled_data, k, neighbor_indices);
        // 计算k-距离
        for (unsigned int i = 0; i < scaled_data.size(); ++i) {
            k_distances.push_back(calculate_distance(scaled_data[i], scaled_data[neighbor_indices[i][k - 1]]));
        }
        // 对k-距离进行排序
        std::sort(k_distances.begin(), k_distances.end());
        // 寻找最优eps值
        double eps = find_optimal_eps(k_distances) * 1.5;
        // 打印结果
        //  std::cout << "eps: " << eps << std::endl;
        std::vector<int> labels;
        // 执行 DBSCAN 聚类
        dbscan(scaled_data, eps, k - 1, labels);
        
        std::map<int, int>temp;
        temp.clear();
        for (int label : labels) {
            temp[label]++;
        }
        for (int &label : labels) {
            if (temp[label] < min_cluster_size)
                label = 0; 
        }

        // 打印结果
        //std::cout << "Cluster labels: ";
        //for (int label : labels) {
        //    std::cout << label << "\n";
        //}
        return labels;
    }
};


//int main() {
//    // 创建一个示例数据集
//    std::vector<cluster_info> data = {
//     cluster_info{.x = 463.12121212 ,.y = 161.87878788},
//     cluster_info{.x = 483.09333333 ,.y = 168.57333333},
//     cluster_info{.x = 864        ,.y = 176.5       },
//     cluster_info{.x = 916.66666667 ,.y = 180.5       },
//     cluster_info{.x = 902.51162791 ,.y = 189.12790698},
//     cluster_info{.x = 355.78378378 ,.y = 203.89189189},
//     cluster_info{.x = 960.5        ,.y = 203       },
//     cluster_info{.x = 368.69444444 ,.y = 205.75      },
//     cluster_info{.x = 943.89473684 ,.y = 206.31578947},
//     cluster_info{.x = 929.47727273 ,.y = 212.06818182},
//     cluster_info{.x = 972.60526316 ,.y = 210.84210526},
//     cluster_info{.x = 997.2        ,.y = 212.76      },
//     cluster_info{.x = 1010.0862069  ,.y = 212.75862069},
//     cluster_info{.x = 648.35135135 ,.y = 213.54054054},
//     cluster_info{.x = 373.86206897 ,.y = 214.62068966},
//     cluster_info{.x = 657.05       ,.y = 216.075     },
//     cluster_info{.x = 948.89583333 ,.y = 220.375     },
//     cluster_info{.x = 968.65168539 ,.y = 221.05617978},
//     cluster_info{.x = 1058.12       ,.y = 220.76      },
//     cluster_info{.x = 1105.48076923 ,.y = 219.71153846},
//     cluster_info{.x = 66        ,.y = 221.5       },
//     cluster_info{.x = 933        ,.y = 221.65384615},
//     cluster_info{.x = 1037.72727273 ,.y = 221.66666667},
//     cluster_info{.x = 1130.63076923 ,.y = 223.30769231},
//     cluster_info{.x = 988.28       ,.y = 224.94      },
//     cluster_info{.x = 1117.125      ,.y = 226.05357143},
//     cluster_info{.x = 886.25       ,.y = 229.38636364},
//     cluster_info{.x = 1001.46052632 ,.y = 232.84210526},
//     cluster_info{.x = 1020.31521739 ,.y = 233.83695652},
//     cluster_info{.x = 1037.52173913 ,.y = 233.82608696},
//     cluster_info{.x = 1090.69105691 ,.y = 240.17886179},
//     cluster_info{.x = 1054.024      ,.y = 241.888     },
//     cluster_info{.x = 1038.69565217 ,.y = 249.89855072},
//     cluster_info{.x = 1093.52884615 ,.y = 273.47115385},
//     cluster_info{.x = 104.6013986  ,.y = 312.92307692},
//     cluster_info{.x = 270.07189542 ,.y = 312.85620915},
//     cluster_info{.x = 188.76923077 ,.y = 321.01282051},
//     cluster_info{.x = 177        ,.y = 325.81632653},
//     cluster_info{.x = 137.56179775 ,.y = 330.79213483},
//     cluster_info{.x = 205.42857143 ,.y = 327.72321429},
//     cluster_info{.x = 176.73770492 ,.y = 345.53005464},
//     cluster_info{.x = 128.3030303  ,.y = 345.15151515},
//     cluster_info{.x = 89.87375415 ,.y = 363.0166113 },
//     cluster_info{.x = 135.11570248 ,.y = 360.94214876},
//     cluster_info{.x = 148.53892216 ,.y = 526.11077844}
//    };
//
//    // 创建standard_scaler对象
//    cluster_num scaler;
//    scaler.find_cluster_num(data, 3);
//
//
//        return 0;
//}
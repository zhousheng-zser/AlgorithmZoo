#pragma once
#include <iostream>
#include <set>
#include <vector>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

namespace bg = boost::geometry;
typedef bg::model::d2::point_xy<double> bg_point;
typedef bg::model::polygon<bg_point> bg_polygon;

using point_coordinates = std::vector<std::array<int, 2>>;
using point_box_list = std::vector<point_coordinates>;

namespace point_reception {
    struct point {
        float x;
        float y;
        bool operator==(const point& other) const {
            return x == other.x && y == other.y;
        }
    };


    struct polygon {
        int number;
        std::vector<point> list;
        static point_box_list polygon_to_point(std::vector<polygon> poly_info) {
            if (poly_info.empty()) {
                return point_box_list{};
            }
            point_box_list result;
            for (auto& element : poly_info) {
                point_coordinates this_poly;
                for (auto& point : element.list) {
                    std::array<int, 2> point_pair;
                    point_pair[0] = point.x;
                    point_pair[1] = point.y;
                    this_poly.push_back(point_pair);
                }
                result.push_back(this_poly);
            }
            return result;
        }

        static std::vector<polygon> point_to_polygon(const point_box_list point_buffer) {
            if (point_buffer.empty()) {
                return std::vector<polygon>{};
            }
            std::vector<polygon> result;
            for (auto& element : point_buffer) {

                if (element.size() >= 2) {
                    polygon this_poly;
                    for (auto p : element) {
                        point this_point;
                        this_point.x = p[0];
                        this_point.y = p[1];
                        this_poly.list.push_back(this_point);
                    }
                    this_poly.number = element.size();
                    result.push_back(this_poly);
                }
            }
            return result;
        }

        static int cross_product(const point& p1, const point& p2) {
            return p1.x * p2.y - p1.y * p2.x;
        }


        static bool is_clockwise(const std::vector<point>& vertices) {
            if (vertices.size() != 4) {
                // 如果顶点数不是4，无法构成四边形
                return false;
            }
            int sum = 0;
            for (int i = 0; i < 4; i++) {
                const point& p1 = vertices[i];
                const point& p2 = vertices[(i + 1) % 4]; // 下一个顶点
                sum += cross_product(p1, p2);
            }
            return sum > 0;
        }

        static std::vector<polygon> point_to_climb_polygon(const point_box_list point_buffer) {
            if (point_buffer.empty()) {
                return std::vector<polygon>{};
            }
            std::vector<polygon> result;
            for (auto& element : point_buffer) {

                if (element.size() > 2) {
                    polygon this_poly;
                    for (auto p : element) {
                        point this_point;
                        this_point.x = p[0];
                        this_point.y = p[1];
                        this_poly.list.push_back(this_point);
                    }
                    this_poly.number = element.size();
                    /*        for (auto& p : this_poly.list)
                            {
                                std::cout << "old x:" << p.x << " old y :" << p.y << std::endl;
                            }*/
                    // 按照顺时针排序多边形的点
                    if (!is_clockwise(this_poly.list)) {
                        std::reverse(this_poly.list.begin(), this_poly.list.end());
                    }
                    result.push_back(this_poly);
                }
            }
            return result;
        }
        static std::string polygon_to_wkt(const polygon& poly) {
            std::stringstream ss;
            ss << "POLYGON((";

            for (size_t i = 0; i < poly.list.size(); ++i) {
                ss << poly.list[i].x << " " << poly.list[i].y;

                // 添加逗号分隔符，除非是最后一个点
                if (i < poly.list.size() - 1) {
                    ss << ", ";
                }
                if (i == poly.list.size() - 1) {
                    ss << ", " << poly.list[0].x << " " << poly.list[0].y;
                }
            }
            ss << "))";
            return ss.str();
        }
        static double count_intersect_area_ratio(polygon roi_poly, polygon poly) {
            bg_polygon bg_polygon1, bg_polygon2;
            std::string roi_poly_wkt = polygon_to_wkt(roi_poly);
            std::string poly_wkt = polygon_to_wkt(poly);
            bg::read_wkt(roi_poly_wkt, bg_polygon1);
            bg::read_wkt(poly_wkt, bg_polygon2);
            bg::correct(bg_polygon1);
            bg::correct(bg_polygon2);

            std::vector<bg_polygon> output;
            bg::intersection(bg_polygon1, bg_polygon2, output);
            double intersection_area = 0.0;

            if (!output.empty()) {
                const bg_polygon& intersection_polygon = output[0];
                //std::cout << "相交多边形的顶点坐标：" << std::endl;
                //for (const auto& point : intersection_polygon.outer()) {
                //    std::cout << "X: " << bg::get<0>(point) << ", Y: " << bg::get<1>(point) << std::endl;
                //}
                intersection_area = bg::area(intersection_polygon);
                double test_poly_area = bg::area(bg_polygon2);
                double ratio_ret = intersection_area / test_poly_area;
                //std::cout << "相交面积为: " << intersection_area << std::endl;
                //std::cout << "相交比例为: " << ratio_ret << std::endl;
                return ratio_ret;
            } else {
                return 0.0;
            }
        }

        static double count_intersect_area_ratio_to_roi(polygon roi_poly, polygon poly) {
            bg_polygon bg_polygon1, bg_polygon2;
            std::string roi_poly_wkt = polygon_to_wkt(roi_poly);
            std::string poly_wkt = polygon_to_wkt(poly);
            bg::read_wkt(roi_poly_wkt, bg_polygon1);
            bg::read_wkt(poly_wkt, bg_polygon2);
            bg::correct(bg_polygon1);
            bg::correct(bg_polygon2);

            std::vector<bg_polygon> output;
            bg::intersection(bg_polygon1, bg_polygon2, output);
            double intersection_area = 0.0;

            if (!output.empty()) {
                const bg_polygon& intersection_polygon = output[0];
                //std::cout << "相交多边形的顶点坐标：" << std::endl;
                //for (const auto& point : intersection_polygon.outer()) {
                //    std::cout << "X: " << bg::get<0>(point) << ", Y: " << bg::get<1>(point) << std::endl;
                //}
                intersection_area = bg::area(intersection_polygon);
                double test_poly_area = bg::area(bg_polygon1);
                double ratio_ret = intersection_area / test_poly_area;
                //std::cout << "相交面积为: " << intersection_area << std::endl;
                //std::cout << "相交比例为: " << ratio_ret << std::endl;
                return ratio_ret;
            } else {
                return 0.0;
            }
        }
        static polygon sdk_info_to_polygon(int x1, int y1, int x2, int y2) {
            point p1, p2, p3, p4;
            std::vector<point> point_vec(4);
            polygon poly_ret{4, point_vec};
            p1.x = x1;
            p1.y = y1;
            p2.x = x2;
            p2.y = y1;
            p3.x = x2;
            p3.y = y2;
            p4.x = x1;
            p4.y = y2;
            poly_ret.list[0] = p1;
            poly_ret.list[1] = p2;
            poly_ret.list[2] = p3;
            poly_ret.list[3] = p4;
            return poly_ret;
        }

        static polygon sdk_info_to_polygon_resize_to_big(int x1, int y1, int x2, int y2) {
            point p1, p2, p3, p4;
            std::vector<point> point_vec(4);
            polygon poly_ret{4, point_vec};
            int wide = x2 - x1;
            int the_long = y2 - y1;
            p1.x = x1 - (wide*0.15);
            p1.y = y1 - (the_long*0.15);
            p2.x = x2 + (wide * 0.15);
            p2.y = y1 - (the_long * 0.15);
            p3.x = x2 + (wide * 0.15);
            p3.y = y2 + (the_long * 0.15);
            p4.x = x1 - (wide * 0.15);
            p4.y = y2 + (the_long * 0.15);

            poly_ret.list[0] = p1;
            poly_ret.list[1] = p2;
            poly_ret.list[2] = p3;
            poly_ret.list[3] = p4;
            return poly_ret;
        }
        static bool are_polygons_equal(const point_reception::polygon& lhs, const point_reception::polygon& rhs) {
            return lhs.number == rhs.number && lhs.list == rhs.list;
        }

        static std::vector<point_reception::polygon> remove_duplicates(
            const std::vector<point_reception::polygon>& input) {
            std::vector<point_reception::polygon> result;
            for (const auto& poly : input) {
                bool isDuplicate = false;
                for (const auto& existingPoly : result) {
                    if (are_polygons_equal(poly, existingPoly)) {
                        isDuplicate = true;
                        break;
                    }
                }
                if (!isDuplicate) {
                    result.push_back(poly);
                }
            }
            return result;
        }
    };

    struct line {
        point p1;
        point p2;
        static std::vector<line> point_to_line(const point_box_list point_buffer) {
            if (point_buffer.empty()) {
                return std::vector<line>{};
            }
            std::vector<line> result;
            for (auto& element : point_buffer) {
                if (element.size() == 2) {
                    line this_line;
                    this_line.p1.x = element[0][0];
                    this_line.p1.y = element[0][1];
                    this_line.p2.x = element[1][0];
                    this_line.p2.y = element[1][1];
                    result.push_back(this_line);
                }
            }
            return result;
        }
    };
} // namespace point_reception

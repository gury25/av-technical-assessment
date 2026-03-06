#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

// THOUGHT PROCESS:
// 1. Read CSV file containing unordered cone coordinates
// 2. Separate cones into two groups: inner and outer boundaries using clustering
// 3. Sort both groups in order around the track (by polar angle from origin)
// 4. Pair corresponding inner/outer cones and generate midline points
// 5. Interpolate waypoints to ensure max 0.5m spacing
// 6. Write output CSV with waypoints

struct Point {
    double x, y;
    Point() : x(0), y(0) {}
    Point(double x_, double y_) : x(x_), y(y_) {}
    
    double distance_to(const Point& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    Point midpoint(const Point& other) const {
        return Point((x + other.x) / 2.0, (y + other.y) / 2.0);
    }
    
    double angle_from_origin() const {
        return std::atan2(y, x);
    }
};

std::pair<std::vector<Point>, std::vector<Point>> separate_boundaries(std::vector<Point>& cones) {
    if (cones.size() < 2) return {cones, {}};

    Point center1(0, 0), center2(0, 0);
    double max_dist = 0;
    Point p1, p2;

    for (size_t i = 0; i < cones.size(); i++) {
        for (size_t j = i + 1; j < cones.size(); j++) {
            double d = cones[i].distance_to(cones[j]);
            if (d > max_dist) {
                max_dist = d;
                p1 = cones[i];
                p2 = cones[j];
            }
        }
    }
    center1 = p1;
    center2 = p2;

    std::vector<Point> cluster1, cluster2;
    for (int iter = 0; iter < 5; iter++) {
        cluster1.clear();
        cluster2.clear();
        
        for (const auto& cone : cones) {
            double d1 = cone.distance_to(center1);
            double d2 = cone.distance_to(center2);
            if (d1 < d2) {
                cluster1.push_back(cone);
            } else {
                cluster2.push_back(cone);
            }
        }

        if (!cluster1.empty()) {
            double sum_x = 0, sum_y = 0;
            for (const auto& p : cluster1) {
                sum_x += p.x;
                sum_y += p.y;
            }
            center1 = Point(sum_x / cluster1.size(), sum_y / cluster1.size());
        }
        
        if (!cluster2.empty()) {
            double sum_x = 0, sum_y = 0;
            for (const auto& p : cluster2) {
                sum_x += p.x;
                sum_y += p.y;
            }
            center2 = Point(sum_x / cluster2.size(), sum_y / cluster2.size());
        }
    }
    
    return {cluster1, cluster2};
}

void sort_by_angle(std::vector<Point>& points) {
    std::sort(points.begin(), points.end(), [](const Point& a, const Point& b) {
        return a.angle_from_origin() < b.angle_from_origin();
    });
}

Point catmull_rom_interpolate(const Point& p0, const Point& p1, const Point& p2, const Point& p3, double t) {
    double t2 = t * t;
    double t3 = t2 * t;
    
    double x = 0.5 * (
        2.0 * p1.x +
        (-p0.x + p2.x) * t +
        (2.0 * p0.x - 5.0 * p1.x + 4.0 * p2.x - p3.x) * t2 +
        (-p0.x + 3.0 * p1.x - 3.0 * p2.x + p3.x) * t3
    );
    
    double y = 0.5 * (
        2.0 * p1.y +
        (-p0.y + p2.y) * t +
        (2.0 * p0.y - 5.0 * p1.y + 4.0 * p2.y - p3.y) * t2 +
        (-p0.y + 3.0 * p1.y - 3.0 * p2.y + p3.y) * t3
    );
    
    return Point(x, y);
}

Point linear_interpolate(const Point& p1, const Point& p2, double t) {
    return Point(p1.x + (p2.x - p1.x) * t, p1.y + (p2.y - p1.y) * t);
}

std::vector<Point> generate_waypoints(std::vector<Point>& inner, std::vector<Point>& outer) {
    std::vector<Point> waypoints;
    const double MAX_WAYPOINT_SPACING = 0.5;

    size_t n = std::min(inner.size(), outer.size());
    
    for (size_t i = 0; i < n; i++) {
        size_t next_i = (i + 1) % n;
        
        Point mid1 = inner[i].midpoint(outer[i]);
        Point mid2 = inner[next_i].midpoint(outer[next_i]);
        
        waypoints.push_back(mid1);

        double dist = mid1.distance_to(mid2);
        int num_segments = static_cast<int>(std::ceil(dist / MAX_WAYPOINT_SPACING));
        
        if (num_segments > 1) {
            for (int seg = 1; seg < num_segments; seg++) {
                double t = static_cast<double>(seg) / num_segments;
                Point interpolated = linear_interpolate(mid1, mid2, t);
                waypoints.push_back(interpolated);
            }
        }
    }
    
    return waypoints;
}

std::vector<Point> read_csv(const std::string& filename) {
    std::vector<Point> points;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return points;
    }
    
    std::string line;
    bool first_line = true;
    
    while (std::getline(file, line)) {
        if (first_line) {
            first_line = false;
            continue; 
        }
        
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string x_str, y_str;
        
        if (std::getline(ss, x_str, ',') && std::getline(ss, y_str, ',')) {
            try {
                double x = std::stod(x_str);
                double y = std::stod(y_str);
                points.push_back(Point(x, y));
            } catch (...) {
                std::cerr << "Error parsing line: " << line << std::endl;
            }
        }
    }
    
    file.close();
    return points;
}

void write_csv(const std::string& filename, const std::vector<Point>& waypoints) {
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error opening file for writing: " << filename << std::endl;
        return;
    }

    file << "x,y\n";

    for (const auto& point : waypoints) {
        file << std::fixed << std::setprecision(6) << point.x << "," << point.y << "\n";
    }
    
    file.close();
    std::cout << "Generated " << waypoints.size() << " waypoints and saved to " << filename << std::endl;
}

int main(int argc, char* argv[]) {
    
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_track.csv> <output_waypoints.csv>" << std::endl;
        return 1;
    }
    
    std::string input_file = argv[1];
    std::string output_file = argv[2];

    std::vector<Point> cones = read_csv(input_file);
    
    if (cones.size() < 4) {
        std::cerr << "Error: Need at least 4 cones" << std::endl;
        return 1;
    }
    
    std::cout << "Read " << cones.size() << " cones from " << input_file << std::endl;

    auto [inner, outer] = separate_boundaries(cones);

    sort_by_angle(inner);
    sort_by_angle(outer);
    
    std::cout << "Separated into " << inner.size() << " inner and " << outer.size() << " outer cones" << std::endl;

    std::vector<Point> waypoints = generate_waypoints(inner, outer);

    write_csv(output_file, waypoints);
    
    return 0;
}
#ifndef TABLE_DETECTOR_H
#define TABLE_DETECTOR_H

#include <vector>
#include <opencv2/opencv.hpp>

struct TableGrid {
    cv::Rect bounds;
    std::vector<int> row_positions;
    std::vector<int> col_positions;
    std::vector<std::vector<cv::Rect>> cells;
    int rows = 0;
    int cols = 0;
};

class TableDetector {
public:
    cv::Mat detectHorizontalLines(const cv::Mat& binary, int min_length = 0);
    cv::Mat detectVerticalLines(const cv::Mat& binary, int min_length = 0);
    std::vector<cv::Point> findIntersections(const cv::Mat& h_lines, const cv::Mat& v_lines);
    TableGrid buildGrid(const cv::Mat& binary);
    std::vector<cv::Mat> extractCells(const cv::Mat& img, const TableGrid& grid);
    std::vector<TableGrid> detectAllTables(const cv::Mat& input);

private:
    std::vector<int> clusterPositions(std::vector<int>& positions, int threshold = 10);
};

#endif // TABLE_DETECTOR_H

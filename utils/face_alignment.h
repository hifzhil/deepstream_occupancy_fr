#ifndef FACE_ALIGNMENT_H
#define FACE_ALIGNMENT_H

#include <opencv2/opencv.hpp>
#include <vector>

cv::Mat alignFace(const cv::Mat& image, const std::vector<float>& landmarks) {
    // Define the target face points (normalized coordinates)
    cv::Point2f dstPoints[5] = {
        cv::Point2f(0.31556875000000000, 0.4615741071428571),
        cv::Point2f(0.68262291666666670, 0.4615741071428571),
        cv::Point2f(0.50026249999999990, 0.6405053571428571),
        cv::Point2f(0.34947187500000004, 0.8246919642857142),
        cv::Point2f(0.65343645833333330, 0.8246919642857142)
    };

    // Convert landmarks to source points
    cv::Point2f srcPoints[5];
    for (int i = 0; i < 5; ++i) {
        srcPoints[i] = cv::Point2f(landmarks[2 * i], landmarks[2 * i + 1]);
    }

    // Convert points to cv::Mat
    cv::Mat srcMat(5, 1, CV_32FC2, srcPoints);
    cv::Mat dstMat(5, 1, CV_32FC2, dstPoints);

    // Estimate the affine transformation
    cv::Mat transform = cv::estimateAffinePartial2D(srcMat, dstMat);

    // Apply the transformation
    cv::Mat aligned;
    cv::warpAffine(image, aligned, transform, cv::Size(112, 112));

    return aligned;
}

#endif // FACE_ALIGNMENT_H 
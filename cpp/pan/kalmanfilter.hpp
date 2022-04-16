#ifndef KALMANFILTER_H
#define KALMANFILTER_H

#include "dataType.hpp"

/*
*   A simple Kalman filter for tracking bounding boxes in image space.
* 
*   The 8-dimensional state space
* 
*       x, y, a, h, vx, vy, va, vh
* 
*   contains the bounding box center position (x, y), aspect ratio a, height h,
*   and their respective velocities.
* 
*   Object motion follows a constant velocity model. The bounding box location
*   (x, y, a, h) is taken as direct observation of the state space (linear
*   observation model).
*/
class KalmanFilter
{
public:
    static const double chi2inv95[10];
    KalmanFilter();

    // Create track from unassociated measurement.
    // measurement : Bounding box coordinates (x, y, a, h) with center position (x, y), aspect ratio a, and height h.
    // Return : Returns the mean vector (8 dimensional) and covariance matrix (8x8 dimensional) of the new track.Unobserved velocities are initialized to 0 mean.
    KAL_DATA initiate(const DETECTBOX& measurement);

    // mean: 8 dimensional object state    at the previous time step.
    // covariance: 8x8 dimensional covariance matrix of the object state    at the previous time step.
    // retuurn: Returns the mean vector and covariance matrix of the predicted state.
    void predict(KAL_MEAN& mean, KAL_COVA& covariance);

    // Project state distribution to measurement space.
    KAL_HDATA project(const KAL_MEAN& mean, const KAL_COVA& covariance);

    // after predict
    // Perform measurement update and track management.
    // mean: The predicted state's mean vector (8 dimensional).
    // covariance: The state's covariance matrix (8x8 dimensional).
    // Returns: measurement-corrected state distribution.
    KAL_DATA update(const KAL_MEAN& mean,
        const KAL_COVA& covariance,
        const DETECTBOX& measurement);

    // Compute gating distance between state distribution and measurements.
    Eigen::Matrix<float, 1, -1> gating_distance(
        const KAL_MEAN& mean,
        const KAL_COVA& covariance,
        const std::vector<DETECTBOX>& measurements,
        bool only_position = false);

private:
    Eigen::Matrix<float, 8, 8, Eigen::RowMajor> _motion_mat;
    Eigen::Matrix<float, 4, 8, Eigen::RowMajor> _update_mat;
    float _std_weight_position;
    float _std_weight_velocity;
};

#endif // KALMANFILTER_H

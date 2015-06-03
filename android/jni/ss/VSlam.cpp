#include "../ss/VSlam.hpp"
#include "NativeLogging.hpp"

using namespace cv;
using namespace std;

namespace vslam
{
    static const char* TAG = "VSlam";
    
    VSlam::VSlam()
    {
        LoadIntrinsicParameters();
        
        Mat init_camera_rot = Mat::eye(3, 3, CV_64F);
        Mat init_camera_pos = Mat::zeros(3, 1, CV_64F);
        world_camera_rot.push_back(init_camera_rot);
        world_camera_pos.push_back(init_camera_pos);
        
        orb_handler = new ORB(500, true);
        Tracking::SetOrbHandler(orb_handler);
        
        curr_state = NOT_INITIALIZED;
    }
    
    void VSlam::ProcessFrame(cv::Mat &img)
    {
        Mat frame;
        cvtColor(img, frame, CV_RGB2GRAY);
        
        if (curr_state == NOT_INITIALIZED)
        {
            LOG_DEBUG(TAG, "NOT_INITIALIZED");

            initial_frame = frame.clone();
            curr_state = INITIALIZING;
        }
        
        if (curr_state == INITIALIZING)
        {
            LOG_DEBUG(TAG, "INITIALIZING");

            if(initializer.InitializeMap(orb_handler, initial_frame, frame, keyframes))
            {
                AppendCameraPose(keyframes.back().GetRotation(), keyframes.back().GetTranslation());
                curr_state = TRACKING;
            }
        }
        
        if (curr_state == TRACKING)
        {
            LOG_DEBUG(TAG, "TRACKING");

            Mat R_vec = world_camera_rot.back().clone();
            Mat t_vec = world_camera_pos.back().clone();
            
            bool new_kf_added = false;
            bool is_lost = !Tracking::TrackMap(frame, keyframes, R_vec, t_vec, new_kf_added);
            
            if (!is_lost)
            {
                AppendCameraPose(R_vec, t_vec);
            }
            else
            {
                curr_state = LOST;
            }
        }
        
        if (curr_state == LOST)
        {
            LOG_DEBUG(TAG, "LOST");            
            // TODO: handle relocalization
        }
    }
    
    void VSlam::AppendCameraPose(Mat rot, Mat pos)
    {
        world_camera_rot.push_back(rot);
        world_camera_pos.push_back(pos);
    }

    void VSlam::LoadIntrinsicParameters()
    {
        // FileStorage fs("../ss/CameraIntrinsics.yaml", FileStorage::READ);

        // if (!fs.isOpened())
        // {
        //     CV_Error(0, "VSlam: Could not load calibration file");
        // }

        // fs["cameraMatrix"] >> camera_matrix;
        // fs["distCoeffs"] >> dist_coeff;
        // fs["imageSize"] >> img_size;

        // The following is hard-coded for demo purposes on Wed 6/3
        camera_matrix = Mat::zeros(3, 3, CV_64F);
        camera_matrix.at<double>(0, 0) = 554.26;
        camera_matrix.at<double>(0, 1) = 0;
        camera_matrix.at<double>(0, 2) = 320;
        camera_matrix.at<double>(1, 0) = 0;
        camera_matrix.at<double>(1, 1) = 554.26;
        camera_matrix.at<double>(1, 2) = 240;
        camera_matrix.at<double>(2, 0) = 0;
        camera_matrix.at<double>(2, 1) = 0;
        camera_matrix.at<double>(2, 2) = 1;

        dist_coeff = Mat::zeros(1, 5, CV_64F);

        img_size = Mat::zeros(1, 2, CV_64F);
        img_size.at<double>(0, 0) = 640;
        img_size.at<double>(0, 1) = 480;
    }

}
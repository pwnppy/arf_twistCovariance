#ifndef GNSS_TO_MAP_HPP_
#define GNSS_TO_MAP_HPP_

#include "rclcpp/rclcpp.hpp"

#include <memory>
#include <geographic_msgs/msg/geo_point.hpp>

#include <geodesy/utm.h>
#include <sensor_msgs/msg/nav_sat_fix.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>

#include "tf2/exceptions.h"
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/buffer.h"
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

#include "math.h"

#define UTM2MGRS 100000

class Gnss_to_map : public rclcpp::Node
{
public:
    explicit Gnss_to_map();

private:
    void pose_callback(const geometry_msgs::msg::PoseWithCovarianceStamped::ConstSharedPtr navsat_msg);

    rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr fix_sub_;
    rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr map_pose_pub_;
    
    geometry_msgs::msg::PoseWithCovarianceStamped gnss2map_msg;
    geometry_msgs::msg::PoseWithCovarianceStamped gnss_msg;

    std::shared_ptr<tf2_ros::TransformListener> tf_listener_{nullptr};
    std::unique_ptr<tf2_ros::Buffer> tf_buffer_;

    std::string target_frame;
    std::string gnss_frame;
};


#endif  // GNSS_TO_MAP_HPP_

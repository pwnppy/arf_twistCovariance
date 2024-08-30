#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
#include <geometry_msgs/msg/twist_with_covariance_stamped.hpp>  // Updated to include TwistWithCovarianceStamped
#include <tf2_ros/transform_broadcaster.h>
#include <tf2/LinearMath/Quaternion.h>
#include <Eigen/Dense>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>

class PoseFusionNode : public rclcpp::Node
{
public:
    PoseFusionNode()
        : Node("pose_fusion_node")
    {
        // Subscribers for LiDAR and GNSS pose
        lidar_pose_sub_ = this->create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
            "/localization/pose_with_covariance", 10,
            std::bind(&PoseFusionNode::lidarPoseCallback, this, std::placeholders::_1));

        gnss_pose_sub_ = this->create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
            "/fix_pose", 10,
            std::bind(&PoseFusionNode::gnssPoseCallback, this, std::placeholders::_1));

        // Subscribers for EKF and Filter twist (now TwistWithCovarianceStamped)
        ekf_twist_sub_ = this->create_subscription<geometry_msgs::msg::TwistWithCovarianceStamped>(
            "/localization/pose_twist_fusion_filter/twist_with_covariance", 10,
            std::bind(&PoseFusionNode::ekfTwistCallback, this, std::placeholders::_1));

        filter_twist_sub_ = this->create_subscription<geometry_msgs::msg::TwistWithCovarianceStamped>(
            "/fix_twist", 10,
            std::bind(&PoseFusionNode::filterTwistCallback, this, std::placeholders::_1));

        // Publisher for final fused pose and fused twist (now TwistWithCovarianceStamped)
        final_pose_pub_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>("/final/pose_with_covariance", 10);
        fused_twist_pub_ = this->create_publisher<geometry_msgs::msg::TwistWithCovarianceStamped>("/fused_twist", 10);

        // Initialize the transform broadcaster
        tf_broadcaster_ = std::make_shared<tf2_ros::TransformBroadcaster>(this);
    }

private:
    void lidarPoseCallback(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr lidar_msg)
    {
        last_lidar_msg_ = lidar_msg;

        if (last_gnss_msg_)
        {
            fusePoses();
        }
    }

    void gnssPoseCallback(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr gnss_msg)
    {
        last_gnss_msg_ = gnss_msg;

        if (last_lidar_msg_)
        {
            fusePoses();
        }
    }

    void ekfTwistCallback(const geometry_msgs::msg::TwistWithCovarianceStamped::SharedPtr ekf_twist_msg)
    {
        last_ekf_twist_msg_ = ekf_twist_msg;

        if (last_filter_twist_msg_)
        {
            fuseTwists();
        }
    }

    void filterTwistCallback(const geometry_msgs::msg::TwistWithCovarianceStamped::SharedPtr filter_twist_msg)
    {
        last_filter_twist_msg_ = filter_twist_msg;

        if (last_ekf_twist_msg_)
        {
            fuseTwists();
        }
    }

    void fusePoses()
    {
        Eigen::Vector3d lidar_pos(last_lidar_msg_->pose.pose.position.x, last_lidar_msg_->pose.pose.position.y, last_lidar_msg_->pose.pose.position.z);
        Eigen::Vector3d gnss_pos(last_gnss_msg_->pose.pose.position.x, last_gnss_msg_->pose.pose.position.y, last_gnss_msg_->pose.pose.position.z);

        geometry_msgs::msg::PoseWithCovarianceStamped fused_pose;
        fused_pose.header.stamp = this->now();
        fused_pose.header.frame_id = "map";

        fused_pose.pose.pose.position.x = lidar_weight_ * lidar_pos.x() + gnss_weight_ * gnss_pos.x();
        fused_pose.pose.pose.position.y = lidar_weight_ * lidar_pos.y() + gnss_weight_ * gnss_pos.y();
        fused_pose.pose.pose.position.z = lidar_weight_ * lidar_pos.z() + gnss_weight_ * gnss_pos.z();

        fused_pose.pose.pose.orientation = last_lidar_msg_->pose.pose.orientation;

        for (size_t i = 0; i < 36; ++i)
        {
            fused_pose.pose.covariance[i] = lidar_weight_ * last_lidar_msg_->pose.covariance[i] +
                                            gnss_weight_ * last_gnss_msg_->pose.covariance[i];
        }

        final_pose_pub_->publish(fused_pose);

        // Broadcast the transform
        broadcastTransform(fused_pose);
    }

    void fuseTwists()
    {
        geometry_msgs::msg::TwistWithCovarianceStamped fused_twist;
        fused_twist.header.stamp = this->now();
        fused_twist.header.frame_id = "map";  // Adjust frame_id as needed

        // Linear twist values (assuming no linear motion in this context)
        fused_twist.twist.twist.linear.x = 0.0;
        fused_twist.twist.twist.linear.y = 0.0;
        fused_twist.twist.twist.linear.z = 0.0;

        // Angular twist: Z component
        fused_twist.twist.twist.angular.x = 0.0;
        fused_twist.twist.twist.angular.y = 0.0;
        fused_twist.twist.twist.angular.z = ekf_twist_weight_ * last_ekf_twist_msg_->twist.twist.angular.z + 
                                            filter_twist_weight_ * last_filter_twist_msg_->twist.twist.angular.z;

        // Covariance fusion (simple weighted sum of covariances)
        for (size_t i = 0; i < 36; ++i)
        {
            fused_twist.twist.covariance[i] = ekf_twist_weight_ * last_ekf_twist_msg_->twist.covariance[i] + 
                                              filter_twist_weight_ * last_filter_twist_msg_->twist.covariance[i];
        }

        fused_twist_pub_->publish(fused_twist);
    }

    void broadcastTransform(const geometry_msgs::msg::PoseWithCovarianceStamped &fused_pose)
    {
        geometry_msgs::msg::TransformStamped transformStamped;

        transformStamped.header.stamp = fused_pose.header.stamp;
        transformStamped.header.frame_id = "map";  // Adjust this to the correct reference frame as needed
        transformStamped.child_frame_id = "base_link";

        transformStamped.transform.translation.x = fused_pose.pose.pose.position.x;
        transformStamped.transform.translation.y = fused_pose.pose.pose.position.y;
        transformStamped.transform.translation.z = fused_pose.pose.pose.position.z;

        transformStamped.transform.rotation.x = fused_pose.pose.pose.orientation.x;
        transformStamped.transform.rotation.y = fused_pose.pose.pose.orientation.y;
        transformStamped.transform.rotation.z = fused_pose.pose.pose.orientation.z;
        transformStamped.transform.rotation.w = fused_pose.pose.pose.orientation.w;

        // Broadcast the transform
        tf_broadcaster_->sendTransform(transformStamped);
    }

    rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr lidar_pose_sub_;
    rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr gnss_pose_sub_;
    rclcpp::Subscription<geometry_msgs::msg::TwistWithCovarianceStamped>::SharedPtr ekf_twist_sub_;
    rclcpp::Subscription<geometry_msgs::msg::TwistWithCovarianceStamped>::SharedPtr filter_twist_sub_;
    rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr final_pose_pub_;
    rclcpp::Publisher<geometry_msgs::msg::TwistWithCovarianceStamped>::SharedPtr fused_twist_pub_;

    std::shared_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;

    geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr last_lidar_msg_;
    geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr last_gnss_msg_;
    geometry_msgs::msg::TwistWithCovarianceStamped::SharedPtr last_ekf_twist_msg_;
    geometry_msgs::msg::TwistWithCovarianceStamped::SharedPtr last_filter_twist_msg_;

    double lidar_weight_ = 0.5; // Weight for LiDAR data
    double gnss_weight_ = 0.5;  // Weight for GNSS data
    double ekf_twist_weight_ = 0.5; // Weight for EKF twist data
    double filter_twist_weight_ = 0.5; // Weight for Filter twist data
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<PoseFusionNode>());
    rclcpp::shutdown();
    return 0;
}
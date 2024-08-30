#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include <cmath>

using std::placeholders::_1;

class PoseCovariancePublisher : public rclcpp::Node
{
public:
  PoseCovariancePublisher()
  : Node("pose_covariance_publisher")
  {
    // GNSS pose 구독 및 콜백 등록
    gnss_pose_subscription_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
      "/gnss_pose", 10, std::bind(&PoseCovariancePublisher::gnss_pose_callback, this, _1));

    // /gnss_pose_with_covariance 및 /lidar_pose_with_covariance 퍼블리셔 생성
    gnss_pose_with_covariance_publisher_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
      "/gnss_pose_with_covariance", 10);

    lidar_pose_with_covariance_publisher_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
      "/lidar_pose_with_covariance", 10);
  }

private:
  void gnss_pose_callback(const geometry_msgs::msg::PoseStamped::SharedPtr msg)
  {
    auto pose_with_covariance_msg = geometry_msgs::msg::PoseWithCovarianceStamped();

    pose_with_covariance_msg.header = msg->header;
    pose_with_covariance_msg.pose.pose = msg->pose;

    // 예시 공분산 행렬 (단위 행렬)
    for (int i = 0; i < 36; ++i) {
      pose_with_covariance_msg.pose.covariance[i] = (i % 7 == 0) ? 0.1 : 0.0;
    }

    gnss_pose_with_covariance_publisher_->publish(pose_with_covariance_msg);

    // LiDAR 공분산 메시지 발행
    lidar_pose_with_covariance_publisher_->publish(pose_with_covariance_msg);
  }

  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr gnss_pose_subscription_;
  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr gnss_pose_with_covariance_publisher_;
  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr lidar_pose_with_covariance_publisher_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<PoseCovariancePublisher>());
  rclcpp::shutdown();
  return 0;
}

// #include <rclcpp/rclcpp.hpp>
// #include <geometry_msgs/msg/pose_stamped.hpp>
// #include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
// #include <sensor_msgs/msg/point_cloud2.hpp>
// #include <tf2_geometry_msgs/tf2_geometry_msgs.h>
// #include <tf2/LinearMath/Quaternion.h>
// #include <Eigen/Dense>
// #include <pcl/point_cloud.h>
// #include <pcl/point_types.h>
// #include <pcl/filters/voxel_grid.h>
// #include <pcl/filters/statistical_outlier_removal.h>
// #include <pcl/filters/passthrough.h>
// #include <pcl_conversions/pcl_conversions.h>


// class PoseConvertFusionNode : public rclcpp::Node
// {
// public:
//     PoseConvertFusionNode()
//         : Node("pose_convert_fusion_node")
//     {
//         // Subscribers for GNSS pose and LiDAR points
//         gnss_pose_sub_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
//             "/gnss_pose", 10,
//             std::bind(&PoseConvertFusionNode::gnssPoseCallback, this, std::placeholders::_1));

//         lidar_points_sub_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
//             "/ouster/points", 10,
//             std::bind(&PoseConvertFusionNode::lidarPointsCallback, this, std::placeholders::_1));

//         // Publishers for PoseWithCovarianceStamped
//         gnss_pose_with_covariance_pub_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
//             "/gnss_pose_with_covariance", 10);

//         lidar_pose_with_covariance_pub_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
//             "/lidar_pose_with_covariance", 10);
//     }

// private:
//     void gnssPoseCallback(const geometry_msgs::msg::PoseStamped::SharedPtr msg)
//     {
//         // Convert PoseStamped to PoseWithCovarianceStamped with dummy covariance
//         geometry_msgs::msg::PoseWithCovarianceStamped pose_with_covariance;
//         pose_with_covariance.header = msg->header;
//         pose_with_covariance.pose.pose = msg->pose;

//         // Set covariance to some default value (diagonal matrix here)
//         pose_with_covariance.pose.covariance.fill(0.0);
//         pose_with_covariance.pose.covariance[0] = 0.1; // Example covariance
//         pose_with_covariance.pose.covariance[7] = 0.1; // Example covariance
//         pose_with_covariance.pose.covariance[14] = 0.1; // Example covariance
//         pose_with_covariance.pose.covariance[21] = 0.1; // Example covariance
//         pose_with_covariance.pose.covariance[28] = 0.1; // Example covariance
//         pose_with_covariance.pose.covariance[35] = 0.1; // Example covariance

//         gnss_pose_with_covariance_pub_->publish(pose_with_covariance);
//     }

    // void lidarPointsCallback(const sensor_msgs::msg::PointCloud2::SharedPtr msg)
    // {
    //     // Convert PointCloud2 to PCL PointCloud
    //     pcl::PCLPointCloud2 pcl_pc2;
    //     pcl_conversions::toPCL(*msg, pcl_pc2);
    //     pcl::PointCloud<pcl::PointXYZ> pcl_cloud;
    //     pcl::fromPCLPointCloud2(pcl_pc2, pcl_cloud);

    //     // Pre-process the point cloud (optional)
    //     pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZ>);
    //     pcl::VoxelGrid<pcl::PointXYZ> voxel_filter;
    //     voxel_filter.setInputCloud(pcl_cloud.makeShared());
    //     voxel_filter.setLeafSize(0.1f, 0.1f, 0.1f);
    //     voxel_filter.filter(*cloud_filtered);

    //     // Compute the centroid of the point cloud as a simple pose estimation
    //     Eigen::Vector4f centroid;
    //     pcl::compute3DCentroid(*cloud_filtered, centroid);

    //     // Create the PoseWithCovarianceStamped message
    //     geometry_msgs::msg::PoseWithCovarianceStamped lidar_pose_with_covariance;
    //     lidar_pose_with_covariance.header.stamp = this->now();
    //     lidar_pose_with_covariance.header.frame_id = "map";

    //     lidar_pose_with_covariance.pose.pose.position.x = centroid[0];
    //     lidar_pose_with_covariance.pose.pose.position.y = centroid[1];
    //     lidar_pose_with_covariance.pose.pose.position.z = centroid[2];
    //     lidar_pose_with_covariance.pose.pose.orientation.x = 0.0;
    //     lidar_pose_with_covariance.pose.pose.orientation.y = 0.0;
    //     lidar_pose_with_covariance.pose.pose.orientation.z = 0.0;
    //     lidar_pose_with_covariance.pose.pose.orientation.w = 1.0;

    //     // Set dummy covariance
    //     lidar_pose_with_covariance.pose.covariance.fill(0.0);
    //     lidar_pose_with_covariance.pose.covariance[0] = 0.1; // Example covariance
    //     lidar_pose_with_covariance.pose.covariance[7] = 0.1; // Example covariance
    //     lidar_pose_with_covariance.pose.covariance[14] = 0.1; // Example covariance
    //     lidar_pose_with_covariance.pose.covariance[21] = 0.1; // Example covariance
    //     lidar_pose_with_covariance.pose.covariance[28] = 0.1; // Example covariance
    //     lidar_pose_with_covariance.pose.covariance[35] = 0.1; // Example covariance

    //     lidar_pose_with_covariance_pub_->publish(lidar_pose_with_covariance);
    // }

//     void lidarPointsCallback(const sensor_msgs::msg::PointCloud2::SharedPtr msg)
// {
//     // Convert PointCloud2 to PCL PointCloud
//     pcl::PCLPointCloud2 pcl_pc2;
//     pcl_conversions::toPCL(*msg, pcl_pc2);
//     pcl::PointCloud<pcl::PointXYZ> pcl_cloud;
//     pcl::fromPCLPointCloud2(pcl_pc2, pcl_cloud);

//     // Pre-process the point cloud (optional)
//     pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZ>);
//     pcl::VoxelGrid<pcl::PointXYZ> voxel_filter;
//     voxel_filter.setInputCloud(pcl_cloud.makeShared());
//     voxel_filter.setLeafSize(0.1f, 0.1f, 0.1f);
//     voxel_filter.filter(*cloud_filtered);

//     // Compute the centroid of the point cloud as a simple pose estimation
//     Eigen::Vector4f centroid = Eigen::Vector4f::Zero();
//     if (!cloud_filtered->empty()) {
//         Eigen::Vector4f sum = Eigen::Vector4f::Zero();
//         for (const auto& point : cloud_filtered->points) {
//             sum[0] += point.x;
//             sum[1] += point.y;
//             sum[2] += point.z;
//         }
//         centroid[0] = sum[0] / cloud_filtered->points.size();
//         centroid[1] = sum[1] / cloud_filtered->points.size();
//         centroid[2] = sum[2] / cloud_filtered->points.size();
//         centroid[3] = 1.0; // Homogeneous coordinate
//     }

//     // Create the PoseWithCovarianceStamped message
//     geometry_msgs::msg::PoseWithCovarianceStamped lidar_pose_with_covariance;
//     lidar_pose_with_covariance.header.stamp = this->now();
//     lidar_pose_with_covariance.header.frame_id = "map";

//     lidar_pose_with_covariance.pose.pose.position.x = centroid[0];
//     lidar_pose_with_covariance.pose.pose.position.y = centroid[1];
//     lidar_pose_with_covariance.pose.pose.position.z = centroid[2];
//     lidar_pose_with_covariance.pose.pose.orientation.x = 0.0;
//     lidar_pose_with_covariance.pose.pose.orientation.y = 0.0;
//     lidar_pose_with_covariance.pose.pose.orientation.z = 0.0;
//     lidar_pose_with_covariance.pose.pose.orientation.w = 1.0;

//     // Set dummy covariance
//     lidar_pose_with_covariance.pose.covariance.fill(0.0);
//     lidar_pose_with_covariance.pose.covariance[0] = 0.1; // Example covariance
//     lidar_pose_with_covariance.pose.covariance[7] = 0.1; // Example covariance
//     lidar_pose_with_covariance.pose.covariance[14] = 0.1; // Example covariance
//     lidar_pose_with_covariance.pose.covariance[21] = 0.1; // Example covariance
//     lidar_pose_with_covariance.pose.covariance[28] = 0.1; // Example covariance
//     lidar_pose_with_covariance.pose.covariance[35] = 0.1; // Example covariance

//     lidar_pose_with_covariance_pub_->publish(lidar_pose_with_covariance);
// }



//     rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr gnss_pose_sub_;
//     rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr lidar_points_sub_;
//     rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr gnss_pose_with_covariance_pub_;
//     rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr lidar_pose_with_covariance_pub_;
// };

// int main(int argc, char *argv[])
// {
//     rclcpp::init(argc, argv);
//     rclcpp::spin(std::make_shared<PoseConvertFusionNode>());
//     rclcpp::shutdown();
//     return 0;
// }

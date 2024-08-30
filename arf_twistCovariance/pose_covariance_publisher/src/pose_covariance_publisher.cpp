// #include "rclcpp/rclcpp.hpp"
// #include "geometry_msgs/msg/pose_stamped.hpp"
// #include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
// #include "geometry_msgs/msg/twist_stamped.hpp"
// #include <cmath>

// using std::placeholders::_1;

// class PoseCovariancePublisher : public rclcpp::Node
// {
// public:
//   PoseCovariancePublisher()
//   : Node("pose_covariance_publisher"), last_yaw_(0.0), first_yaw_received_(false)
//   {
//     // GNSS pose 구독 및 콜백 등록
//     gnss_pose_subscription_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
//       "/gnss_pose", 10, std::bind(&PoseCovariancePublisher::gnss_pose_callback, this, _1));

//     // /gnss_pose_with_covariance 및 /lidar_pose_with_covariance 퍼블리셔 생성
//     gnss_pose_with_covariance_publisher_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
//       "/gnss_pose_with_covariance", 10);

//     lidar_pose_with_covariance_publisher_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
//       "/lidar_pose_with_covariance", 10);

//     // /fix_twist 퍼블리셔 생성
//     fix_twist_publisher_ = this->create_publisher<geometry_msgs::msg::TwistStamped>(
//       "/fix_twist", 10);
//   }

// private:
//   void gnss_pose_callback(const geometry_msgs::msg::PoseStamped::SharedPtr msg)
//   {
//     auto pose_with_covariance_msg = geometry_msgs::msg::PoseWithCovarianceStamped();

//     pose_with_covariance_msg.header = msg->header;
//     pose_with_covariance_msg.pose.pose = msg->pose;

//     // 예시 공분산 행렬 (단위 행렬)
//     for (int i = 0; i < 36; ++i) {
//       pose_with_covariance_msg.pose.covariance[i] = (i % 7 == 0) ? 0.1 : 0.0;
//     }

//     gnss_pose_with_covariance_publisher_->publish(pose_with_covariance_msg);

//     // LiDAR 공분산 메시지 발행
//     lidar_pose_with_covariance_publisher_->publish(pose_with_covariance_msg);

//     // 현재 yaw 계산
//     double current_yaw = calculate_yaw(msg->pose.orientation);

//     // 첫 번째 yaw 수신 시에는 이전 yaw를 설정만 하고, 이후에 처리
//     if (!first_yaw_received_) {
//       last_yaw_ = current_yaw;
//       last_time_ = rclcpp::Time(msg->header.stamp);  // 현재 시간을 rclcpp::Time으로 변환하여 저장
//       first_yaw_received_ = true;
//       return;
//     }

//     // 시간 간격 계산 (시간 변화량)
//     rclcpp::Time current_time(msg->header.stamp);  // 현재 시간을 rclcpp::Time으로 변환
//     double dt = (current_time - last_time_).seconds();  // 시간 간격 계산

//     // Yaw 변화 계산
//     double yaw_diff = normalize_angle(current_yaw - last_yaw_);

//     // TwistStamped 메시지 생성 및 발행
//     auto twist_msg = geometry_msgs::msg::TwistStamped();
//     twist_msg.header.stamp = msg->header.stamp;
//     twist_msg.header.frame_id = "base_link";  // 필요에 따라 프레임 ID 조정

//     // Linear 속성들은 0으로 설정
//     twist_msg.twist.linear.x = 0.0;
//     twist_msg.twist.linear.y = 0.0;
//     twist_msg.twist.linear.z = 0.0;

//     // Angular 속성 중 z축 각속도 계산
//     twist_msg.twist.angular.x = 0.0;
//     twist_msg.twist.angular.y = 0.0;
//     twist_msg.twist.angular.z = yaw_diff / dt;

//     fix_twist_publisher_->publish(twist_msg);

//     // 업데이트
//     last_yaw_ = current_yaw;
//     last_time_ = current_time;
//   }

//   double calculate_yaw(const geometry_msgs::msg::Quaternion &quat)
//   {
//     // 쿼터니언을 사용하여 yaw (방위각) 계산
//     double siny_cosp = 2.0 * (quat.w * quat.z + quat.x * quat.y);
//     double cosy_cosp = 1.0 - 2.0 * (quat.y * quat.y + quat.z * quat.z);
//     return std::atan2(siny_cosp, cosy_cosp);
//   }

//   double normalize_angle(double angle)
//   {
//     // 각도를 -π에서 π 사이로 정규화
//     while (angle > M_PI) angle -= 2.0 * M_PI;
//     while (angle < -M_PI) angle += 2.0 * M_PI;
//     return angle;
//   }

//   rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr gnss_pose_subscription_;
//   rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr gnss_pose_with_covariance_publisher_;
//   rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr lidar_pose_with_covariance_publisher_;
//   rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr fix_twist_publisher_;

//   double last_yaw_;
//   rclcpp::Time last_time_;
//   bool first_yaw_received_;
// };

// int main(int argc, char * argv[])
// {
//   rclcpp::init(argc, argv);
//   rclcpp::spin(std::make_shared<PoseCovariancePublisher>());
//   rclcpp::shutdown();
//   return 0;
// }



#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include "geometry_msgs/msg/twist_with_covariance_stamped.hpp"  // Update the include
#include <cmath>

using std::placeholders::_1;

class PoseCovariancePublisher : public rclcpp::Node
{
public:
  PoseCovariancePublisher()
  : Node("pose_covariance_publisher"), last_yaw_(0.0), first_yaw_received_(false)
  {
    // GNSS pose 구독 및 콜백 등록
    gnss_pose_subscription_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
      "/gnss_pose", 10, std::bind(&PoseCovariancePublisher::gnss_pose_callback, this, _1));

    // /gnss_pose_with_covariance 및 /lidar_pose_with_covariance 퍼블리셔 생성
    gnss_pose_with_covariance_publisher_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
      "/gnss_pose_with_covariance", 10);

    lidar_pose_with_covariance_publisher_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
      "/lidar_pose_with_covariance", 10);

    // /fix_twist 퍼블리셔를 TwistWithCovarianceStamped로 생성
    fix_twist_publisher_ = this->create_publisher<geometry_msgs::msg::TwistWithCovarianceStamped>(
      "/fix_twist", 10);
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

    // 현재 yaw 계산
    double current_yaw = calculate_yaw(msg->pose.orientation);

    // 첫 번째 yaw 수신 시에는 이전 yaw를 설정만 하고, 이후에 처리
    if (!first_yaw_received_) {
      last_yaw_ = current_yaw;
      last_time_ = rclcpp::Time(msg->header.stamp);  // 현재 시간을 rclcpp::Time으로 변환하여 저장
      first_yaw_received_ = true;
      return;
    }

    // 시간 간격 계산 (시간 변화량)
    rclcpp::Time current_time(msg->header.stamp);  // 현재 시간을 rclcpp::Time으로 변환
    double dt = (current_time - last_time_).seconds();  // 시간 간격 계산

    // Yaw 변화 계산
    double yaw_diff = normalize_angle(current_yaw - last_yaw_);

    // TwistWithCovarianceStamped 메시지 생성 및 발행
    auto twist_msg = geometry_msgs::msg::TwistWithCovarianceStamped();
    twist_msg.header.stamp = msg->header.stamp;
    twist_msg.header.frame_id = "base_link";  // 필요에 따라 프레임 ID 조정

    // Linear 속성들은 0으로 설정
    twist_msg.twist.twist.linear.x = 0.0;
    twist_msg.twist.twist.linear.y = 0.0;
    twist_msg.twist.twist.linear.z = 0.0;

    // Angular 속성 중 z축 각속도 계산
    twist_msg.twist.twist.angular.x = 0.0;
    twist_msg.twist.twist.angular.y = 0.0;
    twist_msg.twist.twist.angular.z = yaw_diff / dt;

    // 예시 공분산 행렬 (단위 행렬)
    for (int i = 0; i < 36; ++i) {
      twist_msg.twist.covariance[i] = (i % 7 == 0) ? 0.1 : 0.0;
    }

    fix_twist_publisher_->publish(twist_msg);

    // 업데이트
    last_yaw_ = current_yaw;
    last_time_ = current_time;
  }

  double calculate_yaw(const geometry_msgs::msg::Quaternion &quat)
  {
    // 쿼터니언을 사용하여 yaw (방위각) 계산
    double siny_cosp = 2.0 * (quat.w * quat.z + quat.x * quat.y);
    double cosy_cosp = 1.0 - 2.0 * (quat.y * quat.y + quat.z * quat.z);
    return std::atan2(siny_cosp, cosy_cosp);
  }

  double normalize_angle(double angle)
  {
    // 각도를 -π에서 π 사이로 정규화
    while (angle > M_PI) angle -= 2.0 * M_PI;
    while (angle < -M_PI) angle += 2.0 * M_PI;
    return angle;
  }

  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr gnss_pose_subscription_;
  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr gnss_pose_with_covariance_publisher_;
  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr lidar_pose_with_covariance_publisher_;
  rclcpp::Publisher<geometry_msgs::msg::TwistWithCovarianceStamped>::SharedPtr fix_twist_publisher_;

  double last_yaw_;
  rclcpp::Time last_time_;
  bool first_yaw_received_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<PoseCovariancePublisher>());
  rclcpp::shutdown();
  return 0;
}

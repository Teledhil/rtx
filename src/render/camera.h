#pragma once

#include "glm.h"

namespace rtx {
class camera {
 public:
  camera() : camera(1, 1) {}
  camera(uint32_t width, uint32_t height) : updated_(false) {
    model_ = glm::mat4(1.0f);

    update_aspect_ratio(width, height);
    update_projection();

    update_view();

    // Model
    rotation_ = glm::vec3(-20, 45, 0);

    // Clip
    // Vulkan clip space has inverted Y and half Z.
    clip_ = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,   //
                      0.0f, -1.0f, 0.0f, 0.0f,  //
                      0.0f, 0.0f, 0.5f, 0.0f,   //
                      0.0f, 0.0f, 0.5f, 1.0f    //
    );

    center_ = glm::vec3(0.0, 0.0, 0.0);

    update_mvp();
  }

  void update_window_size(uint32_t width, uint32_t height) {
    update_aspect_ratio(width, height);
    update_projection();
    update_mvp();
  }

  const glm::mat4 &mvp() {
    if (updated_) {
      update_mvp();
      updated_ = false;
    }
    return mvp_;
  }

  const glm::mat4 &view() const { return view_; }
  const glm::mat4 inverse_view() const { return glm::inverse(view_); }

  const glm::mat4 projection() const { return clip_ * projection_; }
  const glm::mat4 inverse_projection() const {
    return glm::inverse(clip_ * projection_);
  }

  void rotate_with_mouse_drag(double x, double y) {
    if (x == 0 && y == 0) {
      return;
    }

    glm::vec3 prev_rotation = rotation_;

    glm::vec3 rotation_delta(y * ROTATION_SPEED, -x * ROTATION_SPEED, 0.0);

    rotation_ += rotation_delta;
    rotation_.x = std::min(rotation_.x, MAX_ROTATION);
    rotation_.x = std::max(rotation_.x, MIN_ROTATION);

    if (rotation_ != prev_rotation) {
      updated_ = true;
    }
  }

  void zoom_with_mouse_wheel(double z) {
    float prev_distance = distance_;

    distance_ += z / 10;
    distance_ = std::max(distance_, MIN_DISTANCE);
    distance_ = std::min(distance_, MAX_DISTANCE);

    if (prev_distance != distance_) {
      updated_ = true;
    }
  }

  void wasd(double key_x, double key_y) {
    auto prev_center = center_;

    auto translation = glm::vec3((key_y * cos(glm::radians(rotation_.y)) -
                                  key_x * sin(glm::radians(rotation_.y))) *
                                     distance_ / 5.0f,
                                 0,
                                 (key_y * sin(glm::radians(rotation_.y)) +
                                  key_x * cos(glm::radians(rotation_.y))) *
                                     distance_ / 5.0f);

    center_ += translation;

    if (center_ != prev_center) {
      updated_ = true;
    }
  }

  bool is_updated() const { return updated_; }

 private:
  float aspect_ratio_;
  static constexpr float MIN_DISTANCE = 0.001f;
  static constexpr float MAX_DISTANCE = 10000.0f;
  static constexpr float MIN_ROTATION = -180.0f;
  static constexpr float MAX_ROTATION = 0.0f;
  static constexpr float FOV = 45.0f;

  static constexpr float ROTATION_SPEED = 0.25f;
  float distance_ = 3.0f;

  glm::mat4 projection_;
  glm::mat4 view_;
  glm::mat4 model_;
  glm::mat4 clip_;
  glm::mat4 mvp_;
  glm::vec3 rotation_;
  glm::vec3 center_;

  bool updated_;

  void update_aspect_ratio(uint32_t width, uint32_t height) {
    aspect_ratio_ = static_cast<float>(width) / static_cast<float>(height);
  }

  void update_projection() {
    projection_ = glm::perspective(glm::radians(FOV), aspect_ratio_,
                                   MIN_DISTANCE, MAX_DISTANCE);
  }

  void update_view() {
    // Look At
    const auto target = glm::vec3(0, 0, 0);
    const auto eye = glm::vec3(0, 0, -distance_);
    const auto up = glm::vec3(0.0, 1.0, 0.0);
    const auto look_at = glm::lookAt(eye, target, up);

    glm::mat4 camera_movement(1.0f);
    camera_movement = glm::rotate(camera_movement, glm::radians(rotation_.x),
                                  glm::vec3(1, 0, 0));
    camera_movement = glm::rotate(camera_movement, glm::radians(rotation_.y),
                                  glm::vec3(0, 1, 0));
    camera_movement = glm::translate(camera_movement, -center_);

    view_ = look_at * camera_movement;
  }

  void update_mvp() {
    update_view();

    model_ = glm::mat4(1.0f);

    mvp_ = clip_ * projection_ * view_ * model_;
  }
};

}  // namespace rtx

#pragma once

#include "glm.h"

namespace rtx {

class camera {
 public:
  camera() : camera(1, 1) {}
  camera(uint32_t width, uint32_t height) : updated_(false) {
    fov_ = 45.0f;

    update_aspect_ratio(width, height);
    update_projection();

    update_view();

    // Model
    rotation_ = glm::vec3(15, 60, 0);

    // Clip
    // Vulkan clip space has inverted Y and half Z.
    clip_ = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,   //
                      0.0f, -1.0f, 0.0f, 0.0f,  //
                      0.0f, 0.0f, 0.5f, 0.0f,   //
                      0.0f, 0.0f, 0.5f, 1.0f    //
    );

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
    glm::vec3 rotation_delta(y * rotation_speed_, -x * rotation_speed_, 0.0);

    rotation_ += rotation_delta;

    std::cout << "Rotation: " << rotation_.x << " " << rotation_.y << std::endl;

    updated_ = true;
  }

  void zoom_with_mouse_wheel(double z) {
    float prev_distance = distance_;

    distance_ += z;
    distance_ = std::max(distance_, near_distance_);
    distance_ = std::min(distance_, far_distance_);

    if (prev_distance != distance_) {
      updated_ = true;
    }
  }

  bool is_updated() const { return updated_; }
  // void update_done() { updated_ = false; }

 private:
  float aspect_ratio_;
  float near_distance_ = 0.001f;
  float far_distance_ = 10000.0f;
  float fov_;

  float rotation_speed_ = 0.25f;
  float distance_ = 2.5f;

  glm::mat4 projection_;
  glm::mat4 view_;
  glm::mat4 model_;
  glm::mat4 clip_;
  glm::mat4 mvp_;
  glm::vec3 rotation_;

  bool updated_;

  void update_aspect_ratio(uint32_t width, uint32_t height) {
    aspect_ratio_ = static_cast<float>(width) / static_cast<float>(height);
  }

  void update_projection() {
    projection_ = glm::perspective(glm::radians(fov_), aspect_ratio_,
                                   near_distance_, far_distance_);
  }

  void update_view() {
    // Look At
    auto eye = glm::vec3(0.0, 2.0, distance_);
    auto center = glm::vec3(0.0, 0.0, 0.0);
    auto up = glm::vec3(0.0, 0.0, 1.0);
    view_ = glm::lookAt(eye, center, up);
    view_ = glm::lookAt(eye, center, up);

    glm::mat4 camera_movement(1.0f);
    camera_movement = glm::rotate(camera_movement, glm::radians(rotation_.x),
                                  glm::vec3(1, 0, 0));
    camera_movement = glm::rotate(camera_movement, glm::radians(rotation_.y),
                                  glm::vec3(0, 0, 1));
    view_ = view_ * camera_movement;
  }

  void update_mvp() {
    update_view();

    model_ = glm::mat4(1.0f);
    // model_ = glm::rotate(model_, glm::radians(rotation_.x), glm::vec3(1, 0,
    // 0)); model_ = glm::rotate(model_, glm::radians(rotation_.y), glm::vec3(0,
    // 0, 1));

    mvp_ = clip_ * projection_ * view_ * model_;
  }
};

}  // namespace rtx

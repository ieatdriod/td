//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2019
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "td/telegram/td_api.h"
#include "td/telegram/telegram_api.h"

#include "td/utils/common.h"
#include "td/utils/Status.h"
#include "td/utils/StringBuilder.h"

namespace td {

string get_color_hex_string(int32 color);

struct GradientInfo {
  int32 top_color = 0;
  int32 bottom_color = 0;

  GradientInfo() = default;
  GradientInfo(int32 top_color, int32 bottom_color) : top_color(top_color), bottom_color(bottom_color) {
  }

  string get_colors_hex_string() const;
};

struct BackgroundType {
  enum class Type : int32 { Wallpaper, Pattern, Solid, Gradient };
  Type type = Type::Solid;
  bool is_blurred = false;
  bool is_moving = false;
  int32 color = 0;
  int32 intensity = 0;
  GradientInfo gradient;

  BackgroundType() = default;
  BackgroundType(bool is_blurred, bool is_moving)
      : type(Type::Wallpaper), is_blurred(is_blurred), is_moving(is_moving) {
  }
  BackgroundType(bool is_moving, int32 color, int32 intensity)
      : type(Type::Pattern), is_moving(is_moving), color(color), intensity(intensity) {
  }
  explicit BackgroundType(int32 color) : type(Type::Solid), color(color) {
  }
  BackgroundType(GradientInfo gradient) : type(Type::Gradient), gradient(gradient) {
  }

  string get_color_hex_string() const;
};

bool operator==(const BackgroundType &lhs, const BackgroundType &rhs);

StringBuilder &operator<<(StringBuilder &string_builder, const BackgroundType &type);

Result<BackgroundType> get_background_type(const td_api::BackgroundType *type);

BackgroundType get_background_type(bool is_pattern, telegram_api::object_ptr<telegram_api::wallPaperSettings> settings);

td_api::object_ptr<td_api::BackgroundType> get_background_type_object(const BackgroundType &type);

telegram_api::object_ptr<telegram_api::wallPaperSettings> get_input_wallpaper_settings(const BackgroundType &type);

}  // namespace td

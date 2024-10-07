#include "dwm_render.h"
#include <vector>
#include "imgui/imgui.h"
#include "../dwm_comm/dwm_comm.h"
#include "dwm_painter_common.h"
#include "debug.hpp"

dwm_render::dwm_render()
  :commer_(nullptr), render_buffer_(nullptr), render_buffer_size_(0)
{
}

bool dwm_render::init()
{
  commer_ = dwm_comm_create();

  return commer_ != nullptr;
}

void dwm_render::render()
{
  if (dwm_comm_get_data(commer_, &render_buffer_, &render_buffer_size_)) 
  {
    draw_info* draw_data = (draw_info*)(render_buffer_);

    for (; draw_data->type != draw_type::unknown; draw_data++)
    {
      TRACE_V("draw type %d", draw_data->type);
      if (draw_data->type == draw_type::text) {

        if (draw_data->info.text.outline) {
          ImGui::GetBackgroundDrawList()->AddText(
            ImVec2(draw_data->info.text.x + 1.0f, draw_data->info.text.y + 1.0f),
            ImColor(0.0f, 0.0f, 0.0f, 0.7f),
            draw_data->info.text.text
          );

        }
        ImGui::GetBackgroundDrawList()->AddText(
          ImVec2(draw_data->info.text.x, draw_data->info.text.y),
          draw_data->info.text.color,
          draw_data->info.text.text
        );
      }
      else if (draw_data->type == draw_type::line) {
        ImGui::GetBackgroundDrawList()->AddLine(
          ImVec2(draw_data->info.line.x1, draw_data->info.line.y1),
          ImVec2(draw_data->info.line.x2, draw_data->info.line.y2),
          draw_data->info.line.color,
          draw_data->info.line.thickness
        );

      }
      else if (draw_data->type == draw_type::rect) {
        if (draw_data->info.rect.fill) {
          ImGui::GetBackgroundDrawList()->AddRectFilled(
            ImVec2(draw_data->info.rect.x, draw_data->info.rect.y),
            ImVec2(draw_data->info.rect.x + draw_data->info.rect.w, draw_data->info.rect.y + draw_data->info.rect.h),
            draw_data->info.rect.color,
            draw_data->info.rect.rounding
          );

        }
        else {
          ImGui::GetBackgroundDrawList()->AddRect(
            ImVec2(draw_data->info.rect.x, draw_data->info.rect.y),
            ImVec2(draw_data->info.rect.x + draw_data->info.rect.w, draw_data->info.rect.y + draw_data->info.rect.h),
            draw_data->info.rect.color,
            draw_data->info.rect.rounding,
            0,
            draw_data->info.rect.thickness
          );
        }
      }
      else if (draw_data->type == draw_type::circle) {
        if (draw_data->info.circle.fill) {
          ImGui::GetBackgroundDrawList()->AddCircleFilled(
            ImVec2(draw_data->info.circle.x, draw_data->info.circle.y),
            draw_data->info.circle.radius,
            draw_data->info.circle.color,
            64
          );
        }
        else {
          ImGui::GetBackgroundDrawList()->AddCircle(
            ImVec2(draw_data->info.circle.x, draw_data->info.circle.y),
            draw_data->info.circle.radius,
            draw_data->info.circle.color,
            64,
            draw_data->info.circle.thickness
          );
        }
      }
    }
  }
}

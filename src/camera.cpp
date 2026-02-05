//  This file is part of Desktop-D30
//  Copyright (C) 2026 Chris Griffin (bearded-griffin)
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation version 3 of the License.
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

/*!***************************************************
 * @file     src/camera.cpp
 * @brief    Handels all camera functions
 * @details  deals with initalizing the camera and
 * updating the camera.
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/

#include "camera.h"
#include "utils.h"

namespace CAMERA {
/*!***************************************************
 * @brief    Updates camera
 * @details
 * @param    camera Camera2D&
 * @param    project const Project&
 * @return   void
 * @note
 * @date     2026.02.01
 * @author   bearded.griffin
 ****************************************************/
void UpdateCamera(Camera2D &camera, const Project &project) {
  camera.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

  static int lastLabelIndex = project.selectedLabelIndex;
  if (lastLabelIndex != project.selectedLabelIndex) {
    LabelSize sz = LabelSizes[project.selectedLabelIndex];
    camera.target = {sz.width / 2.0f, sz.height / 2.0f};
    lastLabelIndex = project.selectedLabelIndex;
  }
}

/*!***************************************************
 * @brief    Sets the camera position
 * @details  Puts the camera looking at the center of the
 * label instead of the center of the screen.
 * @param    scrnwidth const int*
 * @param    scrnheight const int*
 * @param    proj Project*
 * @return   void
 * @note
 * @date     2026.02.01
 * @author   bearded.griffin
 ****************************************************/
void InitializeCamera(const int *scrnwidth, const int *scrnheight,
                      Camera2D *camera, Project *proj) {

  // We want the camera to look at the center of the label, not (0,0)
  LabelSize initialSize = LabelSizes[proj->selectedLabelIndex];

  *camera = {0};
  camera->zoom = 2.0f;
  camera->offset = {(*scrnwidth) / 2.0f,
                    (*scrnheight) / 2.0f}; // Center of screen
  camera->target = {initialSize.width / 2.0f,
                    initialSize.height / 2.0f}; // Center of label
}
} // namespace CAMERA
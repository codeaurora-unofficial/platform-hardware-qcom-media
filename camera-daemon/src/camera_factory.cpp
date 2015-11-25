/* Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "camera_factory.h"
#include "json/json_parser.h"
#include "camerad.h"
#include "camera_parameters.h"
#include "qcamvid_log.h"

namespace camerad
{

std::map<int, std::weak_ptr<camera::ICameraDevice>> CameraFactory::cameras_;

std::shared_ptr<camera::ICameraDevice> CameraFactory::get(int index)
{
    std::shared_ptr<camera::ICameraDevice> s = nullptr;

    auto i = cameras_.find(index);
    if (i != cameras_.end()) {   /* found */
        s = i->second.lock();
    }
    else {
        camera::ICameraDevice* icd;

        /* create an instance */
        if (0 == camera::ICameraDevice::createInstance(index, &icd)) {
            JSONParser jp;

            /* configure this camera to default configuration */
            if (0 == cfgGetCamera(index, jp)) {
                unsigned int fps;
                JSONID jsid;
                camera::CameraParams param;

                param.init(icd);

                if (JSONPARSER_SUCCESS == JSONParser_Lookup(
                    &jp, 0, "settings.fps", 0, &jsid)
                    && JSONPARSER_SUCCESS == JSONParser_GetUInt(
                        &jp, jsid, &fps)) {
                    /* set the preview fps */
                    camera::Range fps_r(fps * 1000, fps * 1000, 1);
                    param.setPreviewFpsRange(fps_r);
                    QCAM_INFO("settings.fps:%d\n", fps);
                }

                param.commit();
            }

            /* install in the shared ptr. override the default delete */
            s.reset(icd, [](camera::ICameraDevice* p) {
                auto j = cameras_.begin();
                while (j != cameras_.end()) {
                    if (j->second.expired()) {
                        cameras_.erase(j++);
                    }
                    else {
                        j++;
                    }
                }
                camera::ICameraDevice::deleteInstance(&p);
            });
            cameras_[index] = s;
        }
    }
    return s;
}
}

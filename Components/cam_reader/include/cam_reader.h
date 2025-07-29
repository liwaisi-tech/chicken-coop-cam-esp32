#include "esp_err.h"
#ifndef CAM_READER_H
#define CAM_READER_H

esp_err_t init_camera(void);
void task_read_cam_picture();

#endif // CAM_READER_H


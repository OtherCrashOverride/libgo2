#pragma once

typedef enum 
{
    Go2HardwareRevision_Unknown = 0,
    Go2HardwareRevision_V1_0,
    Go2HardwareRevision_V1_1
} go2_hardware_revision_t;


go2_hardware_revision_t go2_hardware_revision_get();

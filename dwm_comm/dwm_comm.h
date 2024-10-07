#pragma once
#include <stdbool.h>
#include <d3d11.h>
#include <stdint.h>
#if defined __cplusplus


extern "C" {
#endif 

  void*  dwm_comm_create();

  void* dwm_comm_open();

  bool dwm_comm_update_data(void* dwm_comm, void* buffer, size_t buffer_size);

  bool dwm_comm_get_data(void* dwm_comm, void** buffer, size_t* buffer_size);

  void dwm_comm_close(void* dwm_comm);

  void dwm_comm_free(void* dwm_comm);

#if defined __cplusplus
}
#endif 
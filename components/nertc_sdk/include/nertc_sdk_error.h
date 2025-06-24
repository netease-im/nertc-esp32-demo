#ifndef __NERTC_SDK_ERROR_H__
#define __NERTC_SDK_ERROR_H__

#ifdef __cplusplus
extern "C" {
#endif

// Error Code define
typedef enum {
  /* No Error */
  NERTC_SDK_ERR_SUCCESS                                   = 0,

  /* General error (no specified reason). */
  NERTC_SDK_ERR_FAILED                                    = -1,

  /* Invalid parameter */
  NERTC_SDK_ERR_INVALID_PARAMETER                         = -2,

  /* Invalid state */
  NERTC_SDK_ERR_INVALID_STATE                             = -3,

  /* The required component is not enabled */
  NERTC_SDK_ERR_COMPONENT_NOT_ENABLED                     = -4,

  /* The feature is disabled */
  NERTC_SDK_ERR_FEATURE_DISABLED                          = -5,

  /* Invalid user id */
  NERTC_SDK_ERR_INVALID_USER_ID                           = -6,

  /* Invalid user id */
  NERTC_SDK_ERR_USER_NOT_FOUND                            = -7,
} nertc_sdk_error_code_e;

#ifdef __cplusplus
}
#endif

#endif // __NERTC_SDK_ERROR_H__
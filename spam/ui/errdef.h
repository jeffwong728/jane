#ifndef SPAM_UI_ERROR_DEFINE_H
#define SPAM_UI_ERROR_DEFINE_H

enum class SpamResult
{
    kSR_SUCCESS = 0,
    kSR_OK = kSR_SUCCESS,
    kSR_SUCCESS_NOOP = 1,
    kSR_IMG_EMPTY = 2,
    kSR_IMG_TYPE_NOT_SUPPORTED = 3,
    kSR_IMG_CORRUPTED = 4,
    kSR_TM_EMPTY_TEMPL_REGION = 5,
    kSR_TM_TEMPL_REGION_TOO_SMALL = 6,
    kSR_TM_TEMPL_REGION_TOO_LARGE = 7,
    kSR_TM_TEMPL_REGION_OUT_OF_RANGE = 8,
    kSR_TM_TEMPL_INSIGNIFICANT = 9,
    kSR_TM_PYRAMID_LEVEL_INVALID = 10,
    kSR_TM_PYRAMID_LEVEL_TOO_LARGE = 11,
    kSR_TM_ANGLE_RANGE_INVALID = 12,
    kSR_TM_CORRUPTED_TEMPL_DATA = 13,
    kSR_TM_INSTANCE_NOT_FOUND = 14,
    kSR_ERROR = -1,
    kSR_FAILURE = kSR_ERROR
};

#endif //SPAM_UI_ERROR_DEFINE_H
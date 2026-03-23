#ifndef COMPONENT_DEF_H
#define COMPONENT_DEF_H

typedef struct
{
    /* Identity. */
    uint8_t who_am_i;

    /* ACTION ----------------------------------------------------------------*/
    /* There should be only a unique identifier for each component, which     */
    /* should be the "who_am_i" parameter, hence this parameter is optional.  */
    /* -----------------------------------------------------------------------*/
    /* Type. */
    uint8_t type;

    /* Configuration. */
    uint8_t address;

    /* Pointer to the Data. */
    void *p_data;

    /* Pointer to the Virtual Table. */
    void *p_vt;

    /* ACTION ----------------------------------------------------------------*/
    /* There should be only a unique virtual table for each component, which  */
    /* should be the "p_vt" parameter, hence this parameter is optional.      */
    /* -----------------------------------------------------------------------*/
    /* Pointer to the Extended Virtual Table. */
    void *p_ext_vt;
} handle_t;

/**
 * @brief  Component's Status enumerator definition.
 */
typedef enum
{
    COMPONENT_OK = 0,
    COMPONENT_ERROR,
    COMPONENT_TIMEOUT,
    COMPONENT_NOT_IMPLEMENTED
} status_t;

#endif

#ifndef COMPONENT_H
#define COMPONENT_H

#include <stdint.h>

#if 0
class Component {
public:

    /**
     * @brief     Initializing the component.
     * @param[in] init pointer to device specific initalization structure.
     * @retval    "0" in case of success, an error code otherwise.
     */
    virtual int init(void *init) = 0;

    /**
     * @brief      Getting the ID of the component.
     * @param[out] id pointer to an allocated variable to store the ID into.
     * @retval     "0" in case of success, an error code otherwise.
     */
    virtual int read_id(uint8_t *id) = 0;

    /**
     * @brief Destructor.
     */
    virtual ~Component() {};
};
#endif

#endif

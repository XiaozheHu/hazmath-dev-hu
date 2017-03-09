/*! \file src/utilities/timing.c
 *
 *  Created by James Adler and Xiaozhe Hu on 10/06/15.
 *  Copyright 2015__HAZMATH__. All rights reserved.
 *
 *  \note: modified by Xiaozhe Hu on 10/27/2016
 *  \note: done cleanup for releasing -- Xiaozhe Hu 10/27/2016
 */

#include "hazmath.h"

/*************************************************************************************/
void get_time (REAL *time)
{
    /*!
     * \fn get_time (REAL *time)
     *
     * \brief Get system time
     *
     * \author Xiaozhe Hu
     * \date   10/06/2015
     *
     */
    
    if ( time != NULL ) {
        *time = (REAL) clock() / CLOCKS_PER_SEC;
    }
}

/******************************* END **************************************************/

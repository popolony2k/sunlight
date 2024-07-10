/*
 * object.h
 *
 *  Created on: May 03, 2022
 *      Author: popolony2k
 */

#include "object.h"


namespace SunLight  {
    namespace Base  {

        /**
         * @brief Constructor. INitialize all class data.
         * 
         */
        Object :: Object( void )  {
            m_pPtrData = nullptr;
        }

        /**
         * @brief Destructor. Finalize all class data.
         */
        Object :: ~Object( void )  {

        }

        /**
         * @brief Set a pointer to object.
         * 
         * @param ptrData The pointer to set;
         */
        void Object :: SetPtrData( VoidPtr ptrData )  {
            
            m_pPtrData = ptrData;
        }

        /**
         * @brief Get the Ptr Data stored on object;
         * 
         * @return VoidPtr The pointer previsously set by @see SetPtrData
         * method;
         */
        Object :: VoidPtr Object :: GetPtrData( void )  {
            
            return m_pPtrData;
        }
    }
}
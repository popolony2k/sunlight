/*
 * object.h
 *
 *  Created on: May 03, 2022
 *      Author: popolony2k
 */

#ifndef __OBJECT_H__
#define __OBJECT_H__

namespace SunLight  {
    namespace Base  {

        /**
         * @brief Base class object implementation.
         * 
         */
        class Object  {

            public:

            /*
             * Internal class definitons.
             */
            typedef void* VoidPtr; 

            Object( void );
            virtual ~Object( void );

            void SetPtrData( VoidPtr ptrData );
            VoidPtr GetPtrData( void );

            protected:

            VoidPtr        m_pPtrData;
        };
    }
}

#endif  /* __OBJECT_H__ */
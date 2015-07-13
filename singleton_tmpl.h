#ifndef _SINGLETON_TMPL_H
#define _SINGLETON_TMPL_H
#include <stdio.h>

namespace richinfo {

/* Singleton Template */
template<typename T>
class Singleton
{
    public:
    static T* GetInstance()
    {
        if (m_Instance == NULL )
        {
            m_Instance = new T;
        }
        return m_Instance;
    }
    static void ReleaseInstance()
    {
        if(m_Instance != NULL)
            delete m_Instance;
        m_Instance = NULL;
    }
    virtual ~Singleton()
    {
        ReleaseInstance();
    }

    protected:
    Singleton();

    private:
    Singleton(const Singleton& );
    Singleton& operator= (const Singleton& );

    private:
    static T* m_Instance;
};

template<typename T>
T* Singleton<T>::m_Instance = NULL;

}  // ns richinfo

#endif // _SINGLETON_TMPL_H

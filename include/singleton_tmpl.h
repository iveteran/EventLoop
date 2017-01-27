#ifndef _SINGLETON_TMPL_H
#define _SINGLETON_TMPL_H
#include <stdio.h>
#if defined(MULTIPLE_THREAD_SUPPORTS)
#include <map>
#include <algorithm>
#endif

namespace evt_loop {

/* Singleton Template */
template<typename T>
class Singleton
{
    public:
#if !defined(MULTIPLE_THREAD_SUPPORTS)
    static T* GetInstance()
    {
        if (m_instance == NULL )
        {
            m_instance = new T;
        }
        return m_instance;
    }
    static void ReleaseInstance()
    {
        delete m_instance;
        m_instance = NULL;
    }
#else
    static T* GetInstance()
    {
        T* instance = NULL;
        uint32_t tid = pthread_self();
        auto iter = m_thread_instance_map.find(tid);
        if (iter != m_thread_instance_map.end()) {
            instance = iter->second;
        } else {
            instance = new T;
            m_thread_instance_map[tid] = instance;
        }
        return instance;
    }
    static void ReleaseInstance()
    {
        for (auto item : m_thread_instance_map ) {
            delete item->second;
        }
    }
#endif
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
#if !defined(MULTIPLE_THREAD_SUPPORTS)
    static T* m_instance;
#else
    static std::map<uint32_t, T*> m_thread_instance_map;
#endif
};

#if !defined(MULTIPLE_THREAD_SUPPORTS)
template<typename T>
T* Singleton<T>::m_instance = NULL;
#else
template<typename T>
std::map<uint32_t, T*> Singleton<T>::m_thread_instance_map;
#endif

}  // ns evt_loop

#endif // _SINGLETON_TMPL_H

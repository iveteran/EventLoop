#ifndef _CALLBACK_H
#define _CALLBACK_H

#include "object.h"

namespace richinfo {

template < typename ClassName >
class ICallback : public Object
{
#define Parent  Object
    public:
    ICallback(ClassName* class_instance = NULL)
        : m_class_instance(class_instance), m_invoke_times(0)
    { }

    virtual string ToString() const
    {
        stringstream ss;
        ss << "{ "
            << Parent::ToString() << ", "
            << "class_instance: " << m_class_instance << ", "
            << "invoke_times: " << m_invoke_times
            << " }";
        return ss.str();
    }

    size_t InvokingTimes() const
    {
        return m_invoke_times;
    }

    protected:
    ClassName*  m_class_instance;
    size_t      m_invoke_times;
#undef Parent
};

template < typename ClassName, typename ParamType >
class Callback : public ICallback<ClassName>
{
#define Parent  ICallback<ClassName>
    typedef void (ClassName::*Method)(ParamType);

    public:
    Callback(ClassName* class_instance = NULL, Method method = NULL)
        : Parent(class_instance), m_method(method)
    { }

    bool Invoke(ParamType parameter)
    {
        this->m_invoke_times++;
        bool success = false;
        if (this->m_class_instance != NULL && m_method != NULL)
        {
            (this->m_class_instance->*m_method)(parameter);
            success = true;
        }
        return success;
    }

    string ToString() const
    {
        stringstream ss;
        ss << "{ "
            << Parent::ToString() << ", "
            << "method: " << m_method << ", "
            << " }";
        return ss.str();
    }

    private:
    Method      m_method;
#undef Parent
};

template < typename ClassName, typename ReturnType, typename ParamType >
class Callback_Rtn : public ICallback<ClassName>
{
#define Parent  ICallback<ClassName>
    typedef ReturnType (ClassName::*Method)(ParamType);

    public:
    Callback_Rtn(ClassName* class_instance = NULL, Method method = NULL) 
        : Parent(class_instance), m_method(method)
    { }

    ReturnType Invoke(ParamType parameter)
    {
        this->m_invoke_times++;
        assert(this->m_class_instance != NULL && m_method != NULL);

        return ((this->m_class_instance)->*m_method)(parameter);
    }

    string ToString() const
    {
        stringstream ss;
        ss << "{ "
            << Parent::ToString() << ", "
            << "method: " << m_method << ", "
            << " }";
        return ss.str();
    }

    private:
    Method      m_method;
#undef Parent
};

template < typename ClassName, typename ParamType, typename ParamType2 >
class Callback2 : public ICallback<ClassName>
{
#define Parent  ICallback<ClassName>
    typedef void (ClassName::*Method)(ParamType, ParamType2);

    public:
    Callback2(ClassName* class_instance = NULL, Method method = NULL) 
        : Parent(class_instance), m_method(method)
    { }

    bool Invoke(ParamType parameter, ParamType2 parameter2)
    {
        this->m_invoke_times++;
        bool success = false;
        if (this->m_class_instance != NULL && m_method != NULL)
        {
            ((this->m_class_instance)->*m_method)(parameter, parameter2);
            success = true;
        }
        printf("[Callback2::Invoke] %s\n", this->ToString().c_str());
        return success;
    }

    string ToString() const
    {
        stringstream ss;
        ss << "{ "
            << Parent::ToString() << ", "
            << "method: " << m_method << ", "
            << " }";
        return ss.str();
    }

    private:
    Method      m_method;
#undef Parent
};

template < typename ClassName, typename ParamType, typename ParamType2, typename ParamType3 >
class Callback3 : public ICallback<ClassName>
{
#define Parent  ICallback<ClassName>
    typedef void (ClassName::*Method)(ParamType, ParamType2, ParamType3);

    public:
    Callback3(ClassName* class_instance = NULL, Method method = NULL) 
        : Parent(class_instance), m_method(method)
    { }

    bool Invoke(ParamType parameter, ParamType2 parameter2, ParamType3 parameter3)
    {
        this->m_invoke_times++;
        bool success = false;
        if (this->m_class_instance != NULL && m_method != NULL)
        {
            ((this->m_class_instance)->*m_method)(parameter, parameter2, parameter3);
            success = true;
        }
        return success;
    }

    private:
    Method      m_method;
#undef Parent
};

}   // namespace richinfo

#endif  // _CALLBACK_H

#ifndef _OBJECT_H
#define _OBJECT_H

#include <stdint.h>
#include <string>
#include <sstream>

using std::string;
using std::stringstream;

class Object
{
    public:
    Object(uint32_t id = 0, string name = "", time_t birthday = time(NULL)) :
        m_id(id),
        m_name(name),
        m_birthday(birthday),
        m_last_errno(0),
        m_last_errtime(0)
    {}
    virtual ~Object() {}

    uint32_t GetID() const
    {
        return m_id;
    }
    const string& GetName() const
    {
        return m_name;
    }
    time_t GetBirthday() const
    {
        return m_birthday;
    }
    virtual string ToString() const
    {
        stringstream ss;
        ss << "{ "
            << "id: " << m_id << ", "
            << "name: " << m_name << ", "
            << "birthday: " << m_birthday << ", "
            << "last errno: " << m_last_errno << ", "
            << "last errstr: " << m_last_errstr
            << " }";
        return ss.str();
    }
    void SetLastError(const string& errstr, int errno_ = -1)
    {
        m_last_errno = errno_;
        m_last_errstr = errstr;
        m_last_errtime = time(NULL);
    }
    int GetLastError(string& errstr, time_t& errtime) const
    {
        errstr = m_last_errstr;
        errtime = m_last_errtime;
        return m_last_errno;
    }

    private:
    uint32_t    m_id;
    string      m_name;
    time_t      m_birthday;

    int         m_last_errno;
    string      m_last_errstr;
    time_t      m_last_errtime;
};

class ObjectSerializable : public Object
{
#define Parent  Object
    public:
    ObjectSerializable() : m_serialized(false) { }

    virtual ObjectSerializable& Serialize()
    {
        m_serialized = true;
        return *this;
    }
    virtual ObjectSerializable& Unserialize(const string& bytes)
    {
        return *this;
    }
    const string& GetSerializedBytes()
    {
        if (!m_serialized)
            Serialize();
        return m_serialized_bytes;
    }
    string ToString() const
    {
        stringstream ss;
        ss << "{ "
            << Parent::ToString() << ", "
            << "serialized: " << m_serialized << ", "
            << "serialized_bytes: " << m_serialized_bytes
            << " }";
        return ss.str();
    }
    
    private:
    bool       m_serialized;
    string     m_serialized_bytes;
#undef Parent
};

#endif // _OBJECT_H

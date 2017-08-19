#ifndef _DOUBLE_MAP_
#define _DOUBLE_MAP_

#include <map>

namespace evt_loop {

template <typename KEY, typename SUB_KEY, typename VALUE>
class DoubleMap
{
    public:
    using ParentMap = std::map<KEY, std::map<SUB_KEY, VALUE>>;
    using SubMap = std::map<SUB_KEY, VALUE>;

    public:
    typename ParentMap::iterator Find(KEY key)
    {
        return double_map_.find(key);
    }
    typename ParentMap::const_iterator Find(KEY key) const
    {
        return double_map_.find(key);
    }

    typename SubMap::iterator Find(KEY key, SUB_KEY sub_key)
    {
        auto iter = double_map_.find(key);
        return iter != double_map_.end() ? iter->second.find(sub_key) : double_map_.end();
    }
    typename SubMap::const_iterator Find(KEY key, SUB_KEY sub_key) const
    {
        auto iter = double_map_.find(key);
        return iter != double_map_.end() ? iter->second.find(sub_key) : double_map_.end();
    }

    typename ParentMap::iterator Begin()
    {
        return double_map_.begin();
    }
    typename ParentMap::iterator End()
    {
        return double_map_.end();
    }

    void Erase(KEY key)
    {
        double_map_.erase(key);
    }
    void Erase(KEY key, SUB_KEY sub_key)
    {
        auto iter = double_map_.find(key);
        if (iter != double_map_.end())
        {
            auto& sub_map = iter->second;
            sub_map.erase(sub_key);
            if (sub_map.empty())
            {
                double_map_.erase(iter);
            }
        }
    }
    void Erase(typename ParentMap::iterator iter)
    {
        double_map_.erase(iter);
    }

    void Insert(KEY key, SUB_KEY sub_key, VALUE value)
    {
        auto iter = double_map_.find(key);
        if (iter == double_map_.end())
        {
            auto result = double_map_.insert(std::make_pair(key, SubMap()));
            iter = result.first;
        }
        auto& sub_map = iter->second;
        sub_map.insert(std::make_pair(sub_key, value));
    }

    size_t Size() const
    {
        return double_map_.size();
    }
    size_t Size(KEY key) const
    {
        auto& iter = double_map_.find(key);
        return iter != double_map_.end() ? iter->second.size() : 0;
    }
    size_t ElementCount() const
    {
        size_t count = 0;
        for (auto& sub_map : double_map_)
        {
            count += sub_map.second.size();
        }
        return count;
    }

    bool Empty()
    {
        return double_map_.empty();
    }
    bool Empty(KEY key)
    {
        auto& iter = double_map_.find(key);
        return iter != double_map_.end() ? iter->second.empty() : true;
    }

    private:
    std::map<KEY, std::map<SUB_KEY, VALUE>> double_map_;
};

}  // namespace evt_loop

#endif   // _DOUBLE_MAP_

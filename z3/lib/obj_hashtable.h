/*++
Copyright (c) 2006 Microsoft Corporation

Module Name:

    obj_hashtable.h

Abstract:

    <abstract>

Author:

    Leonardo de Moura (leonardo) 2008-02-16.

Revision History:

--*/
#ifndef _OBJ_HASHTABLE_H_
#define _OBJ_HASHTABLE_H_

#include"hash.h"
#include"hashtable.h"


/**
   \brief Special entry for a hashtable of obj pointers (i.e.,
   objects that have a hash() method).
   This entry uses 0x0 and 0x1 to represent HT_FREE and HT_DELETED.
*/
template<typename T>
class obj_hash_entry {
    T *             m_ptr;
public:
    typedef T * data;
    obj_hash_entry():m_ptr(0) {}
    unsigned get_hash() const { return m_ptr->hash(); }
    bool is_free() const { return m_ptr == 0; }
    bool is_deleted() const { return m_ptr == reinterpret_cast<T *>(1); }
    bool is_used() const { return m_ptr != reinterpret_cast<T *>(0) && m_ptr != reinterpret_cast<T *>(1); }
    T * get_data() const { return m_ptr; }
    T * & get_data() { return m_ptr; }
    void set_data(T * d) { m_ptr = d; }
    void set_hash(unsigned h) { SASSERT(h == m_ptr->hash()); }
    void mark_as_deleted() { m_ptr = reinterpret_cast<T *>(1); }
    void mark_as_free() { m_ptr = 0; }
};

template<typename T>
class obj_hashtable : public core_hashtable<obj_hash_entry<T>, obj_ptr_hash<T>, ptr_eq<T> > {
public:
    obj_hashtable(unsigned initial_capacity = DEFAULT_HASHTABLE_INITIAL_CAPACITY):
        core_hashtable<obj_hash_entry<T>, obj_ptr_hash<T>, ptr_eq<T> >(initial_capacity) {}
};

template<typename Key, typename Value>
class obj_map {
public:
    struct key_data {
        Key *  m_key;
        Value  m_value;
        key_data():m_key(0) {
        }
        key_data(Key * k):
            m_key(k) {
        }
        key_data(Key * k, Value const & v):
            m_key(k),
            m_value(v) {
        }
        Value const & get_value() const { return m_value; }
        unsigned hash() const { return m_key->hash(); }
        bool operator==(key_data const & other) const { return m_key == other.m_key; }
    };

    class obj_map_entry {
        key_data m_data;
    public:
        typedef key_data data;
        obj_map_entry() {}
        unsigned get_hash() const { return m_data.hash(); }
        bool is_free() const { return m_data.m_key == 0; }
        bool is_deleted() const { return m_data.m_key == reinterpret_cast<Key *>(1); }
        bool is_used() const { return m_data.m_key != reinterpret_cast<Key *>(0) && m_data.m_key != reinterpret_cast<Key *>(1); }
        key_data const & get_data() const { return m_data; }
        key_data & get_data() { return m_data; }
        void set_data(key_data const & d) { m_data = d; }
        void set_hash(unsigned h) { SASSERT(h == m_data.hash()); }
        void mark_as_deleted() { m_data.m_key = reinterpret_cast<Key *>(1); }
        void mark_as_free() { m_data.m_key = 0; }
    };

    typedef core_hashtable<obj_map_entry, obj_hash<key_data>, default_eq<key_data> > table;

    table m_table;
  
public:
    obj_map():
        m_table(DEFAULT_HASHTABLE_INITIAL_CAPACITY) {}
    
    typedef typename table::iterator iterator;
    typedef Key    key;
    typedef Value  value;

    void reset() {
        m_table.reset();
    }
            
    void finalize() {
        m_table.finalize();
    }
    
    bool empty() const { 
        return m_table.empty();
    }
    
    unsigned size() const { 
        return m_table.size(); 
    }
    
    unsigned capacity() const { 
        return m_table.capacity();
    }
    
    iterator begin() const { 
        return m_table.begin();
    }
    
    iterator end() const { 
        return m_table.end();
    }
    
    void insert(Key * k, Value const & v) {
        m_table.insert(key_data(k, v));
    }
    
    key_data const & insert_if_not_there(Key * k, Value const & v) {
        return m_table.insert_if_not_there(key_data(k, v));
    }

    obj_map_entry * insert_if_not_there2(Key * k, Value const & v) {
        return m_table.insert_if_not_there2(key_data(k, v));
    }
    
    obj_map_entry * find_core(Key * k) const {
        return m_table.find_core(key_data(k));
    }

    bool find(Key * k, Value & v) const {
        obj_map_entry * e = find_core(k);
        if (e) {
            v = e->get_data().m_value;
        }
        return (0 != e);
    }

    value const & find(key * k) const {
        obj_map_entry * e = find_core(k);
        SASSERT(e);
        return e->get_data().m_value;
    }

    value & find(key * k)  {
        obj_map_entry * e = find_core(k);
        SASSERT(e);
        return e->get_data().m_value;
    }
    
    iterator find_iterator(Key * k) const { 
        return m_table.find(key_data(k));
    }

    bool contains(Key * k) const { 
        return find_core(k) != 0; 
    }

    void remove(Key * k) {
        m_table.remove(key_data(k));
    }
    
    void erase(Key * k) {
        remove(k);
    }

    unsigned long long get_num_collision() const { return m_table.get_num_collision(); }

    void swap(obj_map & other) {
        m_table.swap(other.m_table);
    }
};

/**
   \brief Reset and deallocate the values stored in a mapping of the form obj_map<Key, Value*>
*/
template<typename Key, typename Value>
void reset_dealloc_values(obj_map<Key, Value*> & m) {
    typename obj_map<Key, Value*>::iterator it  = m.begin();
    typename obj_map<Key, Value*>::iterator end = m.end();
    for (; it != end; ++it) {
        dealloc(it->m_value);
    }
    m.reset();
}

/**
   \brief Remove the key k from the mapping m, and delete the value associated with k.
*/
template<typename Key, typename Value>
void erase_dealloc_value(obj_map<Key, Value*> & m, Key * k) {
    Value * v = 0;
    bool contains = m.find(k, v);
    m.erase(k);
    if (contains) {
        dealloc(v);
    }
}

#endif /* _OBJ_HASHTABLE_H_ */


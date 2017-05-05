#ifndef VECTOR_H
#define VECTOR_H

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

template <typename TT>
void destroy_all(TT* data, size_t size,
    typename std::enable_if<!std::is_trivially_destructible<TT>::value>::type* = nullptr)
{
    for (size_t i = 0; i != size; i++)
        data[i].~TT();
}

template <typename TT>
void destroy_all(TT*, size_t,
    typename std::enable_if<std::is_trivially_destructible<TT>::value>::type* = nullptr)
{}

template <typename TT>
void copy_construct_all(TT* dst, TT const* src, size_t size,
    typename std::enable_if<!std::is_trivially_copyable<TT>::value>::type* = nullptr)
{
    size_t i = 0;
    
    try
    {
        for (; i != size; ++i)
            new (dst + i) TT(src[i]);
    }
    catch (...)
    {
        while (i != 0)
        {
            dst[i - 1].~TT();
            --i;
        }
        throw;
    }
}

template <typename TT>
void copy_construct_all(TT* dst, TT const* src, size_t size,
    typename std::enable_if<std::is_trivially_copyable<TT>::value>::type* = nullptr)
{
    // memcpy ожидает, что dst и src валидные указатели даже если
    // (size == 0). vector может вызывать copy_construct_all с
    // (src == nullptr) и (size == 0). Поэтому в copy_construct_all
    // необходимо иметь проверку на (size != 0), чтобы не вызвать
    // memcpy c (src == nullptr) и не вызвать undefined behavior.
    if (size != 0)
        memcpy(dst, src, size * sizeof(TT));
}

template <typename T>
struct vector
{
    typedef T* iterator;
    typedef T const* const_iterator;

    vector();
    vector(vector const&);
    vector& operator=(vector const& other);

    ~vector();

    T& operator[](size_t i);
    T const& operator[](size_t i) const;

    T* data();
    T const* data() const;

    size_t size() const;

    T& front();
    T const& front() const;

    T& back();
    T const& back() const;

    bool empty() const;

    size_t capacity() const;
    void reserve(size_t);
    void shrink_to_fit();
    
    void clear();
    
    void push_back(T const&);
    void pop_back();
    
    void swap(vector&);

    iterator insert(iterator pos, T const&);
    iterator insert(const_iterator pos, T const&);

    iterator erase(iterator pos);
    iterator erase(const_iterator pos);

    iterator erase(iterator first, iterator last);
    iterator erase(const_iterator first, const_iterator last);

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

private:
    size_t increase_capacity() const;
    void push_back_realloc(T const&);
    void new_buffer(size_t new_capacity);
    
private:
    T* data_;
    size_t size_;
    size_t capacity_;
};

template <typename T>
vector<T>::vector()
    : data_(nullptr)
    , size_(0)
    , capacity_(0)
{}

template <typename T>
vector<T>::vector(vector const& other)
    : vector()
{
    new_buffer(other.size());
    copy_construct_all(data_, other.data_, other.size_);
    size_ = other.size_;
}

template <typename T>
vector<T>& vector<T>::operator=(vector const& other)
{
    vector copy(other);
    swap(copy);
    return *this;
}

template <typename T>
vector<T>::~vector()
{
    destroy_all(data_, size_);
    operator delete(data_);
}

template <typename T>
T& vector<T>::operator[](size_t i)
{
    return data_[i];
}

template <typename T>
T const& vector<T>::operator[](size_t i) const
{
    return data_[i];
}

template <typename T>
T* vector<T>::data()
{
    return data_;
}

template <typename T>
T const* vector<T>::data() const
{
    return data_;
}

template <typename T>
size_t vector<T>::size() const
{
    return size_;    
}

template <typename T>
T& vector<T>::front()
{
    return *data_;
}

template <typename T>
T const& vector<T>::front() const
{
    return *data_;
}


template <typename T>
T& vector<T>::back()
{
    return data_[size_ - 1];
}

template <typename T>
T const& vector<T>::back() const
{
    return data_[size_ - 1];
}

template <typename T>
bool vector<T>::empty() const
{
    return size_ == 0;
}

template <typename T>
size_t vector<T>::capacity() const
{
    return capacity_;
}

template <typename T>
void vector<T>::reserve(size_t desired_capacity)
{
    if (desired_capacity < capacity_)
        return;

    new_buffer(desired_capacity);
}

template <typename T>
void vector<T>::shrink_to_fit()
{
    if (capacity_ == size_)
        return;

    new_buffer(size_);
}

template <typename T>
void vector<T>::clear()
{
    destroy_all(data_, size_);
    size_ = 0;
}

template <typename T>
void vector<T>::push_back(T const& val)
{
    if (size_ != capacity_)
    {
        new (data_ + size_) T(val);
        ++size_;
    }
    else
    {
        push_back_realloc(val);
    }
}

template <typename T>
void vector<T>::pop_back()
{
    assert(size_ != 0);

    data_[size_ - 1].~T();
    --size_;
}

template <typename T>
void vector<T>::swap(vector& other)
{
    using std::swap;

    swap(data_,     other.data_);
    swap(size_,     other.size_);
    swap(capacity_, other.capacity_);
}

template <typename T>
typename vector<T>::iterator vector<T>::insert(iterator pos, T const& val)
{
    if (size_ == capacity_)
    {
        vector tmp;
        tmp.new_buffer(increase_capacity());
        copy_construct_all(tmp.data_, data_, pos - begin());
        tmp.size_ = pos - begin();
        
        auto result = tmp.end();

        tmp.push_back(val);
        
        copy_construct_all(tmp.end(), pos, end() - pos);
        tmp.size_ += end() - pos;
        
        swap(tmp);
        return result;
    }
    
    push_back(back());

    for (auto i = (end() - 1); i != pos; --i)
    {
        using std::swap;
        swap(*i, *(i - 1));
    }
    
    *pos = val;
    return pos;
}

template <typename T>
typename vector<T>::iterator vector<T>::insert(const_iterator pos, T const& val)
{
    return insert(data_ + (pos - data_), val);
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(iterator pos)
{
    return erase(pos, pos + 1);
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(const_iterator pos)
{
    return erase(data_ + (pos - data_));
    
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(iterator first, iterator last)
{
    iterator result = first;

    while (last != end())
    {
        using std::swap;
        swap(*first, *last);
        ++first;
        ++last;
    }

    size_ = first - data_;
    while (first != last)
    {
        first->~T();
        ++first;
    }

    return result;
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(const_iterator first, const_iterator last)
{
    return erase(data_ + (first - data_),
                 data_ + (last  - data_));
}

template <typename T>
typename vector<T>::iterator vector<T>::begin()
{
    return data_;
}

template <typename T>
typename vector<T>::iterator vector<T>::end()
{
    return data_ + size_;
}


template <typename T>
typename vector<T>::const_iterator vector<T>::begin() const
{
    return data_;
}


template <typename T>
typename vector<T>::const_iterator vector<T>::end() const
{
    return data_ + size_;
}


template <typename T>
size_t vector<T>::increase_capacity() const
{
    if (capacity_ == 0)
        return 4;
    else
        return capacity_ * 3 / 2;
}

template <typename T>
void vector<T>::push_back_realloc(T const& val)
{
    vector<T> tmp;
    tmp.new_buffer(increase_capacity());
    copy_construct_all(tmp.data_, data_, size_);
    tmp.size_ = size_;

    tmp.push_back(val);
    swap(tmp);
}

template <typename T>
void vector<T>::new_buffer(size_t new_capacity)
{
    assert(new_capacity >= size_);

    vector<T> tmp;
    tmp.data_ = static_cast<T*>(operator new(new_capacity * sizeof(T)));
    tmp.capacity_ = new_capacity;
    copy_construct_all(tmp.data_, data_, size_);
    tmp.size_ = size_;

    swap(tmp);
}

#endif // VECTOR_H

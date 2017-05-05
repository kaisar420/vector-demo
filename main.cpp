#include "vector.h"
#include "gtest/gtest.h"

template struct vector<int>;

template <typename T>
T const& as_const(T& obj)
{
    return obj;
}

template <typename T>
struct counted
{
    counted()
    {
        ++instances();
    }

    counted(T const& val)
        : val(val)
    {
        ++instances();
    }

    counted(counted const& rhs)
        : val(rhs.val)
    {
        copy();
        ++instances();
    }

    counted& operator=(counted const& rhs)
    {
        copy();
        val = rhs.val;
        return *this;
    }

    ~counted()
    {
        --instances();
    }

    static size_t& instances()
    {
        static size_t value = 0;
        return value;
    }

    static void expect_no_instances()
    {
        EXPECT_EQ(0, instances());
    }

    static void set_throw_countdown(size_t val)
    {
        throw_countdown = val;
    }

    friend bool operator==(counted const& a, counted const& b)
    {
        return a.val == b.val;
    }

    friend bool operator!=(counted const& a, counted const& b)
    {
        return a.val != b.val;
    }

private:
    void copy()
    {
        if (throw_countdown != 0)
        {
            --throw_countdown;
            if (throw_countdown == 0)
                throw std::runtime_error("copy failed");
        }
    }

private:
    T val;
    static size_t throw_countdown;
};

template <typename T>
size_t counted<T>::throw_countdown = 0;

TEST(correctness, default_ctor)
{
    vector<counted<int> > a;
    counted<int>::expect_no_instances();
    EXPECT_TRUE(a.empty());
    EXPECT_EQ(0, a.size());
}

TEST(correctness, push_back)
{
    {
        vector<counted<size_t> > a;
        for (size_t i = 0; i != 200; ++i)
            a.push_back(i);

        for (size_t i = 0; i != 200; ++i)
            EXPECT_EQ(i, a[i]);
    }

    counted<size_t>::expect_no_instances();
}

TEST(correctness, push_back_from_self)
{
    {
        vector<counted<size_t> > a;
        a.push_back(42);
        for (size_t i = 0; i != 100; ++i)
            a.push_back(a[0]);

        for (size_t i = 0; i != a.size(); ++i)
            EXPECT_EQ(42, a[i]);
    }

    counted<size_t>::expect_no_instances();
}

TEST(correctness, subscription)
{
    vector<int> a;
    a.push_back(4);
    a.push_back(8);
    a.push_back(15);
    a.push_back(16);
    a.push_back(23);
    a.push_back(42);

    EXPECT_EQ(4, a[0]);
    EXPECT_EQ(8, a[1]);
    EXPECT_EQ(15, a[2]);
    EXPECT_EQ(16, a[3]);
    EXPECT_EQ(23, a[4]);
    EXPECT_EQ(42, a[5]);

    vector<int> const& ca = a;
    EXPECT_EQ(4, ca[0]);
    EXPECT_EQ(8, ca[1]);
    EXPECT_EQ(15, ca[2]);
    EXPECT_EQ(16, ca[3]);
    EXPECT_EQ(23, ca[4]);
    EXPECT_EQ(42, ca[5]);
}

TEST(correctness, data)
{
    vector<counted<size_t> > a;
    a.push_back(5);
    a.push_back(6);
    a.push_back(7);
    
    {
        counted<size_t>* ptr = a.data();
        EXPECT_EQ(5, ptr[0]);
        EXPECT_EQ(6, ptr[1]);
        EXPECT_EQ(7, ptr[2]);
    }

    {
        counted<size_t> const* cptr = as_const(a).data();
        EXPECT_EQ(5, cptr[0]);
        EXPECT_EQ(6, cptr[1]);
        EXPECT_EQ(7, cptr[2]);
    }
}

TEST(correctness, front_back)
{
    vector<counted<size_t> > a;
    a.push_back(5);
    a.push_back(6);
    a.push_back(7);

    EXPECT_EQ(5, a.front());
    EXPECT_EQ(5, as_const(a).front());

    EXPECT_EQ(7, a.back());
    EXPECT_EQ(7, as_const(a).back());
}

TEST(correctness, capacity)
{
    {
        vector<counted<size_t> > a;
        a.reserve(10);
        EXPECT_GE(a.capacity(), 10);
        a.push_back(5);
        a.push_back(6);
        a.push_back(7);
        EXPECT_GE(a.capacity(), 10);
        a.shrink_to_fit();
        EXPECT_EQ(3, a.capacity());
    }
    counted<size_t>::expect_no_instances();
}

TEST(correctness, superfluous_reserve)
{
    {
        vector<counted<size_t> > a;
        a.reserve(10);
        size_t c = a.capacity(); 
        EXPECT_GE(c, 10);
        a.reserve(5);
        EXPECT_GE(a.capacity(), 10);
    }
    counted<size_t>::expect_no_instances();
}

TEST(correctness, clear)
{
    {
        vector<counted<size_t> > a;
        a.push_back(5);
        a.push_back(6);
        a.push_back(7);
        size_t c = a.capacity(); 
        a.clear();
        EXPECT_EQ(c, a.capacity());
    }
    counted<size_t>::expect_no_instances();
}

TEST(correctness, superfluous_shrink_to_fit)
{
    {
        vector<counted<size_t> > a;
        a.reserve(10);
        size_t n = a.capacity(); 
        for (size_t i = 0; i != n; ++i)
            a.push_back(i);

        counted<size_t>* old_data = a.data();        
        a.shrink_to_fit();
        
        EXPECT_EQ(old_data, a.data());
    }
    counted<size_t>::expect_no_instances();
}

TEST(correctness, copy_ctor)
{
    {
        size_t const N = 5;
        vector<counted<size_t> > a;
        for (size_t i = 0; i != N; ++i)
            a.push_back(i);

        vector<counted<size_t> > b = a;
        for (size_t i = 0; i != N; ++i)
            EXPECT_EQ(i, b[i]);
    }
    counted<size_t>::expect_no_instances();
}

TEST(correctness, assignment_operator)
{
    {
        size_t const N = 5;
        vector<counted<size_t> > a;
        for (size_t i = 0; i != N; ++i)
            a.push_back(i);

        vector<counted<size_t> > b;
        b.push_back(42);

        b = a;
        EXPECT_EQ(N, b.size());
        for (size_t i = 0; i != N; ++i)
            EXPECT_EQ(i, b[i]);

        b.push_back(5);
        EXPECT_EQ(5, b[5]);
    }
    counted<size_t>::expect_no_instances();
}

TEST(correctness, self_assignment)
{
    {
        vector<counted<size_t> > a;
        a.push_back(5);
        a.push_back(6);
        a.push_back(7);

        a = a;

        EXPECT_EQ(5, a[0]);
        EXPECT_EQ(6, a[1]);
        EXPECT_EQ(7, a[2]);
    }
    counted<size_t>::expect_no_instances();
}

TEST(correctness, pop_back)
{
    vector<counted<size_t> > a;
    a.push_back(5);
    a.push_back(6);
    a.push_back(7);

    EXPECT_EQ(7, a.back());
    a.pop_back();
    EXPECT_EQ(2, a.size());

    EXPECT_EQ(6, a.back());
    a.pop_back();
    EXPECT_EQ(1, a.size());

    EXPECT_EQ(5, a.back());
    a.pop_back();
    EXPECT_EQ(0, a.size());

    counted<size_t>::expect_no_instances();
}

TEST(correctness, empty)
{
    vector<counted<size_t> > a;

    EXPECT_TRUE(a.empty());
    a.push_back(5);
    EXPECT_FALSE(a.empty());
    a.pop_back();
    EXPECT_TRUE(a.empty());
}

TEST(correctness, insert_begin)
{
    size_t const N = 100;
    vector<counted<size_t> > a;

    for (size_t i = 0; i != N; ++i)
        a.insert(a.begin(), i);

    for (size_t i = 0; i != N; ++i)
    {
        EXPECT_EQ(i, a.back());
        a.pop_back();
    }
}

TEST(correctness, insert_end)
{
    {
        vector<counted<size_t> > a;
        
        a.push_back(4);
        a.push_back(5);
        a.push_back(6);
        a.push_back(7);
    
        EXPECT_EQ(4, a.size());
    
        a.insert(a.end(), 8);
        EXPECT_EQ(5, a.size());
        EXPECT_EQ(8, a.back());
    
        a.insert(a.end(), 9);
        EXPECT_EQ(6, a.size());
        EXPECT_EQ(9, a.back());
    }
    
    counted<size_t>::expect_no_instances();
}

TEST(correctness, erase)
{
    {
        vector<counted<size_t> > a;
        
        a.push_back(4);
        a.push_back(5);
        a.push_back(6);
        a.push_back(7);

        a.erase(a.begin() + 2);

        EXPECT_EQ(3, a.size());
        EXPECT_EQ(4, a[0]);
        EXPECT_EQ(5, a[1]);
        EXPECT_EQ(7, a[2]);
    }
    
    counted<size_t>::expect_no_instances();
}

TEST(correctness, reallocation_throw)
{
    {
        vector<counted<size_t> > a;
        a.reserve(10);
        size_t n = a.capacity();
        for (size_t i = 0; i != n; ++i)
            a.push_back(i);
        
        counted<size_t>::set_throw_countdown(7);
        EXPECT_THROW(a.push_back(42), std::runtime_error);
    }
    counted<size_t>::expect_no_instances();
}

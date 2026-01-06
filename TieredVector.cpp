#include <bits/stdc++.h>
using namespace std;

template <typename T>
class tiered_vector{
    public:
        template <bool is_const>
        class TieredVectorIterator{
            public:
                using iterator_category      = std::random_access_iterator_tag;
                using difference_type        = std::ptrdiff_t;
                using value_type             = T;
                using pointer                = std::conditional_t<is_const, const T*, T*>;
                using reference              = std::conditional_t<is_const, const T&, T&>;
                using parent_type            = std::conditional_t<is_const, const tiered_vector*, tiered_vector*>;
                using reverse_iterator       = std::reverse_iterator<TieredVectorIterator>;
                using const_reverse_iterator = std::reverse_iterator<const TieredVectorIterator>;

            private:
                parent_type parent;
                size_t idx;

            public:
                TieredVectorIterator(parent_type v, size_t i) : parent(v), idx(i) {}

                reference operator*() const {return (*parent)[idx];}
                pointer operator->() const {return &(*parent)[idx];}

                TieredVectorIterator& operator++(){++idx; return *this;}
                TieredVectorIterator operator++(int){TieredVectorIterator tmp = *this; ++(*this); return tmp;}
                TieredVectorIterator& operator--(){--idx; return *this;}
                TieredVectorIterator operator--(int){TieredVectorIterator tmp = *this; --(*this); return tmp;}

                TieredVectorIterator& operator+=(difference_type incr){idx += incr; return *this;}
                TieredVectorIterator& operator-=(difference_type incr){idx -= incr; return *this;}

                friend TieredVectorIterator operator+(TieredVectorIterator it, difference_type incr){return TieredVectorIterator(it.parent, it.idx + incr);}
                friend TieredVectorIterator operator+(difference_type incr, TieredVectorIterator it){return TieredVectorIterator(it.parent, it.idx + incr);}
                friend TieredVectorIterator operator-(TieredVectorIterator it, difference_type incr){return TieredVectorIterator(it.parent, it.idx - incr);}
                friend TieredVectorIterator operator-(difference_type incr, TieredVectorIterator it){return TieredVectorIterator(it.parent, it.idx - incr);}

                friend difference_type operator-(const TieredVectorIterator& a, const TieredVectorIterator& b){return a.idx - b.idx;}
                friend difference_type operator+(const TieredVectorIterator& a, const TieredVectorIterator& b){return a.idx + b.idx;}
                
                bool operator==(const TieredVectorIterator& other){return idx == other.idx;}
                bool operator!=(const TieredVectorIterator& other){return idx != other.idx;}
                bool operator<(const TieredVectorIterator& other){return idx < other.idx;}
                bool operator<=(const TieredVectorIterator& other){return idx <= other.idx;}
                bool operator>(const TieredVectorIterator& other){return idx > other.idx;}
                bool operator>=(const TieredVectorIterator& other){return idx >= other.idx;}

                reference operator[](difference_type incr) const {return *(*this + incr);};

                
        };
    private:
        T** pdata;
        T* internal_pdata[8];
        size_t block_sz;
        size_t block_cap;
        size_t sz;

        void reallocate(size_t new_cap){
            T** new_data = new T*[new_cap]();

            for(size_t i = 0; i<block_sz; ++i){
                new_data[i] = move(pdata[i]);
            }
            if(pdata != internal_pdata && pdata != nullptr)
                delete[] pdata;
            pdata = new_data;
            block_cap = new_cap;
        }

        void allocate(){

        }

        void insertBlock(){
            reallocate(block_cap << 1);
        }

        void initNextSubArray(){
            if(block_cap == 0){
                pdata = internal_pdata;
                block_cap = 8;
            }
            else if(block_sz == block_cap){
                reallocate(block_cap<<1);
            }

            pdata[block_sz++] = new T[1024]();
        }

    public:

        using iterator = TieredVectorIterator<false>;
        using const_iterator = TieredVectorIterator<true>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        tiered_vector() : pdata(nullptr), block_sz(0), block_cap(0), sz(0) {}

        ~tiered_vector(){
            for(size_t i = 0; i < block_sz; ++i){
                delete[] pdata[i];
            }
            if(pdata != internal_pdata && pdata != nullptr)
                delete [] pdata;
        }

        tiered_vector(initializer_list<T> value) : tiered_vector(){
            reserve(value.size());
            for(auto & item : value){
                push_back(item);
            }
        }

        tiered_vector(const tiered_vector& value){
            sz = value.sz;
            block_sz = value.block_sz;
            block_cap = value.block_cap;

            if(block_cap == 0){
                pdata = nullptr;
                return;
            }

            if(value.pdata == value.internal_pdata){
                pdata = internal_pdata;
            }
            else{
                pdata = new T*[block_cap]();
            }

            for(size_t i = 0; i<block_sz; ++i){
                pdata[i] = new T[1024];
                copy(value.pdata[i], value.pdata[i] + 1024, pdata[i]);
            }
        }

        void swap(tiered_vector& other){
            if(pdata != internal_pdata && other.pdata != other.internal_pdata){
                std::swap(pdata, other.pdata);
            }
            else if(pdata == internal_pdata && other.pdata == other.internal_pdata){
                for (size_t i = 0; i < 8; ++i) {
                    std::swap(internal_pdata[i], other.internal_pdata[i]);
                }
            }
            else if(pdata != internal_pdata && other.pdata == other.internal_pdata){
                other.pdata = pdata; 
                
                for(size_t i = 0; i<8; ++i){
                    internal_pdata[i] = other.internal_pdata[i];
                }
                pdata = internal_pdata;
            }
            else{
                pdata = other.pdata;
                
                for(size_t i = 0; i<8; ++i){
                    other.internal_pdata[i] = internal_pdata[i];
                }
                other.pdata = other.internal_pdata;
            }

            std::swap(sz, other.sz);
            std::swap(block_sz, other.block_sz);
            std::swap(block_cap, other.block_cap);
        }

        tiered_vector& operator= (tiered_vector value){
            this-> swap(value);
            return *this;
        }

        tiered_vector(tiered_vector && value) noexcept :
            pdata(value.pdata),
            block_sz(value.block_sz),
            block_cap(value.block_cap),
            sz(value.sz)
        {
            if(value.pdata == value.internal_pdata){
                pdata = internal_pdata;
                for(size_t i=0; i<block_sz; ++i) internal_pdata[i] = value.internal_pdata[i];
            }
            value.pdata = nullptr;
            value.block_sz = 0;
            value.block_cap = 0;
            value.sz = 0;
        }

        void push_back(const T& value){
            if((sz&1023) == 0){
                initNextSubArray();
            }
            pdata[sz>>10][sz&1023] = value;
            sz++;
        }

        void push_back(T&& value){
            if((sz&1023) == 0){
                initNextSubArray();
            }
            pdata[sz>>10][sz&1023] = move(value);
            sz++;
        }

        void pop_back(){
            if(sz == 0) return;

            sz--;
            pdata[sz>>10][sz&1023].~T();

            size_t needed_blocks = (sz == 0) ? 0 : (sz>>10)+1;

            if(block_sz > needed_blocks+1){
                delete[] pdata[--block_sz];
                pdata[block_sz] = nullptr;
            }
        }

        void reserve(size_t n){
            if(n <= capacity()) return;

            size_t needed_blocks = (n + 1023) >> 10;

            size_t new_cap = block_cap == 0 ? 8 : block_cap;
            while(new_cap < needed_blocks) new_cap <<= 1;

            reallocate(new_cap);
        }

        void resize(size_t new_size){
            if(new_size == sz) return;

            if(new_size < sz){
                for(size_t i = new_size; i<sz; ++i){
                    pdata[i>>10][i&1023].~T();
                }
                sz = new_size;
                return;
            }

            size_t needed = (new_size+1023) >> 10;
            if(needed > block_cap){
                size_t new_cap = block_cap == 0 ? 8 : block_cap;

                while(new_cap < needed) new_cap <<= 1;

                reallocate(new_cap);
            }

            for(size_t i = block_sz; i<needed; ++i){
                pdata[i] = new T[1024]();
            }

            block_sz = needed;
            sz = new_size;
        }

        T& operator[](size_t idx){
            return pdata[idx>>10][idx&1023];
        }

        const T& operator[](size_t idx) const {
            return pdata[idx>>10][idx&1023];
        }

        iterator begin() {return iterator(this, 0);}
        iterator end() {return iterator(this, sz);}
        reverse_iterator rbegin() {return reverse_iterator(end());}
        reverse_iterator rend() {return reverse_iterator(begin());}

        const_iterator begin() const {return const_iterator(this, 0);}
        const_iterator end() const {return const_iterator(this, sz);}
        const_reverse_iterator rbegin() const {return const_reverse_iterator(end());}
        const_reverse_iterator rend() const {return const_reverse_iterator(begin());}

        size_t size() const {return this->sz;}
        size_t capacity() const {return this->block_cap<<10;}
        bool empty() const {return ((this->sz) == 0);}
};
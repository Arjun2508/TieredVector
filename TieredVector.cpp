#include <bits/stdc++.h>
using namespace std;

template <typename T>
class tiered_vector{
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
        tiered_vector() : pdata(nullptr), block_sz(0), block_cap(0), sz(0) {}

        ~tiered_vector(){
            for(size_t i = 0; i < block_sz; ++i){
                delete[] pdata[i];
            }
            if(pdata != internal_pdata && pdata != nullptr)
                delete [] pdata;
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

        size_t size() const {return this->sz;}
        size_t capacity() const {return this->block_cap<<10;}
        bool empty() const {return ((this->sz) == 0);}
};
#include <iostream>

using namespace std;

class A{
public:
    void get(){
    cout<<"the val of this class is:"<<val<<endl;
    }
    virtual ~A()=default;
private:
    int val = 1;
};

class B:public A{
public:
    void getb() {
        cout << "the val of this class is:" << bval << endl;
    }
private:
    int bval = 2;
};
template <typename T>
class my_shared_ptr{
public:
    /*
     * 构造函数
     */
    explicit my_shared_ptr(T* ptr = nullptr): _ptr(ptr){
        cout<<"alloc ptr."<<endl;
    }
    /*
     * 拷贝构造函数
     */
    my_shared_ptr(my_shared_ptr&& other){
        _ptr = other.relese();
    }
    template<typename U>
    explicit my_shared_ptr(my_shared_ptr<U>&& other){
        _ptr = other.relese();
    }
    my_shared_ptr& operator=(my_shared_ptr rhs){
        rhs.swap(*this);
        return *this;
    }
    ~my_shared_ptr(){
        delete _ptr;
        cout<<"delete ptr."<<endl;
    }
    T* relese(){
        T* ptr = _ptr;
        _ptr = nullptr;
        return ptr;
    }
    void swap(my_shared_ptr& rhs){
        using std::swap;
        swap(_ptr, rhs._ptr);
    }
    /*
     * 重载运算符
     */
    T& operator*() const{ return *_ptr;}
    T* operator->() const{ return _ptr;}
    T* get(){
        return _ptr;
    }
private:
    T* _ptr;
};
class shared_count{
public:
    shared_count() : _count(1){}
    void add_account(){
        ++_count;
    }
    long reduce_count(){
        return --_count;
    }
    long get_count() const{
        return _count;
    }

private:
    long _count;
};


template <typename T>
class smart_ptr{
public:
    template<typename U>
    friend class smart_ptr;
    //构造函数
    explicit smart_ptr(T* ptr = nullptr): _ptr(ptr) {
     if(ptr){
         _shared_count = new shared_count();
     }
    }
    //智能指针类型转换
    template<typename U>
    smart_ptr(const smart_ptr<U>& other, T* ptr){
        _ptr = ptr;
        if(_ptr){
            other._shared_count->add_account();
            _shared_count = other._shared_count;
        }
    }
    //赋值函数
    smart_ptr& operator=(smart_ptr rhs){
        rhs.swap(*this);
        return *this;
    }
    //拷贝构造函数
    smart_ptr(const smart_ptr& other){
        _ptr = other._ptr;
        if(_ptr){
            other._shared_count->add_account();
            _shared_count = other._shared_count;
        }
    }
    template<typename U>
    smart_ptr(const smart_ptr<U>& other){
        _ptr = other._ptr;
        if(_ptr){
            other._shared_count->add_account();
            _shared_count = other._shared_count;
        }
    }
    //移动构造函数
    template<typename U>
    smart_ptr(smart_ptr<U>&& other){
        _ptr = other._ptr;
        if(_ptr){
            _shared_count = other._shared_count;
            other._ptr = nullptr;
        }
    }
    ~smart_ptr(){
        if(_ptr && !_shared_count->reduce_count()){
            delete _ptr;
            delete _shared_count;
        }
    }
    void swap(smart_ptr& rhs){
        using std::swap;
        swap(_ptr, rhs._ptr);
        swap(_shared_count, rhs._shared_count);
    }
    long use_count() const{
        if(_ptr){
            return _shared_count->get_count();
        }else{
            return 0;
        }
    }
    /*
     * 重载运算符
     */
    T& operator*() const{ return *_ptr;}
    T* operator->() const{ return _ptr;}

    T* get() const {
        return _ptr;
    }
private:
    T* _ptr;
    shared_count* _shared_count;
};

template<typename T, typename U>
smart_ptr<T> dynamic_pointer_cast(const smart_ptr<U>& other){
    T* ptr = dynamic_cast<T*>(other.get());
    return smart_ptr<T>(other, ptr);
}

int main(){
//    my_shared_ptr<B> a{new B};
//    my_shared_ptr<A> b;
//    b = a;
    smart_ptr<B> ptr1{new B};
    cout<<"use count of ptr1 is:"<<ptr1.use_count()<<endl;
    smart_ptr<A> ptr2;
    cout<<"use count of ptr2 is:"<<ptr2.use_count()<<endl;
    ptr2 = ptr1;
    cout<<"use count of ptr2 is:"<<ptr2.use_count()<<endl;
    cout<<"use count of ptr1 is:"<<ptr1.use_count()<<endl;
    smart_ptr<B> ptr3 = dynamic_pointer_cast<B>(ptr2);
    cout<<"use count of ptr3 is:"<<ptr3.use_count()<<endl;
    //ptr2->getb();
    return 0;
}

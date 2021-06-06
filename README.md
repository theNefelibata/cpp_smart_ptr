`std::share_prt`

`std:unique_ptr`

`std::weak_ptr`

### 深拷贝与浅拷贝

- 深拷贝优缺点
    - 优点：每一个对象（即使是通过拷贝构造函数实例化的对象）的指针都有指向的内存空间，而不是共享，这样在释放的时候就不存在重复释放或者是内存泄漏问题了
    - 缺点：内存开销大
- 浅拷贝优缺点
    - 通过拷贝构造函数实例化的指针数据变量指向的是共享的内存空间，因此内存开销小。
    - 对象析构的时候可能会重复释放或造成内存泄漏。

## 引用计数：

### 对浅拷贝进行优化

```cpp
struct RefValue 
{

	RefValue(const char* pszName);
	~RefValue();

	void AddRef();
	void Release();

	char* m_pszName;
	int   m_nCount;
};

RefValue::RefValue(const char* pszName)
{
	m_pszName = new char[strlen(pszName)+1];
	
	m_nCount = 1;

}

RefValue::~RefValue()
{

	if (m_pszName != NULL)
	{
		delete m_pszName;
		m_pszName = NULL;
	}
}

void RefValue::AddRef()
{
	m_nCount++;
}

void RefValue::Release()
{
	if (--m_nCount == 0)
	{
		delete this;
	}
	
}
```

在进行浅拷贝时加入一个变量保存引用次数，引用一次数值+1，释放一次数值-1，当引用计数为0时，进行delete或者free();

仍然存在的问题：

- 如果对其中某一个类对象中的资源进行了修改，那么所有引用该资源的对象全部会被修改，这显然是错误的

## 写时拷贝：

 使用引用计数时，当发生共享资源被改变的时候，将共享资源重新复制一份进行修改，不改变共享资源本身。

## 智能指针实现：

首先一个智能指针是一个类，通过构造函数与析构函数实现资源的分配与回收：

```cpp
#include <iostream>

using namespace std;

class smart_ptr{
public:
    explicit smart_ptr(){
        _ptr = new int;
        cout<<"alloc ptr."<<endl;
    }
    ~smart_ptr(){
        delete _ptr;
        cout<<"delete ptr."<<endl;
    }
    int* get(){
        return _ptr;
    }
private:
    int* _ptr;
};

int main(){
    smart_ptr ptr;
    return 0;
}
```

这个类可以完成智能指针的最基本功能：对超出作用域的对象进行释放，但它缺少了：

- 这个类只适用于int
- 这个类用起来不像指针
- 拷贝该类对象会引发程序行为异常

### 模板化：

要让这个类能够包装任意类型的指针，需要把它变成一个类模板：

```cpp
#include <iostream>

using namespace std;

class A{
public:
    void get(){
    cout<<"the val of this class is:"<<val<<endl;
}
private:
    int val = 1;
};
template <typename T>
class smart_ptr{
public:
    explicit smart_ptr(T* ptr = nullptr): _ptr(ptr){
        cout<<"alloc ptr."<<endl;
    }
    ~smart_ptr(){
        delete _ptr;
        cout<<"delete ptr."<<endl;
    }
    T* get(){
        return _ptr;
    }
private:
    T* _ptr;
};

int main(){
    smart_ptr<A> ptr(new A);
    ptr.get()->get();
    smart_ptr<int> ptr2(new int(5));
    cout<<"the value of ptr2:"<<*(ptr2.get())<<endl;
    return 0;
}
```

好了，通过将这个类模板化，我们可以将这个智能指针用于任何类型。

输出为：

```
alloc ptr.
the val of this class is:1
alloc ptr.
the value of ptr2:5
delete ptr.
delete ptr.
```

### 运算符重载：

为了让这个类用起来更像一个指针，我们还需要对以下的运算符进行重载：

```cpp
T& operator*() const{ return *_ptr;}
T* operator->() const{ return _ptr;}
```

```cpp
int main(){
    smart_ptr<A> ptr(new A);
    ptr->get();
    (*ptr).get();
    return 0;
}
```

输出：

```cpp
alloc ptr.
the val of this class is:1
the val of this class is:1
delete ptr.
```

### 拷贝构造和赋值：

我们应该如何定义拷贝的行为，假设有如下代码：

```cpp
smart_ptr<A> ptr1(new B); //B是A的子类
smart_ptr<A> ptr2{ptr1};
```

```cpp
template <typename T>
class smart_ptr {
  …
  smart_ptr(smart_ptr& other)
  {
    ptr_ = other.release();//拷贝构造函数让传入的对象失去所有权，同时拷贝对象接管所有权
  }
  smart_ptr& operator=(smart_ptr& rhs)
  {
		//这里首先调用拷贝构造函数创建一个临时类smart_ptr(rhs)，this指针和rhs交换
		//临时对象拿到this之前维护的指针随着临时对象的析构回收
    smart_ptr(rhs).swap(*this);
    return *this;
  }
  …
  T* release()
  {
    T* ptr = ptr_;
    ptr_ = nullptr;
    return ptr;
  }
  void swap(smart_ptr& rhs)
  {
    using std::swap;
    swap(ptr_, rhs.ptr_);
  }
  …
};
```

### 移动指针：

```cpp
template <typename T>
class smart_ptr {
  …
  smart_ptr(smart_ptr&& other)
  {
    ptr_ = other.release();
  }
  smart_ptr& operator=(smart_ptr rhs)
  {
    smart_ptr(rhs).swap(*this);
    return *this;
  }
  …
};
```

修改的地方：

- 把拷贝构造函数中的参数类型  `smart_ptr&` 改成了  `smart_ptr&&`；现在它成了移动构造函数。
- 把赋值函数中的参数类型  `smart_ptr&` 改成了 `smart_ptr`，在构造参数时直接生成新的智能指针，从而不再需要在函数体中构造临时对象。现在赋值函数的行为是移动还是拷贝，完全依赖于构造参数时走的是移动构造还是拷贝构造。

根据 C++ 的规则，如果我提供了移动构造函数而没有手动提供拷贝构造函数，那后者自动被禁用

```cpp
int main(){
    smart_ptr<A> ptr1{new B};
    //ptr->get();
    //(*ptr).get();
    smart_ptr<A> ptr2{std::move(ptr1)};
    ptr2->get();
    return 0;
}
```

输出为：

```cpp
alloc ptr.
the val of this class is:1
delete ptr.
delete ptr.
```

### 子类指针向基类指针的转换：

```cpp
template<typename U>
    explicit smart_ptr(smart_ptr<U>&& other){
        _ptr = other.relese();
    }
```

### 引用计数：

上面的智能指针基本完成了unique_ptr的功能，一个对象只能被单个unique_ptr拥有，显然不能满足所有使用场景，一种常见的情况是：多个智能指针同时拥有一个对象，当他们全部失效时，这个对象也同时会被删除，这也就是shared_ptr了。多个不同的shared_ptr不仅可以共享一个对象，同时还应共享同一计数器。

计数器接口：

```cpp
class share_count{
public:
    share_count();
    void add_account();
    long reduce_count();
    long get_count() const;
};
```

这个share_count类除构造函数外有三个方法：增加计数，减少计数，获取计数。增加计数不需要返回值，减少计数需要返回当前计数以判断是不是最后一个共享计数的share_ptr。

```cpp
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
```

加下来我们就可以将引用计数器加入到我们的智能指针里了：

```cpp
template <typename T>
class smart_ptr{
    explicit smart_ptr(T* ptr = nullptr): _ptr(ptr) {
     if(ptr){
         _shared_count = new shared_count();
     }   
    }
    ~smart_ptr(){
        if(_ptr && !_shared_count->reduce_count()){
            delete _ptr;
            delete _shared_count;
        }
    }
private:
    smart_ptr* _ptr;
    shared_count* _shared_count;
};
```

逻辑很清晰，当然我们还有更多细节需要处理：

```cpp
void swap(smart_ptr& rhs){
        using std::swap;
        swap(_ptr, rhs._ptr);
        swap(_shared_count, rhs._shared_count);
    }
```

赋值函数可以不变，但是拷贝构造函数和移动构造函数需要更新：

```cpp
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
    explicit smart_ptr(const smart_ptr<U>& other){
        _ptr = other._ptr;
        if(_ptr){
            other._shared_count->add_account();
            _shared_count = other._shared_count;
        }
    }
    //移动构造函数
    template<typename U>
    explicit smart_ptr(smart_ptr<U>&& other){
        _ptr = other._ptr;
        if(_ptr){
            _shared_count = other._shared_count;
            other._ptr = nullptr;
        }
    }
```

因为模板的实例之间并不天然就有friend关系，所以上面的代码会报错，我们需要在smart_ptr的定义中显式的声明：

```cpp
template<typename U>
friend class smart_ptr;
```

添加一个获取当前引用计数的函数：

```cpp
long use_count() const{
        if(_ptr){
            return _shared_count->get_count();
        }else{
            return 0;
        }
    }
```

测试一下：

```cpp
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
int main(){
    smart_ptr<B> ptr1{new B};
    cout<<"use count of ptr1 is:"<<ptr1.use_count()<<endl;
    smart_ptr<A> ptr2;
    cout<<"use count of ptr2 is:"<<ptr2.use_count()<<endl;
    ptr2 = ptr1;
    cout<<"use count of ptr2 is:"<<ptr2.use_count()<<endl;
    cout<<"use count of ptr1 is:"<<ptr1.use_count()<<endl;
    return 0;
}
```

输出为：

```cpp
use count of ptr1 is:1
use count of ptr2 is:0
use count of ptr2 is:2
use count of ptr1 is:2
```

### 指针类型转换：

首先需要添加构造函数，允许在对智能指针内部的指针对象赋值时，使用一个现有的智能指针的共享计数：

```cpp
template<typename U>
    smart_ptr(const smart_ptr<U>& other, T* ptr){
        _ptr = ptr;
        if(_ptr){
            other._shared_count->add_account();
            _shared_count = other._shared_count;
        }
    }
```

```cpp
template<typename T, typename U>
smart_ptr<T> dynamic_pointer_cast(const smart_ptr<U>& other){
    T* ptr = dynamic_cast<T*>(other.get());
    return smart_ptr<T>(other, ptr);
}
```

测试：

```cpp
int main(){
    smart_ptr<B> ptr1{new B};
    cout<<"use count of ptr1 is:"<<ptr1.use_count()<<endl;
    smart_ptr<A> ptr2;
    cout<<"use count of ptr2 is:"<<ptr2.use_count()<<endl;
    ptr2 = ptr1;
    cout<<"use count of ptr2 is:"<<ptr2.use_count()<<endl;
    cout<<"use count of ptr1 is:"<<ptr1.use_count()<<endl;
    smart_ptr<B> ptr3 = dynamic_pointer_cast<B>(ptr2);
    cout<<"use count of ptr3 is:"<<ptr3.use_count()<<endl;
    return 0;
```

输出：

```cpp
use count of ptr1 is:1
use count of ptr2 is:0
use count of ptr2 is:2
use count of ptr1 is:2
use count of ptr3 is:3
```

完整代码(待完善)：

```cpp
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
```

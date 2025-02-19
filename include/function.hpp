#include <exception>
#include "unique_ptr.hpp"
#include <memory>
#include <type_traits>
#include "utility.hpp"
namespace hstl {

class bad_function_call: public std::exception {
  virtual const char* what() const noexcept {
    return "bad_function_call";
  }
};

template<typename>
class function;

template<typename R, typename... Args>
class function<R(Args...)> {
public:
  function() noexcept = default;
  function(std::nullptr_t) noexcept {}
  function(const function& other) {
    // 类型擦除了怎么知道other中保存的可调用类型具体是什么？
    // 使用clone可以通过多态找到实际的类型
    f = other.f->clone();
  }
  function(function&& other): f(other.f) {
    other.f = nullptr;
  }

  // func的可能类型为：
  // 1. 普通函数指针
  // 2. 成员函数指针
  // 3. functor
  // 4. lambda表达式
  template<typename F>
  function(F&& func): f(new FuncImpl<F>(hstl::forward<F>(func))) {}

  ~function() {
    delete f;
  }

  R operator()(Args... args) {
    if (f == nullptr) {
      throw bad_function_call();
    }
    return (*f)(hstl::forward<Args>(args)...);
  }
private:
  struct FuncBase {
    virtual R operator()(Args... args) = 0;
    // 由于类型擦除，存放的可调用对象F的具体类型是不知道的，
    // clone用于实际的可调用对象类型F的拷贝，类似于工厂方法，
    // 这里的工厂就是自己
    virtual FuncBase* clone() = 0;
    virtual ~FuncBase() = default;
  };
  template<typename F>
  struct FuncImpl: FuncBase {
    // 用于去除F的引用，避免两个构造函数发生冲突
    using FuncType = std::decay_t<F>;
    FuncType f;
    explicit FuncImpl(const FuncType& func): f(func) {}
    explicit FuncImpl(FuncType&& func) : f(hstl::move(func)) {}
    FuncBase* clone() override {
      return new FuncImpl(f);
    }
    FuncImpl(const FuncImpl& fi): f(fi.f) {}
    R operator()(Args... args) override {
      return f(hstl::forward<Args>(args)...);
    }
  };

  std::aligned_storage<sizeof(void *) * 3> buffer;
  FuncBase* f;
};

}
#ifndef MYJSON_PARSER_JOBJECT_H
#define MYJSON_PARSER_JOBJECT_H

#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace json {
/* 定义这些是为了方便使用标准库 */

using std::get_if; /*get_if 是 std::variant的一个函数*/
using std::map;
using std::string;
using std::string_view;
using std::stringstream;
using std::variant;
using std::vector;

/* === 枚举类型，用来标记json的数据类型 === */
enum TYPE { T_NULL, T_BOOL, T_INT, T_DOUBLE, T_STR, T_LIST, T_DICT };

class JObject;
/* json的数据类型 <==> C++的数据类型 */
using null_t = string;
using int_t = int32_t;
using bool_t = bool;
using double_t = double;
using str_t = string;
/* 因为list是可以嵌套的，所以这里用一个 JObject的vector来实现嵌套的效果 */
using list_t = vector<JObject>;
/* json的字典其实就是一个C++的map，
 * 或者是 FIXME: unordered_map 相比 map 也许性能会提高*/
using dict_t = std::unordered_map<string, JObject>;
/* 用于在 __编译时__确定两个变量的类型，使用 is_same
 * 模板类，它返回bool值表示两个类型是否相同 */

/**
 * 使用 if
 * constexpr在编译期判断一个类型是否是基本数据类型，在编译期间对传入的类型进行检查，
 * 并在函数调用时返回一个bool值（是基本类型：true，不是false）。
 * 这里我们定义的基本类型是：string、bool、double、int。
 * 判断类型的操作主要是依靠 std::is_same
 * @functional 主要作用：方便后续根据对应的类型，执行对于的操作
 * @tparam T
 * @return bool
 */
#define IS_TYPE(typeA, typeB) std::is_same<typeA, typeB>::value
template <class T> constexpr bool is_basic_type() {
  if constexpr (IS_TYPE(T, str_t) || IS_TYPE(T, bool_t) ||
                IS_TYPE(T, double_t) || IS_TYPE(T, int_t))
    return true;
  return false;
}
/*
 ======================================================================
 |                         JObject 类定义开始                          |
 ======================================================================
 */
class JObject {
public:
  /* 这里的作用就是定义类型，为了代码简洁 */
  using value_t = variant<bool_t, int_t, double_t, str_t, list_t, dict_t>;

  JObject() /*键值 ,类型默认为null类型*/
  {
    m_type = T_NULL;
    m_value = "null";
  }

  /*TODO：隐式转化在C++里有个坑，只能为类提供一种方向的隐式转化，比如提供了int把转为
   * JObject的隐式转化后，就不能再提供把JObject转为int的隐式转化了，这两种必须要有一个是explicit，否则报错*/
  /*但是这里我们不需要加 explicit,因为要的效果就是隐式转换*/
  /*****************************************
   * start:下面就是传入不同类型，对构造函数的重写，FIXME：这里写这么多构造函数的意义就是
   * 保证 = 的时候能够 隐式转换
   * ***************************************/
  JObject(int_t value) { Int(value); }
  JObject(bool_t value) { Bool(value); }
  JObject(double_t value) { Double(value); }
  JObject(str_t const &value) { Str(value); }
  JObject(list_t value) { List(std::move(value)); }
  JObject(dict_t value) { Dict(std::move(value)); }
  void Null() {
    m_type = T_NULL;
    m_value = "null";
  }
  void Int(int_t value) {
    m_value = value;
    m_type = T_INT;
  }
  void Bool(bool_t value) {
    m_value = value;
    m_type = T_BOOL;
  }
  void Double(double_t value) {
    m_type = T_DOUBLE;
    m_value = value;
  }
  void Str(string_view value) {
    m_value = string(value);
    m_type = T_STR;
  }
  void List(list_t value) {
    m_value = std::move(value);
    m_type = T_LIST;
  }
  void Dict(dict_t value) {
    m_value = std::move(value);
    m_type = T_DICT;
  }
  /************************
   * end：构造函数重载
   *************************/

#define THROW_GET_ERROR(erron)                                                 \
  throw std::logic_error("type error in get " #erron " value!")
  /**
   * 获取 JObject 内部的 任意类型数据（泛型）
   * 内部有调用value()方法得到对应的数据指针，而Value方法则负责将指针强转，
   * 其内部也实现了强大的错误处理，防止处理指针的意外宕机。
   * @tparam V
   * @return
   */
  template <class V> V &Value() {
    /*下面的if constexpr 主要是为了安全检查，防止莫名其妙的宕机行为*/
    if constexpr (IS_TYPE(V, str_t)) {
      if (m_type != T_STR)
        THROW_GET_ERROR(string);
    } else if constexpr (IS_TYPE(V, bool_t)) {
      if (m_type != T_BOOL)
        THROW_GET_ERROR(BOOL);
    } else if constexpr (IS_TYPE(V, int_t)) {
      if (m_type != T_INT)
        THROW_GET_ERROR(INT);
    } else if constexpr (IS_TYPE(V, double_t)) {
      if (m_type != T_DOUBLE)
        THROW_GET_ERROR(DOUBLE);
    } else if constexpr (IS_TYPE(V, list_t)) {
      if (m_type != T_LIST)
        THROW_GET_ERROR(LIST);
    } else if constexpr (IS_TYPE(V, dict_t)) {
      if (m_type != T_DICT)
        THROW_GET_ERROR(DICT);
    }
    /*这里 value()返回的是对应类型数据的指针，再将他转为 void*
     * FIXME:这样做的目的：因为void*指针可以转任意类型的指针 */
    void *v = value();
    if (v == nullptr)
      throw std::logic_error("unknown type in JObject::Value()");
    return *((V *)v); /*FIXME: V是泛型，这里将 void* 转为
                V类型的指针，再解引用，所以最终返回的是 一个引用 */
  }
  /**
   * 返回JObject的类型
   * @return
   */
  TYPE Type() { return m_type; }

  string to_string();
  /**
   * 为list类型的数据定义一个push_back方法
   * @param item
   */
  void push_back(JObject item) {
    if (m_type == T_LIST) {
      auto &list = Value<list_t>();
      list.push_back(std::move(item));
      return;
    }
    /*如果调用这个函数的不是 list，那么抛出异常*/
    throw std::logic_error("not a list type! JObjcct::push_back()");
  }

  void pop_back() {
    if (m_type == T_LIST) {
      auto &list = Value<list_t>();
      list.pop_back();
      return;
    }
    throw std::logic_error("not list type! JObjcct::pop_back()");
  }
  /**
   * 重载了 下标运算符，使得 dict的类型可以直接 dict[i] = xx;
   * @param key
   * @return
   */
  JObject &operator[](string const &key) {
    if (m_type == T_DICT) {
      auto &dict = Value<dict_t>();
      return dict[key];
    }
    throw std::logic_error("not dict type! JObject::opertor[]()");
  }

private:
  // 根据类型获取值的地址，直接硬转为void*类型，然后外界调用Value函数进行类型的强转
  void *value();

private:
  /* JObject需要两种数据，第一个就是 tag ： 标识了当前存的是什么样的数据，
   *                     第二个是 实际存储的数据*/
  TYPE m_type;     /* 枚举类型 */
  value_t m_value; /* variant类型，存json的键值实际数据  */
};
/*
 ======================================================================
 |                         JObject 类定义结束                          |
 ======================================================================
 */

/* FIXME:下面是写的方法 */
void *JObject::value() {
  /*调用get_if得到对应的数据指针（std::variant 获取数据的一种方式）
   * get得到的是对象的引用，如果获取不到，则抛出异常，get_if获取对象的指针，如果获取不到则返回nullptr
   *FIXME: 使用get_if的原因是:
   *这个异常的处理可以由你自己来设定提示，而不是对着底层的get提示而摸不着头脑。*/
  switch (m_type) { /*根据类型获取值*/
  case T_NULL:
    return get_if<str_t>(&m_value);
  case T_BOOL:
    return get_if<bool_t>(&m_value);
  case T_INT:
    return get_if<int_t>(&m_value);
  case T_DOUBLE:
    return get_if<double_t>(&m_value);
  case T_LIST:
    return get_if<list_t>(&m_value);
  case T_DICT:
    return get_if<dict_t>(&m_value);
  case T_STR:
    return std::get_if<str_t>(&m_value);
  default:
    return nullptr;
  }
}
/*用于简化指针强转过程的宏*/
#define GET_VALUE(type) *((type *)value)
/**
 * 序列化
 * 把JObject转化为string类型的数据，相当于把序列化的过程反推一遍
 * @return
 */
std::string JObject::to_string() {
  void *value = this->value();
  std::ostringstream OutStream; /*定义输出流，向字符串写入数据*/
  switch (m_type) {
  case T_NULL:
    OutStream << "null";
    break;
  case T_BOOL:
    if (GET_VALUE(bool))
      OutStream << "true";
    else
      OutStream << "false";
    break;
  case T_INT:
    OutStream << GET_VALUE(int);
    break;
  case T_DOUBLE:
    OutStream << GET_VALUE(double);
    break;
  case T_STR:
    OutStream << '\"' << GET_VALUE(string) << '\"';
    break;
  case T_LIST: { /*FIXME：如果是列表的话，只需要遍历他的每一个元素，递归调用
            to_string()方法*/
    list_t &list = GET_VALUE(list_t);
    OutStream << '['; /*注意在最开始的时候加上 左括号 `[` */
    for (auto i = 0; i < list.size(); i++) {
      /*遍历列表*/
      if (i != list.size() - 1) {
        OutStream << ((list[i]).to_string());
        OutStream << ','; /*注意，在中间还需要输出 ， 逗号*/
      } else
        OutStream << ((list[i]).to_string()); /*如果是最后一个元素的话，不用输出
                                   逗号 了*/
    }
    OutStream << ']'; /*注意在最结束的时候加上 右括号 `[` */
    break;
  }
  case T_DICT: { /*如果是字典，*/
    dict_t &dict = GET_VALUE(dict_t);
    OutStream << '{'; /*先输出 { */
    for (auto it = dict.begin(); it != dict.end(); ++it) {
      if (it != dict.begin()) /*为了保证最后的json格式正确，中间要输出逗号*/
        OutStream << ',';
      /* first是key，second是value。对于key，要使用" " 包裹，然后再输出冒号 : */
      OutStream << '\"' << it->first << "\":" << it->second.to_string();
    }
    OutStream << '}'; /* 输出 } ，结束*/
    break;
  }
  default:
    return ""; /*啥都没有，输出空即可*/
  }
  return OutStream.str();
}
} // namespace json

#endif // MYJSON_PARSER_JOBJECT_H

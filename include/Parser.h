#ifndef MYJSON_PARSER_PARSER_H
#define MYJSON_PARSER_PARSER_H

#include "JObject.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
namespace json {
/*===== 用于定义序列化和反序列化函数的函数名 =====*/
#define FUNC_TO_NAME _to_json
#define FUNC_FROM_NAME _from_json

/**---------------------------------
 *|     @start:序列化函数宏定义      |
 * --------------------------------*/
/*=====  序列化函数的起始标志 =====*/
#define START_TO_JSON void FUNC_TO_NAME(json::JObject &obj) const {
/*==将当前对象的成员变量序列化为 JSON 对象的键值对，并将其加入到 JObject
 * 对象中== */
#define to(key) obj[key]
/*将一个自定义类型的成员变量（比如结构体）添加到 JSON 对象中。
 * 首先，创建一个 json::JObject 对象 tmp，用来存储要添加到 JSON 对象中的自定义类型的成员变量。
 * 接着，调用自定义类型的成员变量的 _to_json 函数，将成员变量的值转换为 tmp 对象中的 JSON 对象。
 * 最后，将 tmp 对象作为一个整体，添加到要返回的 JSON 对象中。*/
#define to_struct(key, struct_member)                                          \
  json::JObject tmp((json::dict_t()));                                         \
  struct_member.FUNC_TO_NAME(tmp);                                             \
  obj[key] = tmp
/*=====  序列化函数的结束标志 =====*/
#define END_TO_JSON }
/**---------------------------------
 *|     @end:序列化函数宏定义      |
 * --------------------------------*/

/**---------------------------------
 *|     @start:反序列化函数宏定义      |
 * --------------------------------*/
#define START_FROM_JSON void FUNC_FROM_NAME(json::JObject &obj) {
/*从 JSON 对象中获取指定键名的值，并将其转换为指定类型的变量值。*/
#define from(key, type) obj[key].Value<type>()
/*从 JSON
 * 对象中获取指定键名的值，并将其转换为自定义类型的变量值。该宏会调用结构体或类的
 * _from_json 函数，将 json::JObject 转换为自定义类型的对象。*/
#define from_struct(key, struct_member) struct_member.FUNC_FROM_NAME(obj[key])
/*反序列化函数结束标志*/
#define END_FROM_JSON }
/**---------------------------------
 *|     @end:反序列化函数宏定义      |
 * --------------------------------*/

/*方便使用 std*/
using std::string;
using std::string_view;
using std::stringstream;
/*对应json的类型*/
using null_t = string;
using int_t = int32_t;
using bool_t = bool;
using double_t = double;
using str_t = string;
/*申明JObject类，因为在Parser中会用到*/
class JObject;

/*
 ======================================================================
 |                         Parser 类定义开始                          |
 ======================================================================
 */
class Parser {
public:
  Parser() = default;
  static JObject FromString(string_view content);
  /**@funtional 对任意类型进行 序列化(C++ struct => json字符串) */
  template <class T> static string ToJSON(T const &src);
  /**@funtional 对任意类型进行 反序列化(json字符串 => C++ struct ) */
  template <class T> static T FromJson(string_view src);
  void init(string_view src);
  void trim_right();
  void skip_comment();
  bool is_esc_consume(size_t pos);
  char get_next_token();
  JObject parse();
  JObject parse_null();
  JObject parse_number();
  bool parse_bool();
  string parse_string();
  JObject parse_list();
  JObject parse_dict();

private:
  string m_str;
  size_t m_idx{}; /*当前解析的字符的位置 0 */
};
/*
 ======================================================================
 |                          Parser 类定义结束                          |
 ======================================================================
 */

/**
 * 反序列化
 * 这里创建一个静态的 Parser实例，将上面提到的三步操作封装为一步
 *  虽然这里不是单例模式，但是复用这一个 instance 就够了。
 * @param content
 * @return
 */
JObject Parser::FromString(string_view content) {
  static Parser instance;
  instance.init(content);
  return instance.parse();
}

/**
 * 为什么用 string_view，因为直接用string会经常发生拷贝，导致性能下降。
 * 为什么不用 string_view 仅仅有观察权，没有资源所有权。
 * @param src 需要解析的字符串
 */
void Parser::init(std::string_view src) {
  /* 当前需要解析的字符串，这里赋值实际上调用了一次 拷贝构造函数，而不是
   * 赋值构造函数 */
  m_str = src;
  m_idx = 0; /* 当前已经解析到的字符的位置 下标 */
  /* 去末尾除多余空格，FIXME: 防止末尾多余的空格对解析过程产生错误 */
  trim_right();
}

/**
 * 去除尾部空字符，方便最后的结束判断
 */
void Parser::trim_right() {
  /* 使用一个 反向迭代器 和 find_if
   * 结合，从字符串末尾开始，直到找到第一个不是空格的字符的位置，
   * 然后使用 erase 函数删除这些空格*/
  m_str.erase(std::find_if(m_str.rbegin(), m_str.rend(),
                           [](char ch) { return !std::isspace(ch); })
                  .base(),
              m_str.end());
}
/**
 * 跳过vscode的 // 开头的注释
 */
void Parser::skip_comment() {
  if (m_str.compare(m_idx, 2, R"(//)") == 0) {
    while (true) {
      /*找换行符*/
      auto next_pos = m_str.find('\n', m_idx);
      if (next_pos == string::npos) {
        throw std::logic_error("invalid comment area!");
      }
      /*查看下一行是否还是注释*/
      m_idx = next_pos + 1;
      /*先跳过 // 之前的空格*/
      while (isspace(m_str[m_idx])) {
        m_idx++;
      }
      /*这是找不到注释的就跳出循环*/
      if (m_str.compare(m_idx, 2, R"(//)") != 0) { // 结束注释
        return;
      }
    }
  }
}
/**
 * 获取json字符串中的token
 * {} [] "
 * 这些都是token，而且在token之间，肯定还会有大量的空格，所以第一个while循环的意义就是在这
 * @return
 */
char Parser::get_next_token() {
  /* 这个 while 的功能就是跳过token之间的空白字符
   * 是跳过，而不是删除这些空格，因为这里的操作是让 目前处理的字符位置++*/
  while (std::isspace(m_str[m_idx]))
    m_idx++;
  /* 如果当前处理的字符位置 >= 字符串的大小了，那么直接抛出异常 */
  if (m_idx >= m_str.size())
    throw std::logic_error("unexpected character in parse json");
  /*如果是注释，记得跳过*/
  skip_comment();
  /* 返回当前解析到的token的字符 */
  return m_str[m_idx];
}

bool Parser::is_esc_consume(size_t pos) {
  size_t end_pos = pos;
  while (m_str[pos] == '\\')
    pos--;
  auto cnt = end_pos - pos;
  /*如果 \ 的个数为偶数，则成功抵消，如果为奇数，则未抵消*/
  return cnt % 2 == 0;
}

/**
 * 解析的核心函数
 * @return 返回一个JObject
 */
JObject Parser::parse() {
  /*跳过空白符号，以及跳过注释(只有vscode版的json才有注释，其余的都没有的)*/
  char token = get_next_token();
  if (token == 'n') {    /* 如果解析到的是n，那么则是 null */
    return parse_null(); /* 返回的是一个 JObject */
  }
  if (token == 't' || token == 'f') { /*bool类型的就是 true 或者 false */
    return parse_bool(); /* FIXME: 这里发生了bool类型，隐式转换成了 JObject
                    类型*/
  }
  if (token == '-' ||
      std::isdigit(
          token)) { /*如果是 `-` 负号，或者数字。那么token就是一个数字*/
    return parse_number(); /*FIXME： 这里需要注意double类型的转换*/
  }
  if (token ==
      '\"') { /*这里需要用转义字符 \ ，如果数据带引号，那么就是字符串类型*/
    return parse_string();
  }
  if (token == '[') { /*list的开头*/
    return parse_list();
  }
  if (token == '{') { /*map的开头*/
    return parse_dict();
  }
  /*如果上面的规则，一个都没匹配上，那么说明这个字符不是我们预期的，抛出异常*/
  throw std::logic_error("unexpected character in parse json");
}
/**
 * 假如token是null，那么当时返回的token的首字母是 n 。
 * 往后找到4个字符，再和 "null" 比较，如果相等，则token正确
 *      当前处理的字符位置+4 ，返回的对象是一个空对象
 * @return 空对象null
 */
JObject Parser::parse_null() {
  if (m_str.compare(m_idx, 4, "null") == 0) {
    m_idx += 4;
    return {}; /*创建一个空的初始化列表，发生从 void*到 JObject的隐式转换*/
  }
  /*如果n开头的token，却不是null的话，说明发生了解析错误（json本身就不对）*/
  throw std::logic_error("parse null error");
}
/**
 * 解析数字，包括负数
 * @return
 */
JObject Parser::parse_number() {
  size_t pos = m_idx;
  /*整数部分*/
  if (m_str[m_idx] == '-') {
    m_idx++; /*处理负号*/
  }
  /*遍历完数据的整数部分*/
  if (isdigit(m_str[m_idx]))
    while (isdigit(m_str[m_idx]))
      m_idx++;
  else {
    throw std::logic_error("invalid character in number");
  }
  /* 如果不存在小数点，那么直接返回以上解析出的数字了！*/
  if (m_str[m_idx] != '.') {
    /*strtol的作用是将字符串转换为 long 类型， endptr
     * 指向第一个不可转换的字符位置的指针，base10 表示转换成10进制数*/
    return (int)strtol(m_str.c_str() + pos, nullptr, 10);
  }

  // 处理小数部分
  if (m_str[m_idx] == '.') {
    m_idx++; /*跳过小数点*/
    if (!std::isdigit(m_str[m_idx])) {
      /*如果小数点后没有数字，那么报错*/
      throw std::logic_error(
          "at least one digit required in parse float part!");
    }
    /*处理小数点之后的数字*/
    while (std::isdigit(m_str[m_idx]))
      m_idx++;
  }
  /*使用strtod将字符串转换为 double 类型的数据*/
  return strtod(
      m_str.c_str() + pos,
      nullptr); /* 没有遇到不能被转换的字符，那么endptr会被设置为 nullptr */
}
/**
 * 将字符 true或者false解析为 true或者false
 * @return
 */
bool Parser::parse_bool() {
  if (m_str.compare(m_idx, 4, "true") == 0) {
    m_idx += 4;
    return true;
  }
  if (m_str.compare(m_idx, 5, "false") == 0) {
    m_idx += 5;
    return false;
  }
  /*如果是其他的，则抛出异常*/
  throw std::logic_error("parse bool error");
}

string Parser::parse_string() {
  auto pre_pos = ++m_idx; /*字符串起始位置*/
                          /*找到下一个 " （字符串结束标志）*/
  auto pos = m_str.find('"', m_idx);
  /*FIXME：如果找到了 " 的话，还需要进一步判断，是转义的还是 真正的字符串结束*/
  if (pos != string::npos) {
    /*解析还没有结束，需要判断是否是转义的结束符号，如果是转义，则需要继续探查*/
    while (true) {
      /*如果不是转义则解析结束*/
      if (m_str[pos - 1] != '\\') {
        break;
      }
      /*如果是转义字符 `\`，则判断
       * 是否已经被抵消，已经被消耗完则跳出，否则继续寻找下个字符串结束符 如果
       * \ 的个数为偶数，则成功抵消，那么可以跳出循环，如果为奇数，则未抵消*/
      if (true == is_esc_consume(pos - 1)) {
        break;
      }
      /*从下一个位置开始，再找 " */
      pos = m_str.find('"', pos + 1);
      /*如果没找到*/
      if (pos == string::npos) {
        throw std::logic_error(R"(expected left '"' in parse string)");
      }
    }
    m_idx = pos + 1; /*跳过 左" */
                     /*截取"..."，返回string的内容*/
    return m_str.substr(pre_pos, pos - pre_pos);
  }
  /*如果根本就没找到 " ，那么json格式是错误的 */
  throw std::logic_error("parse string error");
}

JObject Parser::parse_list() {
  /* list_t实际上是 => vector<JObject> */
  JObject arr(
      (list_t())); /* 这里没有使用隐式转换，而是显示的定义了一个 JObject 类 */
  m_idx++;         /*跳过 `[` 字符*/
  char ch = get_next_token(); /*过滤空字符，得到下一个 token */
  if (ch == ']') { /*如果下一个字符是 `]` ，则list结束 ，直接返回*/
    m_idx++;
    return arr;
  }
  /*如果list没有结束，其中包含 那6种基础类型*/
  while (true) {
    arr.push_back(
        parse());          /*FIXME：这里是递归调用了
                  parse，得到基本数据类型（也有可能是list，嵌套过多可能导致爆栈）*/
    ch = get_next_token(); /*再获取下一个token*/
                           /*遇到 ] 说明结束了*/
    if (ch == ']') {
      m_idx++; /*跳过 ] 返回就行*/
      break;
    }
    /*如果不是逗号，报错*/
    if (ch != ',') {
      throw std::logic_error("expected ',' in parse list");
    }
    /*如果是逗号，则 跳过逗号，下面还有字符要解析，再循环*/
    m_idx++;
  }
  /*整个list解析完成*/
  return arr;
}
/**
 * dict_t => map<string, JObject>;
 * @return
 */
JObject Parser::parse_dict() {
  JObject dict((dict_t()));
  m_idx++; /*跳过 { */
  char ch = get_next_token();
  /*如果是 } 则结束*/
  if (ch == '}') {
    m_idx++;
    return dict;
  }
  while (true) {
    /* 首先解析key，这里的
     * .Value<string>方法返回的是parse得到的数据（默认map的key是string类型的）
     */
    /*因为parse().Value()得到的值是匿名对象，返回之后就是死掉了。
将返回的字符串的值转移给key，防止拷贝，原来字符串的值可能变成空或者其他不确定的值*/
    string key = std::move(parse().Value<string>());
    ch = get_next_token();
    /*如果不是 冒号，那么不符合 json 规则了。*/
    if (ch != ':') {
      throw std::logic_error("expected ':' in parse dict");
    }
    m_idx++; /*跳过冒号*/

    /*解析value*/
    dict[key] = parse(); /*FIXME：这里dict重载了下标运算符，可以直接 = 运算*/
    ch = get_next_token();
    /*如果到 }，则结束了*/
    if (ch == '}') {
      m_idx++;
      break; /*FIXME：出口，解析完毕*/
    }
    /*没有结束，此时必须为逗号*/
    if (ch != ',') {
      throw std::logic_error("expected ',' in parse dict");
    }
    /*跳过逗号*/
    m_idx++;
    /*继续循环*/
  }
  return dict;
}
template <class T> T Parser::FromJson(string_view src) {
  JObject object = FromString(src);
  // 如果是基本类型
  if constexpr (is_basic_type<T>()) {
    return object.template Value<T>();
  }

  // 调用T类型对应的成岩函数
  if (object.Type() != T_DICT)
    throw std::logic_error("not dict type fromjson");
  T ret;
  ret.FUNC_FROM_NAME(object);
  return ret;
}
template <class T> string Parser::ToJSON(const T &src) {
  /*如果是基本类型(非dict)，先封装成JObject，才能调用其 ToString的方法*/
  if constexpr (IS_TYPE(T, int_t)) {
    JObject object(src);
    return object.ToString();
  } else if constexpr (IS_TYPE(T, bool_t)) {
    JObject object(src);
    return object.ToString();
  } else if constexpr (IS_TYPE(T, double_t)) {
    JObject object(src);
    return object.ToString();
  } else if constexpr (IS_TYPE(T, str_t)) {
    JObject object(src);
    return object.ToString();
  }
  /*如果是自定义类型调用方法完成dict的赋值，然后ToString即可
   * （自定义类型肯定是 dict 类型）*/
  json::JObject obj((json::dict_t())); /*创建一个空dict*/
  /*需要你对该类型定义对应的方法，该方法需要将值传递给
   * JObject，为了简化这个过程我们用宏来替代。 FUNC_TO_NAME是通过宏，自动生成
   * 对应方法 的*/
  src.FUNC_TO_NAME(obj);
  return obj.ToString();
}
} // namespace json

#endif // MYJSON_PARSER_PARSER_H

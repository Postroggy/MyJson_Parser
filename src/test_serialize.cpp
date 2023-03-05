/*用于测试反序列化与序列化*/
/*Json类*/
#include "../include/Parser.h"
/*sys类*/
#include <iostream>
using namespace json;
struct Base {
  int pp;
  string qq;
  START_FROM_JSON
  pp = from("pp", int);
  qq = from("qq", string);
  END_FROM_JSON

  START_TO_JSON
  to("pp") = pp;
  to("qq") = qq;
  END_TO_JSON
};

struct Mytest {
  int id;
  std::string name;
  Base q;

  START_TO_JSON
  to_struct("base", q);
  to("id") = id;
  to("name") = name;
  END_TO_JSON

  START_FROM_JSON
  id = from("id", int);
  name = from("name", string);
  from_struct("base", q);
  END_FROM_JSON
};

void test_class_serialization() {
  Mytest test{.id = 32, .name = "fda"}; /*先创建一个struct*/
                                        /*测试反序列化*/
  auto item = Parser::FromJson<Mytest>(
      R"({"base":{"pp":0,"qq":""},"id":32,"name":"fda"})");
  /*R是 C++ 11 中引入的，用于表示不作任何转义，R"xxx(原始字符串)xxx"*/
  /*测试序列化*/
  std::cout << Parser::ToJSON(item);
}

int main(int argc, char *argv[]) { test_class_serialization(); }
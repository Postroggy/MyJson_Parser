/*用于测试JSON字符串的解析*/
#include "../include/Parser.h"
#include "../BenchMark_Tool/Timer.cpp"
#include "../BenchMark_Tool/scienum.cpp"
#include <fstream>
#include <iostream>

void test_string_parser() {
  std::ifstream fin(R"(../test_json/test.json)");
  if (!fin) {
    std::cout << "read file error";
    return;
  }
  std::string text((std::istreambuf_iterator<char>(fin)),
                   std::istreambuf_iterator<char>());
  /*测试MyJSONParser*/
  {
    Timer t; /*RAII封装，出作用域打印耗时*/
    auto object = json::Parser::FromString(text);
    /*试试有没有解析成功*/
    std::cout << ((object["[css]"]["editor.suggest.insertMode"]).ToString())
              << "\n";
    std::cout<<"MyJsonParser : ";
  }
  /*测试*/
}
int main(int argc, char *argv[]) { test_string_parser(); }
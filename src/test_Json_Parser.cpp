/*用于测试JSON字符串的解析*/
/*Json类*/
#include "../include/JObject.h"
#include "../include/Parser.h"
/*计时类*/
#include "../BenchMark_Tool/Timer.cpp"
#include "../BenchMark_Tool/scienum.cpp"
/*sys类*/
#include <fstream>
#include <iostream>
using namespace json;

void test_string_parser() {
  std::ifstream fin(R"(../test_json/test.json)");
  if (!fin) {
    std::cout << "read file error";
    return;
  }
  std::string text((std::istreambuf_iterator<char>(fin)),
                   std::istreambuf_iterator<char>());
  {
    Timer t;
    auto object = json::Parser::FromString(text);

    std::cout << ((object["[css]"]["editor.suggest.insertMode"]).ToString())
              << "\n";
  }
}
int main(int argc, char *argv[]) { test_string_parser(); }
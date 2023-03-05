/*用于测试JSON字符串的解析*/
#include "../BenchMark_Tool/Timer.cpp"
#include "../BenchMark_Tool/scienum.cpp"
#include "../include/Parser.h"
#include "../other_include/rapidJson/document.h"
#include "../other_include/simdjson/simdjson.h"
#include <fstream>
#include <iostream>
using std::cout;
using std::endl;
using std::istreambuf_iterator;
using std::string;

void test_rapidJSON(std::ifstream &fin) {
  if (!fin) {
    std::cerr << "无法打开文件" << std::endl;
    return;
  }
  // 将文件内容读入字符串
  string json_str((istreambuf_iterator<char>(fin)),
                  istreambuf_iterator<char>());
  // 创建一个 Document 对象，用于解析 JSON
  rapidjson::Document doc;
  {
    Timer t;
    doc.Parse(json_str.c_str());
    cout << "rapidJSON : ";
  }
}

void test_MyJSON(std::ifstream &fin) {
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
    //    std::cout <<
    //    ((object["[css]"]["editor.suggest.insertMode"]).ToString())
    //              << "\n";
    std::cout << "MyJsonParser : ";
  }
}
void test_simdJson(std::ifstream &fin) {

  if (!fin) {
    std::cout << "read file error";
    return;
  }
  using namespace simdjson;
  ondemand::parser parser;
  padded_string json = padded_string::load(R"(../test_json/large-file.json)") ;
  {
    Timer t;
    ondemand::document  j = parser.iterate(json);
    cout<<"simdjson : ";
  }
}
int main(int argc, char *argv[]) {
  /*读取文件*/
  std::ifstream ifs(R"(../test_json/large-file.json)");
  test_MyJSON(ifs);
  test_rapidJSON(ifs);
  test_simdJson(ifs);
  //  test_nlohmannJSON(ifs); 这个解析时，说JSON格式错误，可能是标准不一样吧
}
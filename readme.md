# 1. 支持vscode类型注释的Json解析器
-[x] 采用递归下降解析json
-[x] header-only的库
-[x] 写着玩，性能上不要有什么期待
# 2. 解析图示
![示例.svg](images/exp.png)
# 3. 用法
使用时，只需要 include 一个头文件：`Parser.h`，JObject用于中间转化，对于用户来说，可以不用理会。
## 3.1 解析JSON文件
见[示例代码](./src/test_Json_Parser.cpp)

# 4. 与其他开源项目的性能对比
测试用的json文件，是我从vscode里面取出来的。
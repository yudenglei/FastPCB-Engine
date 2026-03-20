#!/bin/bash
# 提交脚本 - 请在 Git Bash 或 WSL 中运行

cd /c/Users/Yudl/.qclaw/workspace/FastPCB-Engine

# 初始化 git 仓库（如果不存在）
if [ ! -d .git ]; then
    git init
    git remote add origin https://github.com/yudenglei/FastPCB-Engine.git
fi

# 配置用户信息
git config user.name "StudyU"
git config user.email "1550051771@qq.com"

# 添加所有文件
git add -A

# 提交更改
git commit -m "refactor: Observer模式改为有向图模式

- 修复循环检测逻辑 (wouldCreateCycle)
  - 正确检测 dependent -> dependency 是否会产生环
  - 避免重复依赖导致的循环误判

- 添加完整的设计文档 (docs/DEPENDENCY_GRAPH.md)
  - 变更原因说明
  - 核心概念图解
  - API 参考
  - 使用示例

- 更新 README 说明设计变更
  - 明确说明 Observer 模式 → 有向图模式的变更
  - 添加使用场景示例

- 添加测试构建配置 (tests/CMakeLists.txt)
  - 支持 test_dependency_graph 等测试

- 新增 Python 验证测试 (test_dependency_graph.py)
  - 14 个测试用例全部通过
  - 覆盖链式传播、菱形依赖等核心场景

核心变更:
- 支持表达式变量的传递依赖 (c → b → a)
- 拓扑排序确定计算顺序
- 增量更新只影响下游节点
- 器件对象自动驱动更新

测试验证:
✓ 节点管理
✓ 基础依赖关系
✓ 拓扑排序
✓ 循环检测
✓ 自依赖检测
✓ 传递依赖
✓ 表达式求值
✓ 链式传播 (核心场景)
✓ 菱形依赖"

# 推送到远程
git push -u origin master

echo "提交完成！"

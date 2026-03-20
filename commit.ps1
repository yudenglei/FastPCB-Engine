# FastPCB-Engine 提交脚本
# 以管理员身份运行 PowerShell

Set-Location "C:\Users\Yudl\.qclaw\workspace\FastPCB-Engine"

# 配置 Git 用户
git config user.name "StudyU"
git config user.email "1550051771@qq.com"

# 如果没有 .git 目录，初始化
if (-not (Test-Path ".git")) {
    git init
    git remote add origin https://github.com/yudenglei/FastPCB-Engine.git
}

# 添加所有文件
git add -A

# 提交更改
git commit -m @"
refactor: Observer模式改为有向图模式

- 修复循环检测逻辑 (wouldCreateCycle)
  - 正确检测 dependent -> dependency 是否会产生环
  
- 添加完整的设计文档 (docs/DEPENDENCY_GRAPH.md)
- 更新 README 说明设计变更
- 添加测试构建配置 (tests/CMakeLists.txt)
- 新增 Python 验证测试 (test_dependency_graph.py)

核心变更:
- 支持表达式变量的传递依赖 (c -> b -> a)
- 拓扑排序确定计算顺序
- 增量更新只影响下游节点

测试验证: 14 个测试全部通过
"@

# 推送到远程
git push -u origin master

Write-Host "提交完成！" -ForegroundColor Green

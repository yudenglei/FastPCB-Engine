# -*- coding: utf-8 -*-
"""
@file test_dependency_graph.py
@brief 依赖图核心逻辑验证脚本

用于验证有向图依赖关系的正确性
- 拓扑排序
- 循环检测
- 脏标记传播
- 表达式求值
"""

import unittest
from typing import Dict, List, Set, Optional, Any, Callable
from collections import deque
import re

# ============================================================================
# 依赖图实现
# ============================================================================

class NodeType:
    Variable = "Variable"
    Component = "Component"

class NodeState:
    Clean = "Clean"
    Dirty = "Dirty"
    Computing = "Computing"

class GraphNode:
    def __init__(self, id: int, node_type: str, name: str = ""):
        self.id = id
        self.type = node_type
        self.name = name
        self.expression = ""
        self.value = 0.0
        self.cached_value = 0.0
        self.state = NodeState.Dirty
        self.user_data = None
        self.dependencies: Set[int] = set()  # 依赖的节点 (上游)
        self.dependents: Set[int] = set()     # 依赖我的节点 (下游)

class CycleDetectedException(Exception):
    def __init__(self, message: str, cycle_path: List[int]):
        super().__init__(message)
        self.cycle_path = cycle_path

class DependencyGraph:
    def __init__(self):
        self.nodes: Dict[int, GraphNode] = {}
        self.name_to_id: Dict[str, int] = {}
        self.evaluators: Dict[int, Callable] = {}
        self.edge_count = 0
    
    def add_node(self, id: int, node_type: str, name: str = "", user_data: Any = None) -> bool:
        if id in self.nodes:
            return False
        
        node = GraphNode(id, node_type, name)
        node.user_data = user_data
        self.nodes[id] = node
        
        if name:
            self.name_to_id[name] = id
        
        return True
    
    def remove_node(self, id: int) -> bool:
        if id not in self.nodes:
            return False
        
        node = self.nodes[id]
        
        # 移除所有相关边
        for dep in node.dependencies:
            if dep in self.nodes:
                self.nodes[dep].dependents.discard(id)
                self.edge_count -= 1
        
        for dep in node.dependents:
            if dep in self.nodes:
                self.nodes[dep].dependencies.discard(id)
                self.edge_count -= 1
        
        # 从名称索引移除
        if node.name:
            self.name_to_id.pop(node.name, None)
        
        self.nodes.pop(id)
        self.evaluators.pop(id, None)
        
        return True
    
    def get_node(self, id: int) -> Optional[GraphNode]:
        return self.nodes.get(id)
    
    def get_node_by_name(self, name: str) -> Optional[GraphNode]:
        id = self.name_to_id.get(name)
        return self.nodes.get(id) if id is not None else None
    
    def set_expression(self, var_id: int, expr: str):
        node = self.get_node(var_id)
        if not node or node.type != NodeType.Variable:
            return
        
        node.expression = expr
        node.state = NodeState.Dirty
        
        # 解析依赖并更新图
        deps = self._parse_expression_dependencies(expr)
        
        # 移除旧的依赖关系
        for old_dep in list(node.dependencies):
            if old_dep in self.nodes:
                self.nodes[old_dep].dependents.discard(var_id)
                self.edge_count -= 1
        node.dependencies.clear()
        
        # 添加新的依赖关系
        for dep_id in deps:
            if dep_id in self.nodes and dep_id != var_id:
                if not self._would_create_cycle(var_id, dep_id):
                    node.dependencies.add(dep_id)
                    self.nodes[dep_id].dependents.add(var_id)
                    self.edge_count += 1
    
    def add_dependency(self, dependent: int, dependency: int, check_cycle: bool = True):
        if dependent not in self.nodes or dependency not in self.nodes:
            return
        
        if dependent == dependency:
            raise CycleDetectedException("Self-dependency detected", [dependent, dependency])
        
        # 检查是否会产生循环
        if check_cycle and self._would_create_cycle(dependency, dependent):
            cycle_path = self.find_cycle()
            raise CycleDetectedException("Cycle detected in dependency graph", cycle_path)
        
        # 检查是否已存在
        if dependency in self.nodes[dependent].dependencies:
            return
        
        # 添加边
        self.nodes[dependent].dependencies.add(dependency)
        self.nodes[dependency].dependents.add(dependent)
        self.edge_count += 1
        
        # 标记下游节点为脏
        self.mark_dirty(dependent)
    
    def remove_dependency(self, dependent: int, dependency: int) -> bool:
        if dependent not in self.nodes or dependency not in self.nodes:
            return False
        
        if dependency in self.nodes[dependent].dependencies:
            self.nodes[dependent].dependencies.discard(dependency)
            self.nodes[dependency].dependents.discard(dependent)
            self.edge_count -= 1
            return True
        return False
    
    def mark_dirty(self, node_id: int):
        if node_id not in self.nodes:
            return
        
        self.nodes[node_id].state = NodeState.Dirty
        
        # 递归标记所有下游节点
        queue = deque([node_id])
        visited = {node_id}
        
        while queue:
            current = queue.popleft()
            for dep in self.nodes[current].dependents:
                if dep not in visited:
                    visited.add(dep)
                    self.nodes[dep].state = NodeState.Dirty
                    queue.append(dep)
    
    def mark_clean(self, node_id: int):
        if node_id in self.nodes:
            self.nodes[node_id].state = NodeState.Clean
    
    def get_dirty_nodes(self) -> List[int]:
        return [id for id, node in self.nodes.items() if node.state == NodeState.Dirty]
    
    def get_update_order(self, changed_nodes: List[int] = None) -> List[int]:
        sources = set(changed_nodes) if changed_nodes else set()
        
        if not sources:
            sources = {id for id, node in self.nodes.items() if node.state == NodeState.Dirty}
        
        return self._topological_sort_from(sources)
    
    def _topological_sort_from(self, sources: Set[int]) -> List[int]:
        result = []
        in_degree = {id: len(node.dependencies) for id, node in self.nodes.items()}
        
        queue = deque([id for id, deg in in_degree.items() if deg == 0])
        
        while queue:
            current = queue.popleft()
            result.append(current)
            
            for dep in self.nodes[current].dependents:
                in_degree[dep] -= 1
                if in_degree[dep] == 0:
                    queue.append(dep)
        
        return result
    
    def get_direct_dependents(self, source: int) -> List[int]:
        if source not in self.nodes:
            return []
        return list(self.nodes[source].dependents)
    
    def get_all_dependents(self, source: int) -> List[int]:
        result = []
        visited = set()
        queue = deque([source])
        visited.add(source)
        
        while queue:
            current = queue.popleft()
            for dep in self.nodes[current].dependents:
                if dep not in visited:
                    visited.add(dep)
                    result.append(dep)
                    queue.append(dep)
        
        return result
    
    def get_direct_dependencies(self, node_id: int) -> List[int]:
        if node_id not in self.nodes:
            return []
        return list(self.nodes[node_id].dependencies)
    
    def has_cycle(self) -> bool:
        visit_state = {}
        path = []
        
        for node_id in self.nodes:
            if node_id not in visit_state:
                if self._dfs_detect_cycle(node_id, visit_state, path):
                    return True
        return False
    
    def _dfs_detect_cycle(self, node_id: int, visit_state: Dict[int, int], path: List[int]) -> bool:
        visit_state[node_id] = 1  # 访问中
        path.append(node_id)
        
        if node_id in self.nodes:
            for next_node in self.nodes[node_id].dependents:
                if next_node not in visit_state:
                    if self._dfs_detect_cycle(next_node, visit_state, path):
                        return True
                elif visit_state[next_node] == 1:
                    path.append(next_node)
                    return True
        
        path.pop()
        visit_state[node_id] = 2  # 已完成
        return False
    
    def _would_create_cycle(self, dependent: int, dependency: int) -> bool:
        # 添加依赖: dependent -> dependency
        # 如果从 dependency 可以走到 dependent，就说明会成环
        # 因为 dependency -> ... -> dependent -> dependency 形成环
        visited = set()
        queue = deque([dependency])
        
        while queue:
            current = queue.popleft()
            if current == dependent:
                return True
            
            if current in visited:
                continue
            visited.add(current)
            
            if current in self.nodes:
                for next_node in self.nodes[current].dependents:
                    if next_node not in visited:
                        queue.append(next_node)
        
        return False
    
    def find_cycle(self) -> List[int]:
        visit_state = {}
        path = []
        
        for node_id in self.nodes:
            if node_id not in visit_state:
                path = []
                if self._dfs_detect_cycle(node_id, visit_state, path):
                    # 找到环
                    if path and path[-1] == path[0]:
                        path.pop()
                    return path
        return []
    
    def validate(self) -> bool:
        for node_id, node in self.nodes.items():
            for dep in node.dependencies:
                if dep not in self.nodes:
                    return False
                if node_id not in self.nodes[dep].dependents:
                    return False
            for dep in node.dependents:
                if dep not in self.nodes:
                    return False
                if node_id not in self.nodes[dep].dependencies:
                    return False
        return True
    
    def evaluate(self, node_id: int) -> float:
        node = self.get_node(node_id)
        if not node:
            return 0.0
        
        if node.state == NodeState.Clean:
            return node.cached_value
        
        if node.state == NodeState.Computing:
            raise CycleDetectedException("Circular dependency detected", self.find_cycle())
        
        node.state = NodeState.Computing
        
        result = 0.0
        
        # 如果有自定义求值器，优先使用
        if node_id in self.evaluators:
            dep_values = [self.evaluate(dep_id) for dep_id in node.dependencies]
            result = self.evaluators[node_id](dep_values)
        elif node.expression:
            var_values = {}
            for dep_id in node.dependencies:
                dep_node = self.get_node(dep_id)
                if dep_node:
                    var_values[dep_node.name] = self.evaluate(dep_id)
            result = self._parse_and_eval(node.expression, var_values)
        else:
            result = node.value
        
        node.cached_value = result
        node.state = NodeState.Clean
        
        return result
    
    def refresh_dirty_nodes(self):
        dirty_nodes = self.get_dirty_nodes()
        order = self.get_update_order(dirty_nodes)
        
        for id in order:
            self.evaluate(id)
    
    def recompute_all(self):
        for node in self.nodes.values():
            node.state = NodeState.Dirty
        
        order = self._topological_sort_from(set())
        for id in order:
            self.evaluate(id)
    
    def set_evaluator(self, node_id: int, evaluator: Callable):
        if node_id in self.nodes:
            self.evaluators[node_id] = evaluator
            self.mark_dirty(node_id)
    
    def topological_sort(self) -> Optional[List[int]]:
        if self.has_cycle():
            return None
        return self._topological_sort_from(set())
    
    @property
    def node_count(self) -> int:
        return len(self.nodes)
    
    @property
    def edge_count_prop(self) -> int:
        return self.edge_count
    
    def clear(self):
        self.nodes.clear()
        self.name_to_id.clear()
        self.evaluators.clear()
        self.edge_count = 0
    
    def _parse_expression_dependencies(self, expr: str) -> List[int]:
        deps = []
        # 提取变量名
        pattern = r'\b([a-zA-Z_][a-zA-Z0-9_]*)\b'
        for match in re.finditer(pattern, expr):
            var_name = match.group(1)
            if var_name in self.name_to_id:
                deps.append(self.name_to_id[var_name])
        return deps
    
    def _parse_and_eval(self, expr: str, vars: Dict[str, float]) -> float:
        # 简单表达式解析器
        # 支持: +, -, *, /, (, ), 数字, 变量
        
        def tokenize(expression):
            tokens = []
            i = 0
            while i < len(expression):
                if expression[i].isspace():
                    i += 1
                elif expression[i] in '+-*/()':
                    tokens.append(expression[i])
                    i += 1
                elif expression[i].isdigit() or expression[i] == '.':
                    j = i
                    while j < len(expression) and (expression[j].isdigit() or expression[j] == '.' or expression[j] in 'eE+-'):
                        j += 1
                    tokens.append(expression[i:j])
                    i = j
                elif expression[i].isalpha() or expression[i] == '_':
                    j = i
                    while j < len(expression) and (expression[j].isalnum() or expression[j] == '_'):
                        j += 1
                    tokens.append(expression[i:j])
                    i = j
                else:
                    i += 1
            return tokens
        
        def parse_expr(tokens, pos):
            return parse_add_sub(tokens, pos)
        
        def parse_add_sub(tokens, pos):
            left, pos = parse_mul_div(tokens, pos)
            while pos < len(tokens) and tokens[pos] in '+-':
                op = tokens[pos]
                pos += 1
                right, pos = parse_mul_div(tokens, pos)
                if op == '+':
                    left += right
                else:
                    left -= right
            return left, pos
        
        def parse_mul_div(tokens, pos):
            left, pos = parse_unary(tokens, pos)
            while pos < len(tokens) and tokens[pos] in '*/':
                op = tokens[pos]
                pos += 1
                right, pos = parse_unary(tokens, pos)
                if op == '*':
                    left *= right
                else:
                    left = left / right if right != 0 else 0
            return left, pos
        
        def parse_unary(tokens, pos):
            if pos < len(tokens) and tokens[pos] == '-':
                val, pos = parse_primary(tokens, pos + 1)
                return -val, pos
            return parse_primary(tokens, pos)
        
        def parse_primary(tokens, pos):
            if pos < len(tokens) and tokens[pos] == '(':
                val, pos = parse_add_sub(tokens, pos + 1)
                if pos < len(tokens) and tokens[pos] == ')':
                    pos += 1
                return val, pos
            elif pos < len(tokens) and (tokens[pos].replace('.', '').replace('-', '').replace('+', '').replace('e', '').replace('E', '').isdigit()):
                return float(tokens[pos]), pos + 1
            elif pos < len(tokens) and tokens[pos] in self.name_to_id:
                name = tokens[pos]
                return vars.get(name, 0.0), pos + 1
            return 0.0, pos
        
        tokens = tokenize(expr)
        result, _ = parse_expr(tokens, 0)
        return result


# ============================================================================
# 测试用例
# ============================================================================

class TestDependencyGraph(unittest.TestCase):
    
    def test_node_management(self):
        """测试：节点管理"""
        graph = DependencyGraph()
        
        self.assertTrue(graph.add_node(1, NodeType.Variable, "x"))
        self.assertTrue(graph.add_node(2, NodeType.Variable, "y"))
        self.assertTrue(graph.add_node(3, NodeType.Component, "comp1"))
        
        self.assertEqual(graph.node_count, 3)
        self.assertTrue(1 in graph.nodes)
        self.assertTrue(2 in graph.nodes)
        
        # 重复添加会失败
        self.assertFalse(graph.add_node(1, NodeType.Variable))
        
        # 获取节点
        node1 = graph.get_node(1)
        self.assertIsNotNone(node1)
        self.assertEqual(node1.name, "x")
        self.assertEqual(node1.type, NodeType.Variable)
        
        # 通过名称查找
        node_x = graph.get_node_by_name("x")
        self.assertIsNotNone(node_x)
        self.assertEqual(node_x.id, 1)
        
        # 移除节点
        self.assertTrue(graph.remove_node(2))
        self.assertEqual(graph.node_count, 2)
        self.assertFalse(2 in graph.nodes)
    
    def test_basic_dependencies(self):
        """测试：基础依赖关系"""
        graph = DependencyGraph()
        
        graph.add_node(1, NodeType.Variable, "x")
        graph.add_node(2, NodeType.Variable, "y")
        graph.add_node(3, NodeType.Variable, "a")
        
        graph.add_dependency(3, 1)  # a 依赖 x
        graph.add_dependency(3, 2)  # a 依赖 y
        
        self.assertEqual(graph.edge_count_prop, 2)
        
        # 检查依赖关系
        deps = graph.get_direct_dependencies(3)
        self.assertEqual(len(deps), 2)
        
        dependents = graph.get_direct_dependents(1)
        self.assertEqual(len(dependents), 1)
        self.assertEqual(dependents[0], 3)
    
    def test_topological_sort(self):
        """测试：拓扑排序"""
        graph = DependencyGraph()
        
        # 创建链: x -> a -> b -> c
        graph.add_node(1, NodeType.Variable, "x")
        graph.add_node(2, NodeType.Variable, "a")
        graph.add_node(3, NodeType.Variable, "b")
        graph.add_node(4, NodeType.Variable, "c")
        
        graph.add_dependency(2, 1)  # a -> x
        graph.add_dependency(3, 2)  # b -> a
        graph.add_dependency(4, 3)  # c -> b
        
        order = graph.topological_sort()
        self.assertIsNotNone(order)
        
        # 验证顺序
        pos = {id: order.index(id) for id in order}
        self.assertTrue(pos[1] < pos[2])
        self.assertTrue(pos[2] < pos[3])
        self.assertTrue(pos[3] < pos[4])
    
    def test_cycle_detection(self):
        """测试：循环检测"""
        graph = DependencyGraph()
        
        graph.add_node(1, NodeType.Variable, "x")
        graph.add_node(2, NodeType.Variable, "y")
        graph.add_node(3, NodeType.Variable, "z")
        
        # 注意：依赖方向是 dependent -> dependency
        # y 依赖 x 表示：x -> y (x 是上游，y 是下游)
        graph.add_dependency(2, 1)  # y 依赖 x
        graph.add_dependency(3, 2)  # z 依赖 y
        
        self.assertFalse(graph.has_cycle())
        
        # 添加循环: z 依赖 x (会成环: x -> y -> z -> x)
        with self.assertRaises(CycleDetectedException):
            graph.add_dependency(1, 3)  # x 依赖 z -> 成环
    
    def test_self_dependency(self):
        """测试：自依赖检测"""
        graph = DependencyGraph()
        
        graph.add_node(1, NodeType.Variable, "x")
        
        with self.assertRaises(CycleDetectedException):
            graph.add_dependency(1, 1)  # 自依赖
    
    def test_transitive_dependents(self):
        """测试：传递依赖"""
        graph = DependencyGraph()
        
        # 创建: x -> a -> b -> c, a -> d
        graph.add_node(1, NodeType.Variable, "x")
        graph.add_node(2, NodeType.Variable, "a")
        graph.add_node(3, NodeType.Variable, "b")
        graph.add_node(4, NodeType.Variable, "c")
        graph.add_node(5, NodeType.Variable, "d")
        
        graph.add_dependency(2, 1)  # a -> x
        graph.add_dependency(3, 2)  # b -> a
        graph.add_dependency(4, 3)  # c -> b
        graph.add_dependency(5, 2)  # d -> a
        
        all_deps = graph.get_all_dependents(1)  # 从 x 出发
        
        # 应该找到 a, b, c, d
        self.assertEqual(len(all_deps), 4)
        self.assertIn(2, all_deps)
        self.assertIn(3, all_deps)
        self.assertIn(4, all_deps)
        self.assertIn(5, all_deps)
    
    def test_expression_evaluation(self):
        """测试：表达式求值"""
        graph = DependencyGraph()
        
        graph.add_node(1, NodeType.Variable, "a")
        graph.add_node(2, NodeType.Variable, "b")
        graph.add_node(3, NodeType.Variable, "c")
        graph.add_node(4, NodeType.Variable, "result")
        
        graph.get_node(1).value = 10.0
        graph.get_node(2).value = 20.0
        graph.get_node(3).value = 5.0
        
        # set_expression 会自动解析依赖，不需要手动添加
        graph.set_expression(4, "a + b * c")
        
        result = graph.evaluate(4)
        
        # a + b * c = 10 + 20 * 5 = 110
        self.assertAlmostEqual(result, 110.0, places=3)
    
    def test_expression_dependency_parsing(self):
        """测试：表达式依赖解析"""
        graph = DependencyGraph()
        
        graph.add_node(1, NodeType.Variable, "x")
        graph.add_node(2, NodeType.Variable, "y")
        graph.add_node(3, NodeType.Variable, "result")
        
        graph.get_node(1).value = 10.0
        graph.get_node(2).value = 20.0
        
        graph.set_expression(3, "x + y")
        
        deps = graph.get_direct_dependencies(3)
        self.assertEqual(len(deps), 2)
        self.assertIn(1, deps)
        self.assertIn(2, deps)
        
        result = graph.evaluate(3)
        self.assertAlmostEqual(result, 30.0, places=3)
    
    def test_complex_expressions(self):
        """测试：复杂表达式"""
        graph = DependencyGraph()
        
        graph.add_node(1, NodeType.Variable, "a")
        graph.add_node(2, NodeType.Variable, "b")
        graph.add_node(3, NodeType.Variable, "c")
        graph.add_node(4, NodeType.Variable, "result")
        
        graph.get_node(1).value = 10.0
        graph.get_node(2).value = 3.0
        graph.get_node(3).value = 2.0
        
        # (a + b) * c = (10 + 3) * 2 = 26
        graph.set_expression(4, "(a + b) * c")
        # set_expression 自动解析依赖，不需要手动添加
        
        result = graph.evaluate(4)
        self.assertAlmostEqual(result, 26.0, places=3)
        
        # a - b / c = 10 - 3 / 2 = 8.5
        graph.get_node(4).expression = "a - b / c"
        graph.get_node(4).state = NodeState.Dirty
        result = graph.evaluate(4)
        self.assertAlmostEqual(result, 8.5, places=3)
    
    def test_dirty_marking(self):
        """测试：脏标记和增量更新"""
        graph = DependencyGraph()
        
        graph.add_node(1, NodeType.Variable, "x")
        graph.add_node(2, NodeType.Variable, "a")
        graph.add_node(3, NodeType.Variable, "b")
        
        graph.add_dependency(2, 1)
        graph.add_dependency(3, 2)
        
        dirty = graph.get_dirty_nodes()
        self.assertEqual(len(dirty), 3)
        
        graph.recompute_all()
        
        dirty = graph.get_dirty_nodes()
        self.assertEqual(len(dirty), 0)
        
        graph.get_node(1).value = 100.0
        graph.mark_dirty(1)
        
        dirty = graph.get_dirty_nodes()
        self.assertEqual(len(dirty), 3)
        
        graph.refresh_dirty_nodes()
        
        dirty = graph.get_dirty_nodes()
        self.assertEqual(len(dirty), 0)
    
    def test_custom_evaluator(self):
        """测试：自定义求值器"""
        graph = DependencyGraph()
        
        graph.add_node(1, NodeType.Variable, "x")
        graph.add_node(2, NodeType.Variable, "y")
        graph.add_node(3, NodeType.Variable, "max")
        
        graph.get_node(1).value = 10.0
        graph.get_node(2).value = 20.0
        
        graph.set_evaluator(3, lambda deps: max(deps[0], deps[1]) if deps else 0)
        graph.add_dependency(3, 1)
        graph.add_dependency(3, 2)
        
        result = graph.evaluate(3)
        self.assertAlmostEqual(result, 20.0, places=3)
    
    def test_component_nodes(self):
        """测试：组件节点"""
        graph = DependencyGraph()
        
        graph.add_node(1, NodeType.Variable, "width")
        graph.add_node(2, NodeType.Variable, "height")
        graph.add_node(3, NodeType.Component, "resistor1")
        
        graph.add_dependency(3, 1)
        graph.add_dependency(3, 2)
        
        graph.recompute_all()
        
        graph.get_node(1).value = 100.0
        graph.mark_dirty(1)
        
        dirty = graph.get_dirty_nodes()
        self.assertEqual(len(dirty), 2)
    
    def test_chain_propagation(self):
        """测试：链式传播 (核心场景: c → b → a)"""
        graph = DependencyGraph()
        
        # 创建: d -> b -> a (当 d 变化时，b 和 a 都应该被标记)
        graph.add_node(1, NodeType.Variable, "d")
        graph.add_node(2, NodeType.Variable, "b")
        graph.add_node(3, NodeType.Variable, "a")
        
        graph.add_dependency(2, 1)  # b 依赖 d
        graph.add_dependency(3, 2)  # a 依赖 b
        
        # d 变化时的传递依赖
        all_deps = graph.get_all_dependents(1)  # 从 d 出发
        self.assertIn(2, all_deps)  # b 应该被影响
        self.assertIn(3, all_deps)  # a 应该被影响
        
        # 设置值并验证增量更新
        graph.get_node(1).value = 10.0
        
        # 设置表达式 (会自动解析依赖)
        graph.set_expression(2, "d * 2")  # b = d * 2
        graph.set_expression(3, "b + 5")  # a = b + 5
        # set_expression 会自动添加依赖，不需要手动调用 add_dependency
        
        graph.recompute_all()
        
        # b = d * 2 = 20, a = b + 5 = 25
        self.assertAlmostEqual(graph.get_node(2).cached_value, 20.0, places=3)
        self.assertAlmostEqual(graph.get_node(3).cached_value, 25.0, places=3)
        
        # 修改 d
        graph.get_node(1).value = 100.0
        graph.mark_dirty(1)
        
        # 增量更新
        graph.refresh_dirty_nodes()
        
        # b = 100 * 2 = 200, a = 200 + 5 = 205
        self.assertAlmostEqual(graph.get_node(2).cached_value, 200.0, places=3)
        self.assertAlmostEqual(graph.get_node(3).cached_value, 205.0, places=3)
    
    def test_diamond_dependency(self):
        """测试：菱形依赖"""
        graph = DependencyGraph()
        
        #    d
        #  ↙   ↘
        # b     c
        #  ↘   ↙
        #    a
        graph.add_node(1, NodeType.Variable, "d")
        graph.add_node(2, NodeType.Variable, "b")
        graph.add_node(3, NodeType.Variable, "c")
        graph.add_node(4, NodeType.Variable, "a")
        
        graph.add_dependency(2, 1)  # b 依赖 d
        graph.add_dependency(3, 1)  # c 依赖 d
        graph.add_dependency(4, 2)  # a 依赖 b
        graph.add_dependency(4, 3)  # a 依赖 c
        
        # 当 d 变化时，b, c, a 都应该被影响
        all_deps = graph.get_all_dependents(1)
        self.assertEqual(len(all_deps), 3)
        
        # 验证拓扑排序仍然有效
        order = graph.topological_sort()
        self.assertIsNotNone(order)
        self.assertTrue(order.index(1) < order.index(2))
        self.assertTrue(order.index(1) < order.index(3))
        self.assertTrue(order.index(2) < order.index(4))
        self.assertTrue(order.index(3) < order.index(4))


if __name__ == '__main__':
    print("=" * 50)
    print("  DependencyGraph Python 验证测试")
    print("=" * 50)
    print()
    
    # 运行测试
    unittest.main(verbosity=2)

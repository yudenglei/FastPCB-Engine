# FastPCB-Engine

楂橀€烶CB寮曟搸 - 闈㈠悜鐧句竾绾у櫒浠剁殑OCAF浼樺寲瀹炵幇

## 椤圭洰鐩爣

- 鏀寔800涓囬噺绾ч珮閫烶CB璁捐
- 鍐呭瓨鍗犵敤鎺у埗鍦?0GB浠ュ唴锛堢洰鏍?-5GB锛?- 淇濈暀鐜版湁OCAF鍙傛暟鍖栧彉閲忔満鍒?- 鏀寔鍒嗗眰鏁版嵁缁撴瀯绠＄悊

## 鏍稿績鎶€鏈壒鎬?
### 1. 鍐呭瓨浼樺寲

- **Union缁撴瀯鍙傛暟鍊?*: 16瀛楄妭绱у噾瀛樺偍 vs 浼犵粺100+瀛楄妭
- **棰勫垎閰嶅唴瀛樻睜**: 閬垮厤棰戠箒malloc/free
- **Reference姹犲寲**: 鏁存暟ID浠ｆ浛Label寮曠敤
- **寤惰繜鍔犺浇**: 鎸夐渶瀹炰緥鍖?
### 2. 鍙傛暟鍖栦緷璧栧浘锛堟湁鍚戝浘DAG锛?
- **鎷撴墤鎺掑簭**: 鑷姩纭畾璁＄畻椤哄簭
- **澧為噺鏇存柊**: 鍙洿鏂板彈褰卞搷鑺傜偣
- **寰幆妫€娴?*: 闃叉鏃犻檺閫掑綊

### 3. 鍒嗗眰鏁版嵁缁撴瀯

- Via: PadTop + PadBottom + Drill
- BondWire: Start + End + Curve + Surface
- Trace: Path + Width + Layer
- Surface: Boundary + Holes + Net

### 4. 鎬ц兘浼樺寲

- 澶氱嚎绋嬫壒閲忓姞杞斤紙8绾跨▼锛?- LRU缂撳瓨
- 鎵归噺鎿嶄綔

## 浠ｇ爜缁撴瀯

```
FastPCB-Engine/
鈹溾攢鈹€ CMakeLists.txt          # 鏋勫缓閰嶇疆
鈹溾攢鈹€ main.cpp               # 涓荤▼搴?鈹溾攢鈹€ benchmark.cpp          # 鎬ц兘娴嬭瘯
鈹溾攢鈹€ include/               # 澶存枃浠?(18涓?
鈹?  鈹溾攢鈹€ PCBConfig.h         # 閰嶇疆
鈹?  鈹溾攢鈹€ PCBMemoryPool.h    # 鍐呭瓨姹?鈹?  鈹溾攢鈹€ PCBVariablePool.h  # 鍙傛暟鍖栧彉閲忔睜
鈹?  鈹溾攢鈹€ PCBParamGraph.h    # 鍙傛暟鍖栦緷璧栧浘
鈹?  鈹溾攢鈹€ PCBRefPool.h       # Reference姹?鈹?  鈹溾攢鈹€ PCBComponent.h     # 鍣ㄤ欢
鈹?  鈹溾攢鈹€ PCBVia.h           # 杩囧瓟
鈹?  鈹溾攢鈹€ PCBBondWire.h     # 閿悎绾?鈹?  鈹溾攢鈹€ PCBTrace.h        # 璧扮嚎
鈹?  鈹溾攢鈹€ PCBSurface.h      # 閾滅毊
鈹?  鈹溾攢鈹€ PCBShapeAttribute.h# Shape灞炴€?鈹?  鈹溾攢鈹€ PCBDataModel.h    # 鏁版嵁妯″瀷
鈹?  鈹溾攢鈹€ PCBLoader.h       # 鎵归噺鍔犺浇鍣?鈹?  鈹溾攢鈹€ PCBGraphNode.h    # 鎷撴墤缃戠粶
鈹?  鈹溾攢鈹€ PCBDesignRule.h   # DRC妫€鏌?鈹?  鈹溾攢鈹€ PCBSelectionSet.h # 閫夋嫨闆?鈹?  鈹溾攢鈹€ PCBTransaction.h  # 浜嬪姟绠＄悊
鈹?  鈹斺攢鈹€ PCBDiagnostics.h  # 鎬ц兘鐩戞帶
鈹溾攢鈹€ src/                   # 瀹炵幇 (19涓?
鈹斺攢鈹€ tests/                 # 鍗曞厓娴嬭瘯 (3涓?
    鈹溾攢鈹€ test_memory_pool.cpp
    鈹溾攢鈹€ test_components.cpp
    鈹斺攢鈹€ test_paramgraph.cpp
```

## 鏋勫缓

```bash
mkdir build
cd build
cmake ..
make
```

## 娴嬭瘯

```bash
# 杩愯涓荤▼搴?./FastPCBEngine

# 杩愯鍩哄噯娴嬭瘯
./benchmark

# 杩愯鍗曞厓娴嬭瘯
./test_memory_pool
./test_components
./test_paramgraph
```

## 鍐呭瓨鐩爣

| 鍣ㄤ欢鏁伴噺 | 鐩爣鍐呭瓨 | 棰勪及 |
|---------|---------|------|
| 100涓?| < 2GB | ~500MB |
| 500涓?| < 5GB | ~2GB |
| 800涓?| < 10GB | ~3GB |

## 鏂囨。

- [README](README.md) - 椤圭洰姒傝堪
- [TECHNICAL_DESIGN.md](docs/TECHNICAL_DESIGN.md) - 璇︾粏鎶€鏈璁?- [OPTIMIZATION.md](docs/OPTIMIZATION.md) - 浼樺寲鎬荤粨

## 璁稿彲璇?
MIT

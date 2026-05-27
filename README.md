# 火车票管理系统

SJTU CS1951 课程大作业

## 项目结构

采用 header-only 设计，所有代码都放在 `includes` 目录下，主入口在 `src/main.cpp`。模块划分如下：

```text
.
├── includes                                                  
│   ├── common                                                  
│   │   ├── algorithm.hpp     # 常用算法库工具，包括内省排序、查找、最值                                       
│   │   ├── date_time.hpp     # 一个轻量日期库，全部用 int 实现                                      
│   │   ├── executor.hpp      # 命令执行器，负责执行解析后的命令
│   │   ├── fixed_string.hpp  # 定长字符串类                                         
│   │   ├── optional.hpp      # std::optional 的轻量实现                                     
│   │   ├── parser.hpp        # 命令解析                                   
│   │   ├── types.hpp         # 自定义类型定义                                  
│   │   └── util.hpp          # 工具函数                                  
│   ├── containers                                          
│   │   ├── exceptions.hpp                                            
│   │   ├── list.hpp          # 链表                                
│   │   ├── map.hpp           # 红黑树                                
│   │   ├── unordered_map.hpp # 哈希表                                          
│   │   ├── utility.hpp       # pair                                    
│   │   └── vector.hpp        # 动态数组                                   
│   ├── managers                                            
│   │   ├── order_manager.hpp # 订单管理器                                           
│   │   ├── train_manager.hpp # 车次管理器                                           
│   │   └── user_manager.hpp  # 用户管理器                                          
│   └── storage                                            
│       ├── b_plus_tree.hpp   # B+ 树                                         
│       └── memory_river.hpp  # 可持久化对象存储器                                         
├── src                                            
│   ├── main.cpp              # 主入口
│   └── test_bpt.cpp          # B+ 树测试入口
├── tests                     # 单元测试
├── testcases                 # 本地样例数据
├── management_system.md
```

## 执行逻辑

- 程序启动后进入命令行界面，等待用户输入命令
- 用户输入命令后，命令解析器将其解析成命令对象
- 命令对象被命令执行器执行，调用相应的管理器方法
- 管理器方法操作外存数据结构，完成业务逻辑

## 外存结构

### `BPlusTree`

`includes/storage/b_plus_tree.hpp` 是 B+ 树模板，支持：

- `insert(key, value)`
- `erase(key, value)`
- `find(key, value)`
- `find_all(key)`
- `find_nth(key, n, value)`
- `clean()`

实现特性：

- 4096 字节页
- 删除页回收
- 页级 LRU 缓存
- 重复 `(key, value)` 插入会被忽略

### `MemoryRiver`

`includes/storage/memory_river.hpp` 是定长对象外存数组，支持：

- `write`
- `read`
- `update`
- `erase`
- `size`
- `clean`

实现特性：

- 删除对象通过 free list 复用空间


## 持久化文件

程序会在运行目录创建数据文件和索引文件，包括：

- `users.dat`, `username.idx`
- `trains.dat`, `train_index.idx`
- `train_seats.dat`, `train_seat.idx`
- `stations.dat`, `station_id.idx`
- `train_station.idx`, `reachable_to.idx`, `reachable_from.idx`
- `orders.dat`, `order_username.idx`, `order_pending.idx`

执行 `clean` 命令会清空所有业务数据和索引。